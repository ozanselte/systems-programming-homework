#ifndef __PROCESSES_H__
#define __PROCESSES_H__

#define RD (0)
#define WR (1)
#define PARENT_EXIT (1)
#define CHILD_EXIT (2)

struct MatrixMult {
    uint8_t **quarterA[4];
    uint8_t **quarterB[4];
    uint8_t **fullA;
    uint8_t **fullB;
    size_t qSide;
};

void checkProcess();
void linkHandlerParent();
void linkHandlerChild();
void signalHandlerParent(int signo);
void signalHandlerChild(int signo);

void initProcesses(struct MatrixMult *mmS);
void closeOlderPipes(int pipes[][2], uint8_t current);
void childMain(int rdFD, int wrFD);
void parentMain(int wtFD, int rdFD[4], int wrFD[4]);

void sendPiece8(int fd, uint8_t *ptr, ssize_t size);
void sendPiece64(int fd, uint64_t *ptr, ssize_t size);
void sendMatrix64(int fd, uint64_t **M, size_t side);
void sendMatrices8(int fd[4], uint8_t **M[4], size_t side);

void receivePiece8(int fd, uint8_t *ptr, ssize_t size);
void receivePiece64(int fd, uint64_t *ptr, ssize_t size);
void receiveMatrix8(int fd, uint8_t **M, size_t side);
void receiveMatrices64(int fd[4], uint64_t **M[4], size_t side);

uint64_t **multiplyMatrices(uint8_t **M[4], size_t side);
void freeMM(struct MatrixMult *mm);
void signalBlock();
void signalUnblock();
void waitSignal();

void svdHelper();

#endif