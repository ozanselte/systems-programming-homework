#define _POSIX_C_SOURCE 200809
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include "funcs.h"
#include "process1.h"

static int inFD, outFD;

void mainP1(const char *inputPath, const char *outputPath)
{
    struct sigaction sa;
    sa.sa_handler = sigHandlerP1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (0 > sigaction(SIGUSR2, &sa, NULL)) {
        errExit("mainP1, sigaction, sa");
    }
    if (0 > sigaction(SIGTERM, &sa, NULL)) {
        errExit("mainP1, sigaction, sa");
    }
    inFD = open(inputPath, O_RDONLY);
    if (0 > inFD) {
        errExit("mainP1, open, inFD");
    }
    outFD = open(outputPath, O_RDWR | O_CREAT | O_SYNC, 0644);
    if (0 > outFD) {
        errExit("mainP1, open, outFD");
    }
    fileOpsP1();
    if (0 > close(inFD)) {
        errExit("mainP1, close, inFD");
    }
    if (0 > close(outFD)) {
        errExit("mainP1, close, outFD");
    }
    if (0 > kill(getppid(), SIGUSR1)) {
        errExit("mainP1, kill, SIGUSR1");
    }
}

void fileOpsP1()
{
    ssize_t readCount, processedBytes = 0;
    uint8_t inBuf[INPUT_BUF_SIZE];
    char *outCursor, outBuf[TEMP_BUF_SIZE];
    double sumX = 0, sumY = 0, sumX2 = 0, sumXY = 0;
    uintmax_t processedLines = 0, pendingCount = 0;
    while (INPUT_BUF_SIZE <= (readCount = read(inFD, inBuf, INPUT_BUF_SIZE))) {
        sumX = 0;
        sumY = 0;
        sumX2 = 0;
        sumXY = 0;
        signalBlock();
        wLockFile(outFD);
        lseek(outFD, 0, SEEK_END);
        memset(outBuf, '\0', TEMP_BUF_SIZE);
        outCursor = outBuf;
        for (int i = 0; i < INPUT_BUF_SIZE; i += 2) {
            if (i) {
                outCursor += sprintf(outCursor, ",");
            }
            uint8_t x = inBuf[i];
            uint8_t y = inBuf[i+1];
            sumX += x;
            sumY += y;
            sumX2 += x * x;
            sumXY += x * y;
            outCursor += sprintf(outCursor, "(%d,%d)", x, y);
        }
        double lsA = leastSquaresA(sumX, sumY, sumX2, sumXY);
        double lsB = leastSquaresB(lsA, sumX, sumY);
        outCursor += sprintf(outCursor, ",%+.3fx%+.3f\n", lsA, lsB);
        size_t len = outCursor - outBuf;
        if (len > write(outFD, outBuf, len)) {
            errExit("fileOpsP1, write, outFD");
        }
        pendingCount += isPendingSignal(SIGINT);
        wUnlockFile(outFD);
        signalUnblock();
        processedBytes += readCount;
        ++processedLines;
        if (0 > kill(getppid(), SIGCONT)) {
            errExit("fileOpsP1, kill, SIGCONT");
        }
    }
    if (0 > readCount) {
        errExit("fileOpsP1, read, inFD");
    }
    printf("%lu bytes and %lu lines processed. ", processedBytes, processedLines);
    printf("%lu SIGINT blocked in critical section.\n", pendingCount);
}

void sigHandlerP1(int sig)
{
    if (SIGTERM == sig) {
        kill(getppid(), SIGUSR2);
    }
    close(inFD);
    close(outFD);
    _exit(EXIT_SUCCESS);
}

double leastSquaresA(double x, double y, double xx, double xy)
{
    uintmax_t n = INPUT_BUF_SIZE / 2;
    return (n*xy - x*y) / (n*xx - x*x);
}

double leastSquaresB(double m, double x, double y)
{
    uintmax_t n = INPUT_BUF_SIZE / 2;
    return (y - m*x) / n;
}