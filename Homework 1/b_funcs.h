#ifndef __B_FUNCS_H__
#define __B_FUNCS_H__

#include <complex.h>

#define INPUT_BUF_SIZE (512)
#define INPUT_NUMS_COUNT (16)
#define OUTPUT_BUF_SIZE (1024)

void printUsage();
void mainFuncB(const char *inPath, const char *outPath, int sleepTime);
off_t programB(int inFD, int outFD, int sleepTime);
void processInputFile(int inFD, int sleepTime, int line, double complex *nums);
void processOutputFile(int outFD, int sleepTime, double complex *nums);
off_t getRandomLine(int fd);
int isLater(int outFD, int sleepTime, off_t size);

#endif