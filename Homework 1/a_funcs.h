#ifndef __A_FUNCS_H__
#define __A_FUNCS_H__

#define INPUT_BUF_SIZE (32)
#define MIN_INPUT_BYTES (32)
#define OUTPUT_BUF_SIZE (256)

void printUsage();
void mainFuncA(const char *inPath, const char *outPath, int sleepTime);
void programA(int inFD, int outFD, int sleepTime);

#endif