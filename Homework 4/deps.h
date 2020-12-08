#ifndef __DEPS_H__
#define __DEPS_H__

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#define PRINTBUF_SIZE (512)
#define INGR_COUNT (5)

enum Ingredient {
    GULLAC=0,
    INGR_M=1,
    INGR_F=2,
    INGR_W=3,
    INGR_S=4
};

struct ChefSpecs {
    enum Ingredient a;
    enum Ingredient b;
    size_t id;
};

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

void printStr(const char *str);
void errExit(const char *msg);

void initSemaphores();
void destroySemaphores();
void waitSem(int sem, unsigned short a, unsigned short b);
void postSem(int sem, unsigned short n);
pthread_t createChef(struct ChefSpecs *chefSpecs);
void *chefMain(void *data);
void wholesalerMain(const char *path);
enum Ingredient charToIngredient(char chr);

void printWholesalerDelivers(enum Ingredient a, enum Ingredient b);
void printWhosalerWaiting();
void printWhosalerObtained();
void printWholesalerExit();
void printChefWaiting(size_t id, enum Ingredient a, enum Ingredient b);
void printChefTaken(size_t id, enum Ingredient a);
void printChefPreparing(size_t id);
void printChefDelivered(size_t id);
void printChefExit(size_t id);

#endif