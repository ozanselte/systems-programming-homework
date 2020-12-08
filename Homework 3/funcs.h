#ifndef __FUNCS_H__
#define __FUNCS_H__

//#define __DEBUG__
#define PATH_LEN (256)
#define PIECE_SIZE (UINT8_MAX)

#define _MIN(a,b) (((a) < (b)) ? (a) : (b))

#include <stdint.h>

void printMatrix8(uint8_t **M, size_t side);
void printMatrix64(uint64_t **M, size_t side);
void printUsage();
void printStr(const char *msg);
void errExit(const char *msg);

char *createStr(const char *str);
void *xcalloc(size_t count, size_t size);
void xfree(void *ptr);
void freeMatrix8(uint8_t **M, size_t side);
void freeMatrix64(uint64_t **M, size_t side);
uint8_t **createMatrix8(size_t side);
uint64_t **createMatrix64(size_t side);

uint8_t **getMatrix(const char *path, size_t side);
void readMatrix(int fd, uint8_t **M, size_t side);
void divideMatrix8(uint8_t **M, uint8_t **div[4], size_t longSide);
void divideMatrix64(uint64_t **M, uint64_t **div[4], size_t longSide);

#endif