#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "funcs.h"

void printMatrix8(uint8_t **M, size_t side)
{
    char str[128];
    printStr("\n--- BEGIN MATRIX ---\n");
    for (size_t i = 0; i < side; ++i) {
        for (size_t j = 0; j < side; ++j) {
            sprintf(str, "%4u ", M[i][j]);
            printStr(str);
        }
        printStr("\n");
    }
    printStr("---  END  MATRIX ---\n\n");
}

void printMatrix64(uint64_t **M, size_t side)
{
    char str[128];
    printStr("\n--- BEGIN MATRIX ---\n");
    for (size_t i = 0; i < side; ++i) {
        for (size_t j = 0; j < side; ++j) {
            sprintf(str, "%8lu ", M[i][j]);
            printStr(str);
        }
        printStr("\n");
    }
    printStr("---  END  MATRIX ---\n\n");
}

void printUsage(const char *name)
{
    printStr("ProgramA Usage:\n\t");
    printStr(name);
    printStr(" -i INPUT -o OUTPUT -n N\n");
    printStr("All arguments are mandatory.\n");
    printStr("\t-i\tAbsolute path of the input file\n");
    printStr("\t-o\tAbsolute path of the output file\n");
    printStr("\t-n\tPositive integer\n");
    exit(EXIT_FAILURE);
}

void printStr(const char *str)
{
    size_t len = 1 + strlen(str);
    if (len != write(STDOUT_FILENO, str, len)) {
        errExit("printStr, write, STDOUT_FILENO");
    }
}

void errExit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

char *createStr(const char *str)
{
    size_t len = 1 + strlen(str);
    char *ptr = (char *)xcalloc(len, sizeof(char));
    strcpy(ptr, str);
    return ptr;
}

void *xcalloc(size_t count, size_t size)
{
    void *ptr = calloc(count, size);
    if (NULL == ptr) {
        errExit("xcalloc, NULL");
    }
    return ptr;
}

void xfree(void *ptr)
{
    if (NULL != ptr) {
        free(ptr);
    }
}

void freeMatrix8(uint8_t **M, size_t side)
{
    for (size_t i = 0; i < side; ++i) {
        free(M[i]);
    }
    free(M);
}

void freeMatrix64(uint64_t **M, size_t side)
{
    for (size_t i = 0; i < side; ++i) {
        free(M[i]);
    }
    free(M);
}

uint8_t **createMatrix8(size_t side)
{
    uint8_t **M = (uint8_t **)xcalloc(side, sizeof(uint8_t *));
    for (size_t i = 0; i < side; ++i) {
        M[i] = (uint8_t *)xcalloc(side, sizeof(uint8_t));
    }
    return M;
}

uint64_t **createMatrix64(size_t side)
{
    uint64_t **M = (uint64_t **)xcalloc(side, sizeof(uint64_t *));
    for (size_t i = 0; i < side; ++i) {
        M[i] = (uint64_t *)xcalloc(side, sizeof(uint64_t));
    }
    return M;
}

uint8_t **getMatrix(const char *path, size_t side)
{
    uint8_t **M = createMatrix8(side);
    int fd = open(path, O_RDONLY);
    if (-1 == fd) {
        freeMatrix8(M, side);
        errExit("getMatrix, open, path");
    }
    readMatrix(fd, M, side);
    if (-1 == close(fd)) {
        freeMatrix8(M, side);
        errExit("getMatrix, close, fd");
    }
    return M;
}

void readMatrix(int fd, uint8_t **M, size_t side)
{
    for (size_t i = 0; i < side; ++i) {
        ssize_t readed, piece = PIECE_SIZE;
        uint8_t *ptr = M[i];
        for (size_t left = side; 0 < left; left -= readed) {
            piece = _MIN(left, piece);
            readed = read(fd, ptr, piece);
            ptr += readed;
            if (-1 == readed) {
                freeMatrix8(M, side);
                errExit("readMatrix, read, fd");
            }
            else if (piece > readed) {
                freeMatrix8(M, side);
                printStr("File is smaller than expected!\n");
                exit(EXIT_FAILURE);
            }
            else if (piece < readed) {
                freeMatrix8(M, side);
                printStr("Unexpected error! See readMatrix in funcs.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void divideMatrix8(uint8_t **M, uint8_t **div[4], size_t longSide)
{
    size_t side = longSide / 2;
    for (uint8_t i = 0; i < 4; ++i) {
        div[i] = (uint8_t **)xcalloc(side, sizeof(uint8_t *));
    }
    for (size_t i = 0; i < side; ++i) {
        div[0][i] = M[i];
        div[1][i] = &(M[i][side]);
        div[2][i] = M[i+side];
        div[3][i] = &(M[i+side][side]);
    }
}

void divideMatrix64(uint64_t **M, uint64_t **div[4], size_t longSide)
{
    size_t side = longSide / 2;
    for (uint8_t i = 0; i < 4; ++i) {
        div[i] = (uint64_t **)xcalloc(side, sizeof(uint64_t *));
    }
    for (size_t i = 0; i < side; ++i) {
        div[0][i] = M[i];
        div[1][i] = &(M[i][side]);
        div[2][i] = M[i+side];
        div[3][i] = &(M[i+side][side]);
    }
}