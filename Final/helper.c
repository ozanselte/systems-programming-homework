#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "helper.h"

void printStr(const char *str)
{
    size_t len = strlen(str);
    if (-1 == write(STDOUT_FILENO, str, len)) {
        errExit("printStr, write, STDOUT_FILENO");
    }
}

void errExit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void readLine(int fd, char *line)
{
    char chr = '\0';
    for (;;) {
        if (-1 == read(fd, &chr, 1)) {
            errExit("readLine, read");
        }
        if ('\n' == chr || '\0' == chr) {
            *line = '\0';
            break;
        } else if ('\r' == chr) {
            continue;
        } else {
            *line = chr;
        }
        ++line;
    }
}

void startTimer(struct Timer *timer)
{
    if (-1 == gettimeofday( &timer->begin, NULL)) {
        errExit("startTimer, gettimeofday");
    }
}

void stopTimer(struct Timer *timer)
{
    if (-1 == gettimeofday(&timer->end, NULL)) {
        errExit("stopTimer, gettimeofday");
    }
}

double readTimer(struct Timer *timer)
{
    time_t sDiff = timer->end.tv_sec - timer->begin.tv_sec;
    suseconds_t uDiff = timer->end.tv_usec - timer->begin.tv_usec;
    double diff = (double)sDiff + ((double)uDiff / 1000000.0);
    return diff;
}

int lockInstance(char *filename)
{
    int fd;
    fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (-1 == fd) {
        errExit("lockInstance, open");
    }
    if (-1 == fcntl(fd, F_SETFD, FD_CLOEXEC)) {
        errExit("lockInstance, fcntl, F_SETFD");
    }
    struct flock lock;
    memset(&lock, '\0', sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (-1 == fcntl(fd, F_SETLK, &lock)) {
        if (EAGAIN == errno || EACCES == errno) {
            printStr("An instance is already running!\n");
            exit(EXIT_FAILURE);
        } else {
            errExit("lockInstance, fcntl");
        }
    }
    if (-1 == ftruncate(fd, 0)) {
        errExit("lockInstance, ftruncate");
    }
    char buf[LINE_LEN];
    sprintf(buf, "%ld\n", (long)getpid());
    if (strlen(buf) != write(fd, buf, strlen(buf))) {
        errExit("lockInstance, write");
    }
    return fd;
}

void signalBlock()
{
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
}

int isPendingSignal()
{
    sigset_t sigset;
    if (0 > sigpending(&sigset)) {
        errExit("isPendingSignal, sigpending, sigset");
    }
    return sigismember(&sigset, SIGINT);
}