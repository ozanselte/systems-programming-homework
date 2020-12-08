#define _POSIX_C_SOURCE 200809
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>

#include "funcs.h"

void printUsage(const char *name)
{
    printf("ProgramA Usage:\n\t");
    printf("%s -i INPUT -o OUTPUT\n", name);
    printf("All arguments are mandatory.\n");
    printf("\t-i\tAbsolute path of the input file\n");
    printf("\t-o\tAbsolute path of the output file\n");
    exit(EXIT_FAILURE);
}

void errExit(const char *msg)
{
    perror(msg);
    printf("SIGTERM is sending to the other process.\n");
    kill(getpid(), SIGTERM);
    exit(EXIT_FAILURE);
}

void wLockFile(int fd)
{
    struct flock lock;
    memset(&lock, '\0', sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (0 > fcntl(fd, F_SETLKW, &lock)) {
        errExit("wLockFile, fcntl, fd");
    }
}

void wUnlockFile(int fd)
{
    struct flock lock;
    memset(&lock, '\0', sizeof(lock));
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (0 > fcntl(fd, F_SETLKW, &lock)) {
        errExit("wUnlockFile, fcntl, fd");
    }
}

void signalBlock()
{
    sigset_t blockSet;
    sigemptyset(&blockSet);
    sigaddset(&blockSet, SIGINT);
    sigaddset(&blockSet, SIGSTOP);
    sigaddset(&blockSet, SIGCONT);
    if (0 > sigprocmask(SIG_BLOCK, &blockSet, NULL)) {
        errExit("signalBlock, sigprocmask, blockset");
    }
}

void signalUnblock()
{
    sigset_t blockSet;
    sigemptyset(&blockSet);
    sigaddset(&blockSet, SIGINT);
    sigaddset(&blockSet, SIGSTOP);
    sigaddset(&blockSet, SIGCONT);
    if (0 > sigprocmask(SIG_UNBLOCK, &blockSet, NULL)) {
        errExit("signalUnblock, sigprocmask, blockset");
    }
}

int isPendingSignal(int signum)
{
    sigset_t sigset;
    if (0 > sigpending(&sigset)) {
        errExit("isPendingSignal, sigpending, sigset");
    }
    return sigismember(&sigset, signum);
}

char *createStr(const char *str)
{
    int len = strlen(str);
    char *ptr = (char *)calloc(len+1, sizeof(char));
    if (NULL == ptr) {
        errExit("createStr, calloc, ptr");
    }
    strcpy(ptr, str);
    return ptr;
}

off_t getFileSize(int fd)
{
    struct stat fStat;
    if (0 > fstat(fd, &fStat)) {
        errExit("getFileSize, fstat, fd");
    }
    return fStat.st_size;
}

off_t getLine(int oldFD, char *str)
{
    int fd = dup(oldFD);
    if (0 > fd) {
        errExit("getLine, dup, oldFD");
    }
    lseek(fd, 0, SEEK_SET);
    off_t len = 0;
    int count;
    while (0 < (count = read(fd, str, 1))) {
        if ('\n' == *str || '\0' == *str) {
            *str = '\0';
            break;
        }
        ++str;
        ++len;
    }
    if (0 > close(fd)) {
        errExit("getLine, close, fd");
        return -1;
    }
    if (0 > count) {
        errExit("getLine, read, fd");
    }
    return len;
}

void removeFirstLine(int fd, off_t size, size_t lineLen)
{
    char buf[MOVE_BUF_SIZE];
    for (int i = lineLen; i < size; i+=MOVE_BUF_SIZE) {
        memset(&buf, '\0', sizeof(buf));
        if (0 > pread(fd, &buf, sizeof(buf), i)) {
            errExit("removeFirstLine, pread, fd");
        }
        if (0 > pwrite(fd, buf, sizeof(buf), i-lineLen)) {
            errExit("removeFirstLine, pwrite, fd");
        }
    }
    if (0 > ftruncate(fd, size-lineLen)) {
        errExit("removeFirstLine, ftruncate, fd");
    }
}

void waitSignal()
{
    sigset_t mask;
    sigemptyset(&mask);
    if (0 > sigsuspend(&mask) && EINTR != errno) {
        errExit("waitSignal, sigsuspend, mask");
    }
}