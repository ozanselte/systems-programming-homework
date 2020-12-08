#ifndef __PROCESS1_H__
#define __PROCESS1_H__

void mainP1(const char *inputPath, const char *outputPath);
void fileOpsP1();
void sigHandlerP1(int sig);
double leastSquaresA(double x, double y, double xx, double xy);
double leastSquaresB(double m, double x, double y);

#endif