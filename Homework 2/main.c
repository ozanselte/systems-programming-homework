#define _POSIX_C_SOURCE 200809
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include "funcs.h"
#include "process1.h"
#include "process2.h"

int main(int argc, char *argv[])
{
    char *inputPath, *outputPath, *tempPath;
    char optC;
    if (5 != argc) {
        printUsage(argv[0]);
    }
    while (0 < (optC = getopt(argc, argv, ":i:o:"))) {
        switch (optC) {
        case 'i':
            inputPath = createStr(optarg);
            break;
        case 'o':
            outputPath = createStr(optarg);
            break;
        case ':':
        default:
            printUsage(argv[0]);
            break;
        }
    }
    tempPath = createStr(TEMP_TEMPLATE);
    int tmp = mkstemp(tempPath);
    close(tmp);
    pid_t pid = fork();
    if (0 == pid) {
        mainP1(inputPath, tempPath);
    } else {
        mainP2(tempPath, outputPath, inputPath, pid);
    }
    free(inputPath);
    free(outputPath);
    free(tempPath);
    return EXIT_SUCCESS;
}