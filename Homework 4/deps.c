#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/stat.h>

#include "deps.h"

extern uint8_t *ingreds;
extern int sems;
static char *ingName[8] = {"dessert", "milk", "flour", "walnuts", "sugar"};

void printStr(const char *str)
{
    size_t len = strlen(str);
    if (-1 == write(STDOUT_FILENO, str, len)) {
        errExit("printStr, write, STDOUT_FILENO");
    }
}

void errExit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void initSemaphores()
{
    sems = semget(IPC_PRIVATE, INGR_COUNT, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (-1 == sems) {
        errExit("initSemaphores, semget, sems");
    }
    union semun arg;
    arg.val = 0;
    for (uint8_t i = 1; i < INGR_COUNT; ++i) {
        if (-1 == semctl(sems, i, SETVAL, arg)) {
            errExit("initSemaphores, semctl, SETALL");
        }
    }
    arg.val = 0;
    if (-1 == semctl(sems, GULLAC, SETVAL, arg)) {
        errExit("initSemaphores, semctl, SETVAL");
    }
}

void destroySemaphores()
{
    if (-1 == semctl(sems, 0, IPC_RMID)) {
        errExit("destroySemaphores, semctl, sems");
    }
}

void waitSem(int sem, unsigned short a, unsigned short b)
{
    struct sembuf sop[2];
    sop[0].sem_num = a;
    sop[0].sem_op = -1;
    sop[0].sem_flg = 0;
    sop[1].sem_num = b;
    sop[1].sem_op = -1;
    sop[1].sem_flg = 0;
    size_t n = a == b ? 1 : 2;
    uint8_t repeat = 0;
    do {
        if (-1 == semop(sem, sop, n)) {
            if (EINTR == errno) {
                repeat = 1;
            } else {
                errExit("waitSem, sem, not EINTR");
            }
        }
    } while (repeat);
}

void postSem(int sem, unsigned short n)
{
    struct sembuf sop;
    sop.sem_num = n;
    sop.sem_op = 1;
    sop.sem_flg = 0;
    uint8_t repeat = 0;
    do {
        if (-1 == semop(sem, &sop, 1)) {
            if (EINTR == errno) {
                repeat = 1;
            } else {
                errExit("postSem, sem, not EINTR");
            }
        }
    } while (repeat);
}

pthread_t createChef(struct ChefSpecs *chefSpecs)
{
    pthread_t thread;
    if (0 != pthread_create(&thread, NULL, chefMain, chefSpecs)) {
        errExit("createChef, pthread_create");
    }
    return thread;
}

void *chefMain(void *data)
{
    struct ChefSpecs *chefSpecs = (struct ChefSpecs *)data;
    enum Ingredient a = chefSpecs->a;
    enum Ingredient b = chefSpecs->b;
    size_t id = chefSpecs->id;
    uint8_t isExit = 0;
    printChefWaiting(id, a, b);
    for (;;) {
        waitSem(sems, a, b);
        if (ingreds[a] && ingreds[b]) {
            ingreds[a]--;
            printChefTaken(id, a);
            ingreds[b]--;
            printChefTaken(id, b);
            printChefPreparing(id);
            sleep((rand() % 5) + 1);
            ingreds[GULLAC]++;
            printChefDelivered(id);
            postSem(sems, GULLAC);
            printChefWaiting(id, a, b);
        } else {
            isExit = 1;
            for (uint8_t i = 0; i < INGR_COUNT; ++i) {
                if (ingreds[i]) {
                    isExit = 0;
                }
            }
            if (isExit) {
                printChefExit(id);
                postSem(sems, a);
                postSem(sems, b);
                break;
            }
        }
    }
    return NULL;
}

void wholesalerMain(const char *path)
{
    enum Ingredient a=GULLAC, b=GULLAC;
    int fd = open(path, O_RDONLY);
    if (-1 == fd) {
        errExit("wholesalerMain, open, fd");
    }
    char buf[3];
    while (3 == read(fd, buf, 3)) {
        memset(ingreds, '\0', INGR_COUNT*sizeof(uint8_t));
        a = charToIngredient(buf[0]);
        b = charToIngredient(buf[1]);
        printWholesalerDelivers(a, b);
        ingreds[a]++;
        ingreds[b]++;
        postSem(sems, a);
        postSem(sems, b);
        printWhosalerWaiting();
        waitSem(sems, GULLAC, GULLAC);
        ingreds[GULLAC]--;
        if (ingreds[GULLAC]) {
            printWhosalerObtained();
        }
    }
    printWholesalerExit();
    memset(ingreds, '\0', INGR_COUNT*sizeof(uint8_t));
    for (size_t i = 0; i < INGR_COUNT; ++i) {
        postSem(sems, i);
    }
}

enum Ingredient charToIngredient(char chr)
{
    enum Ingredient ing;
    switch (chr) {
        case 'M': ing = INGR_M; break;
        case 'F': ing = INGR_F; break;
        case 'W': ing = INGR_W; break;
        case 'S': ing = INGR_S; break;
        default:
            errExit("charToIngredient");
    }
    return ing;
}

void printWholesalerDelivers(enum Ingredient a, enum Ingredient b)
{
    char printBuf[PRINTBUF_SIZE];
    sprintf(printBuf, "the wholesaler delivers %s and %s\n", ingName[a], ingName[b]);
    printStr(printBuf);
}

void printWhosalerWaiting()
{
    printStr("the wholesaler is waiting for the dessert\n");
}

void printWhosalerObtained()
{
    printStr("the wholesaler has obtained the dessert and left to sell it\n");
}

void printWholesalerExit()
{
    printStr("the wholesaler is sending recursive exit signal\n");
}

void printChefWaiting(size_t id, enum Ingredient a, enum Ingredient b)
{
    char printBuf[PRINTBUF_SIZE];
    sprintf(printBuf, "chef%zu is waiting for %s and %s\n", id ,ingName[a], ingName[b]);
    printStr(printBuf);
}

void printChefTaken(size_t id, enum Ingredient a)
{
    char printBuf[PRINTBUF_SIZE];
    sprintf(printBuf, "chef%zu has taken the %s\n", id, ingName[a]);
    printStr(printBuf);
}

void printChefPreparing(size_t id)
{
    char printBuf[PRINTBUF_SIZE];
    sprintf(printBuf, "chef%zu is preparing the dessert\n", id);
    printStr(printBuf);
}

void printChefDelivered(size_t id)
{
    char printBuf[PRINTBUF_SIZE];
    sprintf(printBuf, "chef%zu has delivered the dessert to the wholesaler\n", id);
    printStr(printBuf);
}

void printChefExit(size_t id)
{
    char printBuf[PRINTBUF_SIZE];
    sprintf(printBuf, "chef%zu got the exit signal\n", id);
    printStr(printBuf);
}