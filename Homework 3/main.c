#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <math.h>

#include "funcs.h"
#include "processes.h"

size_t getArgs(int argc, char *argv[], char *inputA, char *inputB);

int main(int argc, char *argv[])
{
    char inputPathA[PATH_LEN], inputPathB[PATH_LEN];
    size_t longSide;
    longSide = getArgs(argc, argv, inputPathA, inputPathB);
    uint8_t **matrixA, **matrixB;
    matrixA = getMatrix(inputPathA, longSide);
    matrixB = getMatrix(inputPathB, longSide);
    struct MatrixMult mm;
    mm.fullA = matrixA;
    mm.fullB = matrixB;
    mm.qSide = longSide / 2;
    divideMatrix8(matrixA, mm.quarterA, longSide);
    divideMatrix8(matrixB, mm.quarterB, longSide);
    #ifdef __DEBUG__
        printMatrix8(matrixA, longSide);
        printMatrix8(matrixB, longSide);
    #endif
    initProcesses(&mm);
    exit(EXIT_SUCCESS);
}

size_t getArgs(int argc, char *argv[], char *inputA, char *inputB)
{
    if (7 != argc) {
        printUsage(argv[0]);
    }
    char optC;
    size_t n;
    while (-1 != (optC = getopt(argc, argv, ":i:j:n:"))) {
        switch (optC) {
        case 'i':
            strcpy(inputA, optarg);
            break;
        case 'j':
            strcpy(inputB, optarg);
            break;
        case 'n':
            n = (size_t)strtoll(optarg, NULL, 10);
            break;
        case ':':
        default:
            printUsage(argv[0]);
            break;
        }
    }
    return (size_t)powl(2, n);
}