#ifndef __DEPS_H__
#define __DEPS_H__

#include <stdlib.h>
#include <stdint.h>
#include <semaphore.h>

enum ActorType {
    ACTOR_NONE=0,
    SUPP,
    COOK,
    STUD
};

enum Plate {
    PLATE_NONE=0,
    P,
    C,
    D
};

struct PlateCounts {
    size_t P;
    size_t C;
    size_t D;
};

struct Args {
    size_t N;
    size_t M;
    size_t T;
    size_t S;
    size_t L;
    size_t K;
    char F[512];
};

struct ProducerCustomer {
    sem_t empty;
    sem_t full;
    sem_t m;
};

struct ReadyLeft {
    struct PlateCounts ready;
    struct PlateCounts left;
};

struct MessHall {
    struct Args args;

    struct ReadyLeft kitchen;
    struct ReadyLeft counter;
    size_t counterLeftCount;
    size_t tablesEmpty;
    size_t studentCount;
    uint8_t *tables;

    struct ProducerCustomer semKitchen;
    struct ProducerCustomer semCounter;
    struct ProducerCustomer semTables;
    sem_t chooseBarrier, addSem;
    int fd;
};

struct Actor {
    size_t id;
    enum ActorType type;
};

void semWaitRepeat(sem_t *sem);
void printStr(const char *str);
void printInt(int num);
void printPlateCounts(struct PlateCounts *plateCounts);
void printPlate(enum Plate plate);
void errExit(const char *msg);
char *createStr(const char *str);
void *xcalloc(size_t count, size_t size);
void xfree(void *ptr);
size_t getCount(struct PlateCounts *counts);
void supplierMain(struct MessHall *hall);
void counterPusher(struct MessHall *hall);
void cookMain(struct MessHall *hall, struct Actor *actor);
void studentMain(struct MessHall *hall, struct Actor *actor);
enum Plate choosePlateKitchen(struct MessHall *hall);
void printSuppGoing(enum Plate plate, struct PlateCounts *counts);
void printSuppDelivered(enum Plate plate, struct PlateCounts *counts);
void printSuppFinished();
void printCookWaiting(size_t id, struct PlateCounts *counts);
void printCookGoing(size_t id, enum Plate plate, struct PlateCounts *counts);
void printCookDelivered(size_t id, enum Plate plate, struct PlateCounts *counts);
void printCookFinished(size_t id);
void printStudArriving(size_t id, size_t round, size_t students, struct PlateCounts *counts);
void printStudWaiting(size_t id, size_t round, size_t tables);
void printStudSitting(size_t id, size_t round, size_t tables, size_t tableId);
void printStudGoing(size_t id, size_t round, size_t tables, size_t tableId);
void printStudFinished(size_t id, size_t rounds);

#endif