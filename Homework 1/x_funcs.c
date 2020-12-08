#define _POSIX_C_SOURCE 200809

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "x_funcs.h"

void printHW(const char *msg)
{
    size_t len = strlen(msg);
    if (len != write(STDOUT_FILENO, msg, len)) {
        errExit("printHW, write");
    }
}

void errExit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void wLockFile(int fd, int sleepTime)
{
    struct flock lock;
    memset(&lock, '\0', sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (0 > fcntl(fd, F_SETLKW, &lock)) {
        errExit("wUnlockFile, fcntl");
    }
}

void wUnlockFile(int fd, int sleepTime)
{
    struct flock lock;
    memset(&lock, '\0', sizeof(lock));
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (0 > fcntl(fd, F_SETLKW, &lock)) {
        errExit("wUnlockFile, fcntl");
    }
}

off_t getFileSize(int fd)
{
    struct stat fStat;
    if (0 > fstat(fd, &fStat)) {
        errExit("getFileSize, fd, fstat");
    }
    return fStat.st_size;
}

void expandFile(int fd, off_t empty, off_t size, size_t strLen)
{
    if (0 > ftruncate(fd, size+strLen-1)) {
        errExit("expandFile, fd, ftruncate");
    }
    char c[MOVE_BUF_SIZE];
    size_t moveSize = size-empty;
    int mod = moveSize % MOVE_BUF_SIZE;
    size_t partSize = 0 != mod ? mod : MOVE_BUF_SIZE;
    for (int i = moveSize; 0 < i;) {
        memset(&c, '\0', sizeof(c));
        if (partSize != pread(fd, &c, partSize, size-i)) {
            errExit("expandFile, pread");
        }
        if (partSize != pwrite(fd, &c, partSize, size-i+strLen)) {
            errExit("expandFile, pwrite");
        }
        i -= partSize;
        if (mod == partSize) {
            partSize = MOVE_BUF_SIZE;
        }
    }
    lseek(fd, empty-1, SEEK_SET);
}

void shrinkFile(int fd, off_t line, off_t size, size_t strLen)
{
    char c[MOVE_BUF_SIZE];
    size_t moveSize = size - (line + strLen);
    int mod = moveSize % MOVE_BUF_SIZE;
    size_t partSize = 0 != mod ? mod : MOVE_BUF_SIZE;
    for (int i = 0; i < moveSize;) {
        memset(&c, '\0', sizeof(c));
        if (partSize != pread(fd, &c, partSize, line+strLen+i)) {
            errExit("shrinkFile, pread");
        }
        if (partSize != pwrite(fd, &c, partSize, line+i)) {
            errExit("shrinkFile, pwrite");
        }
        i += partSize;
        if (mod == partSize) {
            partSize = MOVE_BUF_SIZE;
        }
    }
    if (0 > ftruncate(fd, size-strLen)) {
        errExit("shrinkFile, ftruncate");
    }
    lseek(fd, line-1, SEEK_SET);
}

off_t getEmptyLineCursor(int *newFD, int oldFD)
{
    off_t count = 1;
    *newFD = dup(oldFD);
    lseek(*newFD, 0, SEEK_SET);
    int r;
    char c, oC = '\0';
    while (0 < (r = read(*newFD, &c, 1))) {
        ++count;
        if ('\n' == c && '\n' == oC) {
            break;
        } else if ('\0' == c && '\n' == oC) {
            break;
        } else if ('\n' == c && '\0' == oC) {
            break;
        }
        oC = c;
    }
    if (0 > r) {
        errExit("getEmptyLineCursor, read");
    }
    return count;
}

void mSleep(int milliseconds)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000 * milliseconds * 10;
    nanosleep(&ts, NULL);
}

off_t getLine(int fd, char *str)
{
    int newFD = dup(fd);
    off_t len = 0;
    int r;
    while (0 < (r = read(newFD, str, 1))) {
        if ('\n' == *str || '\0' == *str) {
            *str = '\0';
            return len;
        }
        ++str;
        ++len;
    }
    if (0 > r) {
        errExit("getLine, read");
    }
    return -1;
}