#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "deps.h"

static char plateStr[32], countStr[256], printBuf[512];

void semWaitRepeat(sem_t *sem)
{
    uint8_t repeat = 0;
    do {
        if (-1 == sem_wait(sem)) {
            if (EINTR == errno) {
                repeat = 1;
            } else {
                errExit("semWaitRepeat, sem, not EINTR");
            }
        }
    } while (repeat);
}

void printStr(const char *str)
{
    size_t len = strlen(str);
    if (-1 == write(STDOUT_FILENO, str, len)) {
        errExit("printStr, write, STDOUT_FILENO");
    }
}

void printInt(int num)
{
    char str[256];
    sprintf(str, "%d", num);
    printStr(str);
}

void printPlateCounts(struct PlateCounts *plateCounts)
{
    size_t p = plateCounts->P;
    size_t c = plateCounts->C;
    size_t d = plateCounts->D;
    sprintf(countStr, "P:%zu, C:%zu, D:%zu = %zu", p, c, d , p+c+d);
}

void printPlate(enum Plate plate)
{
    switch (plate) {
        case P:
            sprintf(plateStr, "soup");
            break;
        case C:
            sprintf(plateStr, "main course");
            break;
        case D:
            sprintf(plateStr, "desert");
            break;
        default:
            sprintf(plateStr, "NONE_PLATE");
    }
}

void errExit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

size_t getCount(struct PlateCounts *counts)
{
    size_t sum = 0;
    sum += counts->P;
    sum += counts->C;
    sum += counts->D;
    return sum;
}

void supplierMain(struct MessHall *hall)
{
    struct ReadyLeft *kitchen = &hall->kitchen;
    struct ProducerCustomer *semK = &hall->semKitchen;
    srand(time(NULL));
    enum Plate plateType = PLATE_NONE;
    char c;

    hall->fd = open(hall->args.F, O_RDONLY);
    if (-1 == hall->fd) {
        errExit("supplierMain, open, hall->fd");
    }
    
    while (getCount(&kitchen->left)) {
        semWaitRepeat(&semK->empty);
        semWaitRepeat(&semK->m);
        //! Kitchen Producer BEGIN
        plateType = PLATE_NONE;
        if (1 > read(hall->fd, &c, 1)) {
            errExit("supplierMain, read, fd");
        }
        switch (c) {
            case 'P':
                plateType = P;
                if (0 == kitchen->left.P) {
                    errExit("supplierMain, switch, P");
                }
                break;
            case 'C':
                plateType = C;
                if (0 == kitchen->left.C) {
                    errExit("supplierMain, switch, C");
                }
                break;
            case 'D':
                plateType = D;
                if (0 == kitchen->left.D) {
                    errExit("supplierMain, switch, D");
                }
                break;
            default:
                errExit("supplierMain, switch, UNKNOWN PLATE");
        }
        printSuppGoing(plateType, &kitchen->ready);
        switch (plateType) {
            case P:
                kitchen->left.P--;
                kitchen->ready.P++;
                break;
            case C:
                kitchen->left.C--;
                kitchen->ready.C++;
                break;
            case D:
                kitchen->left.D--;
                kitchen->ready.D++;
                break;
            default:
                errExit("supplierMain, switch, kitchen");
        }
        printSuppDelivered(plateType, &kitchen->ready);
        //* Kitchen Producer END
        sem_post(&semK->m);
        sem_post(&semK->full);
        sem_post(&hall->addSem);
    }
    printSuppFinished();
    if (0 > close(hall->fd)) {
        errExit("supplierMain, close, hall->fd");
    }
}

void counterPusher(struct MessHall *hall)
{
    struct ReadyLeft *counter = &hall->counter;
    struct PlateCounts *ready = &counter->ready;
    struct ProducerCustomer *semC = &hall->semCounter;
    size_t limit = 3 * hall->args.L * hall->args.M;
    while (0 < limit) {
        limit--;
        semWaitRepeat(&semC->full);
        semWaitRepeat(&semC->m);
        if (0 < ready->P && 0 < ready->C && 0 < ready->D) {
            ready->P--;
            ready->C--;
            ready->D--;
            sem_post(&semC->m);
            sem_post(&hall->chooseBarrier);
            sem_post(&semC->empty);
            sem_post(&semC->empty);
            sem_post(&semC->empty);
        }
        else {
            sem_post(&semC->m);
        }
    }
}

void cookMain(struct MessHall *hall, struct Actor *actor)
{
    struct ReadyLeft *kitchen = &hall->kitchen;
    struct ProducerCustomer *semK = &hall->semKitchen;
    struct ReadyLeft *counter = &hall->counter;
    struct ProducerCustomer *semC = &hall->semCounter;
    enum Plate plateType = PLATE_NONE;
    semWaitRepeat(&semK->m);
    while (0 < hall->counterLeftCount) {
        hall->counterLeftCount--;
        sem_post(&semK->m);
        printCookWaiting(actor->id, &kitchen->ready);
        semWaitRepeat(&semC->empty);
        semWaitRepeat(&semC->m);
        //! Counter Producer BEGIN
        semWaitRepeat(&semK->full);
        while (PLATE_NONE == plateType) {
            semWaitRepeat(&semK->m);
            //! Kitchen Consumer BEGIN
            plateType = choosePlateKitchen(hall);
            switch (plateType) {
                case P:
                    kitchen->ready.P--;
                    break;
                case C:
                    kitchen->ready.C--;
                    break;
                case D:
                    kitchen->ready.D--;
                    break;
                default:
                    break;
            }
            //* Kitchen Consumer END
            sem_post(&semK->m);
            if (PLATE_NONE == plateType) {
                semWaitRepeat(&hall->addSem);
            }
        }
        sem_post(&semK->empty);

        printCookGoing(actor->id, plateType, &counter->ready);
        
        switch (plateType) {
            case P:
                counter->left.P--;
                counter->ready.P++;
                break;
            case C:
                counter->left.C--;
                counter->ready.C++;
                break;
            case D:
                counter->left.D--;
                counter->ready.D++;
                break;
            default:
                errExit("cookMain, switch, counter");
        }
        printCookDelivered(actor->id, plateType, &counter->ready);
        plateType = PLATE_NONE;
        //* Counter Producer END
        sem_post(&semC->m);
        sem_post(&semC->full);
        semWaitRepeat(&semK->m);
    }
    sem_post(&semK->m);
    printCookFinished(actor->id);
}

void studentMain(struct MessHall *hall, struct Actor *actor)
{
    struct ReadyLeft *counter = &hall->counter;
    struct PlateCounts *cReady = &counter->ready;
    struct ProducerCustomer *semC = &hall->semCounter;
    struct ProducerCustomer *semT = &hall->semTables;
    uint8_t *tables = hall->tables;
    size_t tableId;
    for (size_t i = 1; i <= hall->args.L; ++i) {
        semWaitRepeat(&semC->m);
        hall->studentCount++;
        printStudArriving(actor->id, i, hall->studentCount, cReady);
        sem_post(&semC->m);
        semWaitRepeat(&hall->chooseBarrier);
        hall->studentCount--;

        printStudWaiting(actor->id, i, hall->tablesEmpty);

        semWaitRepeat(&semT->full);
        semWaitRepeat(&semT->m);
        //! Tables Consumer BEGIN
        for (tableId = 0; 0 != tables[tableId]; ++tableId);
        tables[tableId] = 1;
        hall->tablesEmpty--;
        printStudSitting(actor->id, i, hall->tablesEmpty, tableId);
        //* Tables Consumer END
        sem_post(&semT->m);
        sem_post(&semT->empty);

        semWaitRepeat(&semT->empty);
        semWaitRepeat(&semT->m);
        //! Tables Producer BEGIN
        hall->tablesEmpty++;
        tables[tableId] = 0;
        if (i+1 != hall->args.L) {
            printStudGoing(actor->id, i, hall->tablesEmpty, tableId);
        }
        //* Tables Producer END
        sem_post(&semT->m);
        sem_post(&semT->full);
    }
    printStudFinished(actor->id, hall->args.L);
}

enum Plate choosePlateKitchen(struct MessHall *hall)
{
    enum Plate plate = PLATE_NONE;
    struct ReadyLeft *kitchen = &hall->kitchen;
    struct ReadyLeft *counter = &hall->counter;
    size_t kP = kitchen->ready.P;
    size_t kC = kitchen->ready.C;
    size_t kD = kitchen->ready.D;
    size_t cP = counter->ready.P;
    size_t cC = counter->ready.C;
    size_t cD = counter->ready.D;
    if (0 == cP && 0 < kP) {
        plate = P;
    } else if (0 == cC && 0 < kC) {
        plate = C;
    } else if (0 == cD && 0 < kD) {
        plate = D;
    } else if (cP <= cC && cP <= cD && 0 < kP) {
        plate = P;
    } else if (cC <= cP && cC <= cD && 0 < kC) {
        plate = C;
    } else if (cD <= cP && cD <= cC && 0 < kD) {
        plate = D;
    }
    return plate;
}

void printSuppGoing(enum Plate plate, struct PlateCounts *counts)
{
    printPlate(plate);
    printPlateCounts(counts);
    sprintf(printBuf, "The supplier is going to the kitchen to deliver %s : kitchen items %s\n", plateStr, countStr);
    printStr(printBuf);
}

void printSuppDelivered(enum Plate plate, struct PlateCounts *counts)
{
    printPlate(plate);
    printPlateCounts(counts);
    sprintf(printBuf, "The supplier delivered %s - after delivery: kitchen items %s\n", plateStr, countStr);
    printStr(printBuf);
}

void printSuppFinished()
{
    printStr("The supplier finished supplying - GOODBYE\n");
}

void printCookWaiting(size_t id, struct PlateCounts *counts)
{
    printPlateCounts(counts);
    sprintf(printBuf, "Cook %zu is going to the kitchen to wait for/get a plate - kitchen items %s\n", id, countStr);
    printStr(printBuf);
}

void printCookGoing(size_t id, enum Plate plate, struct PlateCounts *counts)
{
    printPlate(plate);
    printPlateCounts(counts);
    sprintf(printBuf, "Cook %zu is going to the counter to deliver %s - counter items %s\n", id, plateStr, countStr);
    printStr(printBuf);
}

void printCookDelivered(size_t id, enum Plate plate, struct PlateCounts *counts)
{
    printPlate(plate);
    printPlateCounts(counts);
    sprintf(printBuf, "Cook %zu placed %s on the counter - counter items %s\n", id, plateStr, countStr);
    printStr(printBuf);
}

void printCookFinished(size_t id)
{
    sprintf(printBuf, "Cook %zu finished serving - items at kitchen: 0 - going home - GOODBYE\n", id);
    printStr(printBuf);
}

void printStudArriving(size_t id, size_t round, size_t students, struct PlateCounts *counts)
{
    printPlateCounts(counts);
    sprintf(printBuf, "Student %zu is going to the counter (round %zu) - # of students at counter: %zu and counter items %s\n", id, round, students, countStr);
    printStr(printBuf);
}

void printStudWaiting(size_t id, size_t round, size_t tables)
{
    sprintf(printBuf, "Student %zu got food and is going to get a table (round %zu) - # of empty tables: %zu\n", id, round, tables);
    printStr(printBuf);
}

void printStudSitting(size_t id, size_t round, size_t tables, size_t tableId)
{
    sprintf(printBuf, "Student %zu sat at the table %zu to eat (round %zu) - # of empty tables: %zu\n", id, tableId, round, tables);
    printStr(printBuf);
}

void printStudGoing(size_t id, size_t round, size_t tables, size_t tableId)
{
    sprintf(printBuf, "Student %zu left the table %zu to eat again (round %zu) - # of empty tables: %zu\n", id, tableId, round, tables);
    printStr(printBuf);
}

void printStudFinished(size_t id, size_t rounds)
{
    sprintf(printBuf, "Student %zu is done eating L=%zu times - going home - GOODBYE\n", id, rounds);
    printStr(printBuf);
}