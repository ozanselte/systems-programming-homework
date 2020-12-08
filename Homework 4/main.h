#ifndef __MAIN_H__
#define __MAIN_H__

#include "deps.h"

#define INPUT_FILE_LEN (512)

int main(int argc, char *argv[]);
void printUsage(const char *name);
void joinThreads(pthread_t *chefs, size_t N);

#endif