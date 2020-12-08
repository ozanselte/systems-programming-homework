#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include "helper.h"
#include "structures.h"
#include "florist.h"

extern size_t floristsLen;
extern struct Florist *florists;
extern size_t flowersLen;
extern char **flowersArr;

static volatile sig_atomic_t exitSignal = 0;

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

size_t strslen(char *str, char sep)
{
    size_t len = 0;
    while (sep != str[len] && '\0' != str[len])
    {
        ++len;
    }
    return len;
}

void readLine(int fd, char *line)
{
    char chr;
    for (;;) {
        if (-1 == read(fd, &chr, 1)) {
            errExit("readLine, read");
        }
        if ('\n' == chr) {
            *line = '\0';
            break;
        } else {
            *line = chr;
        }
        ++line;
    }
}

void sleepMs(size_t ms)
{
    struct timespec req = {0};
    size_t ns = ms * 1000000L;
    req.tv_sec = ns / 1000000000L;
    req.tv_nsec = ns % 1000000000L;
    for (;;) {
        if (0 != nanosleep(&req, (struct timespec *)NULL)) {
            if (EINTR == errno) {
                checkExitSignal();
                continue;
            }
            errExit("sleepMs, nanosleep");
        }
        break;
    }
}

void linkParentHandler()
{
    struct sigaction sa;
    sa.sa_handler = signalHandlerParent;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (0 > sigaction(SIGINT, &sa, NULL)) {
        errExit("linkParentHandler, sigaction");
    }
}

void signalHandlerParent(int signo)
{
    int errnoBackup = errno;
    exitSignal = 1;
    errno = errnoBackup;
}

void checkExitSignal()
{
    if (!exitSignal) {
        return;
    }
    for (size_t i = 0; i < floristsLen; ++i) {
        pthread_t self = pthread_self();
        if (0 != pthread_equal(self, florists[i].id)) {
            if (0 != pthread_cancel(florists[i].id)) {
                errExit("signalHandlerParent, pthread_cancel");
            }
        }
    }
    pthread_exit(PTHREAD_CANCELED);
}

void exitGracefully()
{
    joinThreads();
    if (exitSignal) {
        printStr("Exiting because of SIGINT\n");
    } else {
        printStatistics();
    }
    freeStructures();
    _Exit(EXIT_SUCCESS);
}

void printStatistics()
{
    char str[LINE_LEN];
    printStr("Sale statistics for today:\n");
    printStr("----------------------------------------\n");
    printStr("Florist        # of sales     Total time\n");
    printStr("----------------------------------------\n");
    for (size_t i = 0; i < floristsLen; ++i) {
        sprintf(str, "%-12s   %-12zu   %zums\n",
            florists[i].name,
            florists[i].sales,
            florists[i].totalTime
        );
        printStr(str);
    }
    printStr("----------------------------------------\n");
}