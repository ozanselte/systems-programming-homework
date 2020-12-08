#define _POSIX_C_SOURCE 200809
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <complex.h>

#include "x_funcs.h"
#include "b_funcs.h"
#include "rosetta_fft.h"

void printUsage(const char *name)
{
    char *s0 = "ProgramB Usage:\n\t";
    char *s1 = " -i INPUT -o OUTPUT -t TIME\n";
    char *s2 = "All arguments are mandatory.\n";
    char *s3 = "\t-i\tAbsolute path of the input file\n";
    char *s4 = "\t-o\tAbsolute path of the output file\n";
    char *s5 = "\t-t\tInteger between 1 and 50\n";
    printHW(s0);
    printHW(name);
    printHW(s1);
    printHW(s2);
    printHW(s3);
    printHW(s4);
    printHW(s5);
    exit(EXIT_FAILURE);
}

void mainFuncB(const char *inPath, const char *outPath, int sleepTime)
{
    srand(time(0));
    int inFD = open(inPath, O_RDWR | O_CREAT | O_SYNC, 0644);
    if (0 > inFD) {
        errExit("mainFuncB, inFD, open");
    }
    int outFD = open(outPath, O_RDWR | O_CREAT | O_SYNC, 0644);
    if (0 > outFD) {
        errExit("mainFuncB, outFD, open");
    }
    off_t size = programB(inFD, outFD, sleepTime);
    if (isLater(outFD, sleepTime, size)) {
        if (0 > close(outFD)) {
            errExit("mainFuncB, outFD, close");
        }
        if (0 > close(inFD)) {
            errExit("mainFuncB, inFD, close");
        }
    }
}

off_t programB(int inFD, int outFD, int sleepTime)
{
    off_t line, size;
    unsigned long passedTime = 0;
    double complex nums[INPUT_NUMS_COUNT];
    while (1) {
        mSleep(sleepTime);
        wLockFile(inFD, sleepTime);
        wLockFile(outFD, sleepTime);
        if (0 > (line = getRandomLine(inFD))) {
            wUnlockFile(inFD, sleepTime);
            wUnlockFile(outFD, sleepTime);
            if (MAX_SLEEP_TIME*SAFETY_MULTIPLIER >= passedTime) {
                mSleep(sleepTime);
                passedTime += sleepTime;
                continue;
            } else {
                return size;
            }
        }
        passedTime = 0;
        memset(nums, '\0', sizeof(nums));
        processInputFile(inFD, sleepTime, line, nums);
        processOutputFile(outFD, sleepTime, nums);
        size = getFileSize(outFD);
        wUnlockFile(outFD, sleepTime);
        wUnlockFile(inFD, sleepTime);
    }
    return size;
}

void processInputFile(int inFD, int sleepTime, int line, double complex *nums)
{
    char buf[INPUT_BUF_SIZE];
    off_t size = getFileSize(inFD);
    memset(buf, '\0', sizeof(buf));
    off_t len = getLine(inFD, buf);
    buf[INPUT_BUF_SIZE-1] = '\0';
    shrinkFile(inFD, line, size, len);
    int j = 0, i;
    for (i = 0; i < INPUT_NUMS_COUNT; ++i) {
        int real, imag;
        sscanf(&(buf[j]), "%d+i%d[,\n]", &real, &imag);
        while (',' != buf[j] && '\0' != buf[j] && '\n' != buf[j]) {
            ++j;
        }
        if (',' == buf[j]) {
            ++j;
        } else if ('\0' == buf[j] || '\n' == buf[j]) {
            if (INPUT_NUMS_COUNT != i + 1) {
                exit(EXIT_FAILURE);
            }
            ++j;
        }
        nums[i] = real + I * imag;
    }
    fft(nums, INPUT_NUMS_COUNT);
}

void processOutputFile(int outFD, int sleepTime, double complex *nums)
{
    int count;
    char outBuf[OUTPUT_BUF_SIZE];
    char *outCursor = outBuf;
    memset(outBuf, '\0', sizeof(outBuf));
    for (int i = 0; i < INPUT_NUMS_COUNT; ++i) {
        if (0 != i) {
            count = sprintf(outCursor, ",");
            outCursor += count;
        }
        double real = creal(nums[i]);
        double imag = cimag(nums[i]);
        count = sprintf(outCursor, "%.3f+i%.3f", real, imag);
        outCursor += count;
    }
    count = sprintf(outCursor, "\n");
    outCursor += count;
    int emptyFD;
    off_t beginning = getEmptyLineCursor(&emptyFD, outFD);
    off_t len = outCursor-outBuf;
    off_t size = getFileSize(outFD);
    if (0 < size) {
        if (size+1 != beginning) {
            expandFile(emptyFD, beginning-1, size, len-1);
        } else {
            expandFile(emptyFD, beginning, size, len);
        }
    }
    if (len > write(emptyFD, outBuf, len)) {
        errExit("outFD, write");
    }
}

off_t getRandomLine(int fd)
{
    off_t size = getFileSize(fd);
    off_t random = rand() % (size + 1);
    off_t pos = 0 != random ? random - 1 : size - 1;
    lseek(fd, random, SEEK_SET);
    char c, o = '\0';
    int r;
    while (1 == (r = read(fd, &c, 1))) {
        if ('\n' != c && '\n' == o) {
            pos = lseek(fd, -1, SEEK_CUR);
            break;
        }
        if (++random == size) {
            lseek(fd, 0, SEEK_SET);
            random = -1;
            o = '\n';
        } else {
            o = c;
        }
        if (random == pos) {
            pos = -1;
            break;
        }
    }
    if (0 > r) {
        errExit("getRandomLine, read");
    }
    if ('\n' == c || '\0' == c) {
        pos = -1;
    }
    return pos;
}

int isLater(int outFD, int sleepTime, off_t size)
{
    unsigned long passedTime = 0;
    while (getFileSize(outFD) == size) {
        if ((MAX_SLEEP_TIME+sleepTime)*SAFETY_MULTIPLIER >= passedTime) {
            mSleep(sleepTime);
            passedTime += sleepTime;
        } else {
            return TRUE;
        }
    }
    return FALSE;
}