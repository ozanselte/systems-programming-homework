#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "b_funcs.h"
#include "x_funcs.h"

int main(int argc, char *argv[])
{
    char *inputPath, *outputPath;
    int sleepTime;
    char optC;
    if (7 != argc) {
        printUsage(argv[0]);
    }
    while (0 < (optC = getopt(argc, argv, ":i:o:t:"))) {
        switch (optC) {
        case 'i':
            inputPath = (char *)calloc(strlen(optarg)+1, sizeof(char));
            strcpy(inputPath, optarg);
            break;
        case 'o':
            outputPath = (char *)calloc(strlen(optarg)+1, sizeof(char));
            strcpy(outputPath, optarg);
            break;
        case 't':
            sleepTime = atoi(optarg);
            if (MIN_SLEEP_TIME > sleepTime || MAX_SLEEP_TIME < sleepTime) {
                printUsage(argv[0]);
            }
            break;
        case ':':
        default:
            printUsage(argv[0]);
            break;
        }
    }
    mainFuncB(inputPath, outputPath, sleepTime);
    free(inputPath);
    free(outputPath);
    return EXIT_SUCCESS;
}