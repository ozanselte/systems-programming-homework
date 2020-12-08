#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "defs_and_types.h"
#include "funcs.h"
#include "processes.h"

static struct MatrixMult *mm = NULL;
static uint8_t **M[4] = {NULL, NULL, NULL, NULL};
static uint64_t **C = NULL;
static volatile sig_atomic_t isChildAlive = 0;
static volatile sig_atomic_t canExit = 0;
static volatile sig_atomic_t waitFD = -1;

void checkProcess()
{
    if (PARENT_EXIT == canExit) {
        if (NULL != C) {
            freeMatrix64(C, 2 * mm->qSide);
            C = NULL;
        }
        for (uint8_t i = 0; i < 4; ++i) {
            if (NULL != M[i]) {
                free(M[i]);
                M[i] = NULL;
            }
        }
        freeMM(mm);
        while (0 != isChildAlive) {
            pid_t pid = waitpid(WAIT_ANY, NULL, 0);
            if (-1 == pid) {
                if (ECHILD == errno) {
                    isChildAlive = 0;
                } else {
                    errExit("checkProcess, waitpid, pid");
                }
            }
        }
        _exit(EXIT_SUCCESS);
    } else if (CHILD_EXIT == canExit) {
        if (-1 != waitFD) {
            close(waitFD);
            waitFD = -1;
        }
        for (uint8_t i = 0; i < 4; ++i) {
            if (NULL != M[i]) {
                freeMatrix8(M[i], mm->qSide);
                M[i] = NULL;
            }
        }
        if (NULL != C) {
            freeMatrix64(C, mm->qSide);
            C = NULL;
        }
        _exit(EXIT_SUCCESS);
    }
}

void linkHandlerParent()
{
    struct sigaction sa;
    sa.sa_handler = signalHandlerParent;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (0 > sigaction(SIGCHLD, &sa, NULL)) {
        errExit("linkHandlerParent, sigaction, sa");
    }
    if (0 > sigaction(SIGINT, &sa, NULL)) {
        errExit("linkHandlerParent, sigaction, sa");
    }
}

void linkHandlerChild()
{
    struct sigaction sa;
    sa.sa_handler = signalHandlerChild;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (0 > sigaction(SIGINT, &sa, NULL)) {
        errExit("linkHandlerChild, sigaction, sa");
    }
}

void signalHandlerParent(int signo)
{
    int errnoBackup = errno;
    pid_t pid = 0;
    switch (signo) {
        case SIGCHLD:
            do {
                pid = waitpid(WAIT_ANY, NULL, WNOHANG);
                if (-1 == pid) {
                    if (ECHILD == errno) {
                        isChildAlive = 0;
                    } else {
                        errExit("signalHandlerParent, waitpid, pid");
                    }
                }
            } while (0 < pid);
            break;
        case SIGINT:
            canExit = PARENT_EXIT;
            break;
    }
    errno = errnoBackup;
}

void signalHandlerChild(int signo)
{
    int errnoBackup = errno;
    canExit = CHILD_EXIT;
    errno = errnoBackup;
}

void initProcesses(struct MatrixMult *mmS)
{
    mm = mmS;
    int pipes[9][2] = {0};
    // 0    -> WAIT
    // i    -> PtoC
    // i+1  -> CtoP
    signalBlock();
    if (-1 == pipe(pipes[0])) {
        errExit("initProcesses, pipe, pipes[0]");
    }
    waitFD = pipes[0][WR];
    for (uint8_t i = 1; i < 9; i += 2) {
        if (-1 == pipe(pipes[i])) {
            errExit("initProcesses, pipe, pipes[i]");
        }
        if (-1 == pipe(pipes[i+1])) {
            errExit("initProcesses, pipe, pipes[i+1]");
        }
        switch (fork()) {
            case -1:
                errExit("initProcesses, fork");
                break;
            case 0:
                if (-1 == close(pipes[0][RD])) {
                    errExit("initProcesses, close, pipes[0][RD]");
                }
                if (-1 == close(pipes[i][WR])) {
                    errExit("initProcesses, close, pipes[i][WR]");
                }
                if (-1 == close(pipes[i+1][RD])) {
                    errExit("initProcesses, close, pipes[i+1][RD]");
                }
                freeMM(mm);
                closeOlderPipes(pipes, i);
                linkHandlerChild();
                childMain(pipes[i][RD], pipes[i+1][WR]);
                if (-1 == close(pipes[i+1][WR])) {
                    errExit("initProcesses, close, pipes[i+1][WR]");
                }
                if (-1 == close(pipes[i][RD])) {
                    errExit("initProcesses, close, pipes[i][RD]");
                }
                signalUnblock();
                _exit(EXIT_SUCCESS);
                break;
            default:
                ++isChildAlive;
                if (-1 == close(pipes[i][RD])) {
                    errExit("initProcesses, close, pipes[i][RD]");
                }
                if (-1 == close(pipes[i+1][WR])) {
                    errExit("initProcesses, close, pipes[i+1][WR]");
                }
        }
    }
    linkHandlerParent();
    if (-1 == close(pipes[0][WR])) {
        errExit("initProcesses, close, pipes[0][WR]");
    }
    int rdFD[4], wrFD[4];
    for (uint8_t i = 1; i <= 4; ++i) {
        rdFD[i-1] = pipes[2*i][RD];
        wrFD[i-1] = pipes[(2*i)-1][WR];
    }
    parentMain(pipes[0][RD], rdFD, wrFD);
    if (-1 == close(pipes[0][RD])) {
        errExit("initProcesses, close, pipes[0][RD]");
    }
    freeMM(mm);
    signalUnblock();
}

void closeOlderPipes(int pipes[][2], uint8_t current)
{
    for (uint8_t i = 1; i < current; i += 2) {
        if (-1 == close(pipes[i][WR])) {
            errExit("closeOtherPipes, close, pipes[i][WR]");
        }
        if (-1 == close(pipes[i+1][RD])) {
            errExit("closeOtherPipes, close, pipes[i+1][RD]");
        }
    }
}

void childMain(int rdFD, int wrFD)
{
    signalUnblock();
    size_t side;
    if (-1 == read(rdFD, &side, sizeof(side))) {
        errExit("childMain, read, rdFD");
    }
    for (uint8_t i = 0; i < 4; ++i) {
        signalBlock();
        M[i] = createMatrix8(side);
        signalUnblock();
        receiveMatrix8(rdFD, M[i], side);
    }
    signalBlock();
    C = multiplyMatrices(M, side);
    if (-1 == close(waitFD)) {
        errExit("childMain, close, waitFD");
    }
    waitFD = -1;
    for (uint8_t i = 0; i < 4; ++i) {
        freeMatrix8(M[i], side);
        M[i] = NULL;
    }
    signalUnblock();
    sendMatrix64(wrFD, C, side);
    signalBlock();
    freeMatrix64(C, side);
    C = NULL;
}

void parentMain(int wtFD, int rdFD[4], int wrFD[4])
{
    signalUnblock();
    uint8_t i;
    for (i = 0; i < 4; ++i) {
        if (-1 == write(wrFD[i], &(mm->qSide), sizeof(mm->qSide))) {
            errExit("parentMain, write, wrFD[i]");
        }
    }
    uint8_t **Q[4];
    for (i = 0; i < 4; ++i) {
        switch (i) {
            case 0:
                Q[0] = mm->quarterA[0]; Q[1] = mm->quarterA[0];
                Q[2] = mm->quarterA[2]; Q[3] = mm->quarterA[2];
                break;
            case 1:
                Q[0] = mm->quarterA[1]; Q[1] = mm->quarterA[1];
                Q[2] = mm->quarterA[3]; Q[3] = mm->quarterA[3];
                break;
            case 2:
                Q[0] = mm->quarterB[0]; Q[1] = mm->quarterB[1];
                Q[2] = mm->quarterB[0]; Q[3] = mm->quarterB[1];
                break;
            case 3:
                Q[0] = mm->quarterB[2]; Q[1] = mm->quarterB[3];
                Q[2] = mm->quarterB[2]; Q[3] = mm->quarterB[3];
                break;
        }
        sendMatrices8(wrFD, Q, mm->qSide);
    }
    if (0 != read(wtFD, &i, 1)) {
        errExit("parentMain, read, wtFD");
    }
    signalBlock();
    C = createMatrix64(2 * mm->qSide);
    divideMatrix64(C, (uint64_t ***)M, 2 * mm->qSide);
    signalUnblock();
    receiveMatrices64(rdFD, (uint64_t ***)M, mm->qSide);
    signalBlock();
    for (uint8_t i = 0; i < 4; ++i) {
        free(M[i]);
        M[i] = NULL;
    }
    #ifdef __DEBUG__
        printMatrix64(C, 2 * mm->qSide);
    #endif
    while (isChildAlive) {
        waitSignal();
    }
    svdHelper();
    freeMatrix64(C, 2 * mm->qSide);
    C = NULL;
}

void sendPiece8(int fd, uint8_t *ptr, ssize_t size)
{
    checkProcess();
    size *= sizeof(uint8_t);
    ssize_t sended = write(fd, ptr, size);
    if (-1 == sended) {
        errExit("sendPiece8, write, fd");
    }
    else if (size != sended) {
        //printStr("Unexpected error! See sendPiece8 in funcs.\n");
        //exit(EXIT_FAILURE);
    }
}

void sendPiece64(int fd, uint64_t *ptr, ssize_t size)
{
    checkProcess();
    size *= sizeof(uint64_t);
    ssize_t sended = write(fd, ptr, size);
    if (-1 == sended) {
        errExit("sendPiece64, write, fd");
    }
    else if (size != sended) {
        //printStr("Unexpected error! See sendPiece64 in funcs.\n");
        //exit(EXIT_FAILURE);
    }
}

void sendMatrix64(int fd, uint64_t **M, size_t side)
{
    for (size_t i = 0; i < side; ++i) {
        size_t piece = PIECE_SIZE / sizeof(uint64_t);
        uint64_t *ptr = M[i];
        for (size_t left = side; 0 < left; left -= piece) {
            piece = MIN(left, piece);
            sendPiece64(fd, ptr, piece);
            ptr += piece;
        }
    }
}

void sendMatrices8(int fd[4], uint8_t **M[4], size_t side)
{
    for (size_t i = 0; i < side; ++i) {
        size_t piece = PIECE_SIZE;
        ssize_t pos = 0;
        for (size_t left = side; 0 < left; left -= piece) {
            piece = _MIN(left, piece);
            for (uint8_t j = 0; j < 4; ++j) {
                sendPiece8(fd[j], M[j][i]+pos, piece);
            }
            pos += piece;
            
        }
    }
}

void receivePiece8(int fd, uint8_t *ptr, ssize_t size)
{
    checkProcess();
    size *= sizeof(uint8_t);
    ssize_t sended = read(fd, ptr, size);
    if (-1 == sended) {
        errExit("receivePiece8, read, fd");
    }
    else if (size != sended) {
        //printStr("Unexpected error! See receivePiece8 in funcs.\n");
        //exit(EXIT_FAILURE);
    }
}

void receivePiece64(int fd, uint64_t *ptr, ssize_t size)
{
    checkProcess();
    size *= sizeof(uint64_t);
    ssize_t sended = read(fd, ptr, size);
    if (-1 == sended) {
        errExit("receivePiece64, read, fd");
    }
    else if (size != sended) {
        //printStr("Unexpected error! See receivePiece64 in funcs.\n");
        //exit(EXIT_FAILURE);
    }
}

void receiveMatrix8(int fd, uint8_t **M, size_t side)
{
    for (size_t i = 0; i < side; ++i) {
        ssize_t piece = PIECE_SIZE;
        uint8_t *ptr = M[i];
        for (size_t left = side; 0 < left; left -= piece) {
            piece = MIN(left, piece);
            receivePiece8(fd, ptr, piece);
            ptr += piece;
        }
    }
}

void receiveMatrices64(int fd[4], uint64_t **M[4], size_t side)
{
    for (size_t i = 0; i < side; ++i) {
        size_t piece = PIECE_SIZE / sizeof(uint64_t);
        ssize_t pos = 0;
        for (size_t left = side; 0 < left; left -= piece) {
            piece = MIN(left, piece);
            for (uint8_t j = 0; j < 4; ++j) {
                receivePiece64(fd[j], M[j][i]+pos, piece);
            }
            pos += piece;
            
        }
    }
}

uint64_t **multiplyMatrices(uint8_t **M[4], size_t side)
{
    uint64_t tmp;
    uint64_t **C = createMatrix64(side);
    for (size_t y = 0; y < side; ++y) {
        for (size_t x = 0; x < side; ++x) {
            tmp = 0;
            for (size_t i = 0; i < 2*side; ++i) {
                if (i < side) {
                    tmp += M[0][y][i] * M[2][i][x];
                } else {
                    tmp += M[1][y][i-side] * M[3][i-side][x];
                }
            }
            C[y][x] = tmp;
            checkProcess();
        }
    }
    return C;
}

void freeMM(struct MatrixMult *mm)
{
    for (uint8_t i = 0; i < 4; ++i) {
        if (NULL != mm->quarterA[i]) {
            free(mm->quarterA[i]);
            mm->quarterA[i] = NULL;
        }
        if (NULL != mm->quarterB[i]) {
            free(mm->quarterB[i]);
            mm->quarterB[i] = NULL;
        }
    }
    if (NULL != mm->fullA) {
        freeMatrix8(mm->fullA, 2 * mm->qSide);
        mm->fullA = NULL;
    }
    if (NULL != mm->fullB) {
        freeMatrix8(mm->fullB, 2 * mm->qSide);
        mm->fullB = NULL;
    }
}

void signalBlock()
{
    checkProcess();
    sigset_t blockSet;
    sigemptyset(&blockSet);
    sigaddset(&blockSet, SIGINT);
    if (0 > sigprocmask(SIG_BLOCK, &blockSet, NULL)) {
        errExit("signalBlock, sigprocmask, blockset");
    }
}

void signalUnblock()
{
    sigset_t blockSet;
    sigemptyset(&blockSet);
    sigaddset(&blockSet, SIGINT);
    if (0 > sigprocmask(SIG_UNBLOCK, &blockSet, NULL)) {
        errExit("signalUnblock, sigprocmask, blockset");
    }
    checkProcess();
}

void waitSignal()
{
    checkProcess();
    sigset_t mask;
    sigemptyset(&mask);
    sigdelset(&mask, SIGCHLD);
    sigdelset(&mask, SIGINT);
    if (0 > sigsuspend(&mask) && EINTR != errno) {
        errExit("waitSignal, sigsuspend, mask");
    }
    checkProcess();
}

void svdHelper()
{
    size_t side = 2 * mm->qSide;
    float **a = (float **)xcalloc(side, sizeof(float *));
    float **v = (float **)xcalloc(side, sizeof(float *));
    float *w = (float *)xcalloc(side, sizeof(float));
    for(size_t i = 0; i < side; ++i) {
        a[i] = (float *)xcalloc(side, sizeof(float));
        v[i] = (float *)xcalloc(side, sizeof(float));
    }
    for(size_t i = 0; i < side; ++i) {
        for(size_t j = 0; j < side; ++j) {
            a[i][j] = C[i][j];
        }
    }
    dsvd(a, (int)side, (int)side, w, v);
    char str[256];
    for(size_t i = 0; i < side; ++i) {
        free(a[i]);
        free(v[i]);
        sprintf(str, "%lu -> %.3f\n", i, w[i]);
        printStr(str);
    }
    free(a);
    free(v);
    free(w);
}