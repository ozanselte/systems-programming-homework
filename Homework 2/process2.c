#define _POSIX_C_SOURCE 200809
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <math.h>

#include "funcs.h"
#include "process2.h"

static volatile sig_atomic_t isInputFinal = 0;
static pid_t child = 0;
static char *binPath, *tempPath, *outPath;
static int inFD, outFD;

void mainP2(char *inputPath, char *outputPath, char *firstPath, pid_t pid)
{
    child = pid;
    binPath = firstPath;
    tempPath = inputPath;
    outPath = outputPath;
    linkHandlers();
    inFD = open(inputPath, O_RDWR | O_CREAT | O_SYNC, 0644);
    if (0 > inFD) {
        errExit("mainP2, open, inFD");
    }
    outFD = open(outputPath, O_RDWR | O_CREAT | O_APPEND | O_SYNC, 0644);
    if (0 > outFD) {
        errExit("mainP2, open, outFD");
    }
    struct MetricsVector vector;
    vector.size = 0;
    vector.capacity = VECTOR_START_CAP;
    vector.arr = (struct Metrics *)calloc(VECTOR_START_CAP, sizeof(struct Metrics));
    if (NULL == vector.arr) {
        errExit("mainP2, calloc, vector.arr");
    }
    do {
        signalBlock();
        fileOpsP2(&vector);
        waitSignal();
    } while (!isInputFinal);
    signalUnblock();
    printCalculations(&vector);
    free(vector.arr);
    if (0 > close(inFD)) {
        errExit("mainP2, close, inFD");
    }
    if (0 > close(outFD)) {
        errExit("mainP2, close, outFD");
    }
    if (0 > unlink(tempPath)) {
        errExit("mainP2, unlink, tempPath");
    }
}

void fileOpsP2(struct MetricsVector *vector)
{
    char *inCursor, inBuf[TEMP_BUF_SIZE];
    char *outCursor, outBuf[TEMP_BUF_SIZE];
    uintmax_t N = INPUT_BUF_SIZE / 2;
    double mae, mse, preds[N];
    uint8_t M[N][2];
    double lsA, lsB, rmse;
    while (MIN_TEMP_SIZE < getFileSize(inFD)) {
        if (0 > getLine(inFD, inBuf)) {
            errExit("fileOpsP2, getLine, inFD");
        }
        wLockFile(inFD);
        removeFirstLine(inFD, getFileSize(inFD), strlen(inBuf)+1);
        wUnlockFile(inFD);
        memset(outBuf, '\0', TEMP_BUF_SIZE);
        inCursor = inBuf;
        outCursor = outBuf;
        outCursor += sprintf(outCursor, "%s,", inBuf);
        getLineNums(N, inCursor, M, &lsA, &lsB);
        makePredictions(N, M, lsA, lsB, preds);
        mae = calculateMAE(N, M, preds);
        mse = calculateMSE(N, M, preds);
        rmse = sqrt(mse);
        addVector(vector, mae, mse, rmse);
        outCursor += sprintf(outCursor, "%.3f,%.3f,%.3f\n", mae, mse, rmse);
        size_t len = outCursor - outBuf;
        if (len > write(outFD, outBuf, len)) {
            errExit("fileOpsP2, write, outFD");
        }
        lseek(inFD, 0, SEEK_SET);
    }
}

void sigHandlerP2(int sig)
{
    int errnoBackup = errno;
    switch (sig) {
        case SIGCONT:
            break;
        case SIGUSR1:
            isInputFinal = 1;
            if (0 > unlink(binPath)) {
                errExit("sigHandlerP2, unlink, binPath");
            }
            break;
        case SIGUSR2:
            termHandler();
            break;
        case SIGTERM:
            if (0 > kill(child, SIGUSR2)) {
                errExit("sigHandlerP2, kill, child");
            }
            termHandler();
            break;
        default:
            break;
    }
    errno = errnoBackup;
}

void termHandler()
{
    close(inFD);
    close(outFD);
    unlink(binPath);
    free(binPath);
    free(tempPath);
    free(outPath);
    if (0 > unlink(tempPath)) {
        errExit("sigHandlerP2, unlink, tempPath");
    }
    _exit(EXIT_SUCCESS);
}

void linkHandlers()
{
    struct sigaction sa;
    sa.sa_handler = sigHandlerP2;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    int sigs[] = {SIGCONT, SIGUSR1, SIGUSR2, SIGTERM};
    for (int i = 0; i < 4; ++i) {
        if (0 > sigaction(sigs[i], &sa, NULL)) {
            errExit("mainP2, sigaction, sa");
        }
    }
}

void getLineNums(uintmax_t N, char *cursor, uint8_t M[][2], double *lsA, double *lsB)
{
    uintmax_t comma = 0;
    for (uintmax_t i = 0; i < N; ++i) {
        comma = 0;
        sscanf(cursor, "(%hhu,%hhu),", &(M[i][0]), &(M[i][1]));
        while (2 != comma) {
            if (',' == *cursor++) {
                ++comma;
            }
        }
    }
    sscanf(cursor, "%lfx%lf", lsA, lsB);
}

void makePredictions(uintmax_t N, uint8_t M[][2], double lsA, double lsB, double preds[])
{
    for (uintmax_t i = 0; i < N; ++i) {
        preds[i] = (lsA * M[i][0]) + lsB;
    }
}

double calculateMAE(uintmax_t N, uint8_t M[][2], double preds[])
{
    double sumOfErrors = 0;
    double error;
    for (uintmax_t i = 0; i < N; ++i) {
        error = preds[i] - M[i][1];
        error = fabs(error);
        sumOfErrors += error;
    }
    return sumOfErrors / N;
}

double calculateMSE(uintmax_t N, uint8_t M[][2], double preds[])
{
    double sumOfErrors = 0;
    double error;
    for (uintmax_t i = 0; i < N; ++i) {
        error = preds[i] - M[i][1];
        sumOfErrors += error * error;
    }
    return sumOfErrors / N;
}

struct MetricsVector *addVector(struct MetricsVector *vector, double mae, double mse, double rmse)
{
    if (vector->size == vector->capacity) {
        vector->capacity *= 2;
        vector->arr = (struct Metrics *)realloc(vector->arr, vector->capacity * sizeof(struct Metrics));
        if (NULL == vector->arr) {
            errExit("addVector, realloc, vector->arr");
        }
    }
    vector->arr[vector->size].mae = mae;
    vector->arr[vector->size].mse = mse;
    vector->arr[vector->size].rmse = rmse;
    vector->size += 1;
    return vector;
}

void printCalculations(struct MetricsVector *vector)
{
    double stdMAE = 0.0, stdMSE = 0.0, stdRMSE = 0.0;
    double mae, mse, rmse;
    double meanMAE = getMeanMAE(vector);
    double meanMSE = getMeanMSE(vector);
    double meanRMSE = getMeanRMSE(vector);
    for (uintmax_t i = 0; i < vector->size; ++i) {
        mae = vector->arr[i].mae;
        mae -= meanMAE;
        stdMAE += mae * mae;
        mse = vector->arr[i].mse;
        mse -= meanMSE;
        stdMSE += mse * mse;
        rmse = vector->arr[i].rmse;
        rmse -= meanRMSE;
        stdRMSE += rmse * rmse;
    }
    stdMAE /= vector->size - 1;
    stdMAE = sqrt(stdMAE);
    stdMSE /= vector->size - 1;
    stdMSE = sqrt(stdMSE);
    stdRMSE /= vector->size - 1;
    stdRMSE = sqrt(stdRMSE);
    printf("         MAE       MSE        RMSE\n");
    printf("MEAN %8.3f   %8.3f   %8.3f\n", meanMAE, meanMSE, meanRMSE);
    printf("STD  %8.3f   %8.3f   %8.3f\n", stdMAE, stdMSE, stdRMSE);
    printf("Size: %lu\n", vector->size);
}

double getMeanMAE(struct MetricsVector *vector)
{
    double mean = 0.0;
    for (uintmax_t i = 0; i < vector->size; ++i) {
        mean += vector->arr[i].mae;
    }
    mean /= vector->size;
    return mean;
}

double getMeanMSE(struct MetricsVector *vector)
{
    double mean = 0.0;
    for (uintmax_t i = 0; i < vector->size; ++i) {
        mean += vector->arr[i].mse;
    }
    mean /= vector->size;
    return mean;
}

double getMeanRMSE(struct MetricsVector *vector)
{
    double mean = 0.0;
    for (uintmax_t i = 0; i < vector->size; ++i) {
        mean += vector->arr[i].rmse;
    }
    mean /= vector->size;
    return mean;
}