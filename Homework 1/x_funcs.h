#ifndef __X_FUNCS_H__
#define __X_FUNCS_H__

#define _FILE_OFFSET_BITS 64
#define MOVE_BUF_SIZE (8*16384)
#define MAX_SLEEP_TIME (50)
#define MIN_SLEEP_TIME (1)
#define SAFETY_MULTIPLIER (8)
#define TRUE (1)
#define FALSE (0)

void printHW(const char *msg);
void errExit(const char *msg);
void wLockFile(int fd, int sleepTime);
void wUnlockFile(int fd, int sleepTime);
void rLockFile(int fd);
void rUnlockFile(int fd);
off_t getFileSize(int fd);
void expandFile(int fd, off_t empty, off_t size, size_t strLen);
void shrinkFile(int fd, off_t line, off_t size, size_t strLen);
off_t getEmptyLineCursor(int *newFD, int oldFD);
void mSleep(int milliseconds);
off_t getLine(int fd, char *str);

#endif