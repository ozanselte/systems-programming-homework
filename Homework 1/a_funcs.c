#define _POSIX_C_SOURCE 200809
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "x_funcs.h"
#include "a_funcs.h"

void printUsage(const char *name)
{
    char *s0 = "ProgramA Usage:\n\t";
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

void mainFuncA(const char *inPath, const char *outPath, int sleepTime)
{
    int inFD = open(inPath, O_RDONLY);
    if (0 > inFD) {
        errExit("mainFuncA, inFD, open");
    }
    int outFD = open(outPath, O_RDWR | O_CREAT | O_SYNC, 0644);
    if (0 > outFD) {
        errExit("mainFuncA, outFD, open");
    }
    programA(inFD, outFD, sleepTime);
    /*if (0 > close(outFD)) {
        errExit("mainFuncA, outFD, close");
    }*/
    if (0 > close(inFD)) {
        errExit("mainFuncA, inFD, close");
    }
}

void programA(int inFD, int outFD, int sleepTime)
{
    int rInput, count;
    u_int8_t inBuf[INPUT_BUF_SIZE];
    char outBuf[OUTPUT_BUF_SIZE];
    char *outCursor;
    while (MIN_INPUT_BYTES <= (rInput = read(inFD, inBuf, INPUT_BUF_SIZE))) {
        wLockFile(outFD, sleepTime);
        memset(outBuf, '\0', OUTPUT_BUF_SIZE);
        outCursor = outBuf;
        for (int i = 0; i < INPUT_BUF_SIZE; i += 2) {
            if (0 != i) {
                count = sprintf(outCursor, ",");
                outCursor += count;
            }
            int real = inBuf[i];
            int imag = inBuf[i+1];
            count = sprintf(outCursor, "%d+i%d", real, imag);
            outCursor += count;
        }
        count = sprintf(outCursor, "\n");
        outCursor += count;
        int emptyFD;
        int beginning = getEmptyLineCursor(&emptyFD, outFD);
        int len = outCursor-outBuf;
        int size = getFileSize(outFD);
        if (0 < size) {
            if (size+1 != beginning) {
                expandFile(emptyFD, beginning-1, size, len-1);
            } else {
                expandFile(emptyFD, beginning, size, len);
            }
        }
        if (len > write(emptyFD, outBuf, len)) {
            errExit("programA, outFD, write");
        }/*
        if (0 > close(emptyFD)) {
            errExit("programA, emptyFD, close");
        }*/
        wUnlockFile(outFD, sleepTime);
        mSleep(sleepTime);
    }
    if (0 > rInput) {
        errExit("programA, inFD, read");
    }
}