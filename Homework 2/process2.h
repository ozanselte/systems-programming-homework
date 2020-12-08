#ifndef __PROCESS2_H__
#define __PROCESS2_H__

#include <stdint.h>

#define VECTOR_START_CAP (8)

struct Metrics {
    double mae;
    double mse;
    double rmse;
};

struct MetricsVector {
    ssize_t size;
    ssize_t capacity;
    struct Metrics *arr;
};

void mainP2(char *inputPath, char *outputPath, char *firstPath, pid_t child);
void fileOpsP2(struct MetricsVector *vector);
void sigHandlerP2(int sig);
void termHandler();
void linkHandlers();
void getLineNums(uintmax_t N, char *cursor, uint8_t M[][2], double *lsA, double *lsB);
void makePredictions(uintmax_t N, uint8_t M[][2], double lsA, double lsB, double preds[]);
double calculateMAE(uintmax_t N, uint8_t M[][2], double preds[]);
double calculateMSE(uintmax_t N, uint8_t M[][2], double preds[]);
struct MetricsVector *addVector(struct MetricsVector *vector, double mae, double mse, double rmse);
void printCalculations(struct MetricsVector *vector);
double getMeanMAE(struct MetricsVector *vector);
double getMeanMSE(struct MetricsVector *vector);
double getMeanRMSE(struct MetricsVector *vector);

#endif