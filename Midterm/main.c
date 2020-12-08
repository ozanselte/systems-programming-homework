#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#include "main.h"
#include "deps.h"

static struct MessHall *hall;
static struct Actor actor;

int main(int argc, char *argv[])
{
    if (13 != argc) {
        printUsage(argv[0]);
    }
    hall = (struct MessHall *)createSharedMem(sizeof(*hall));
    struct Args *args = &hall->args;
    char optC;
    while (-1 != (optC = getopt(argc, argv, ":N:M:T:S:L:F:"))) {
        switch (optC) {
        case 'N':
            args->N = (size_t)strtoll(optarg, NULL, 10);
            break;
        case 'M':
            args->M = (size_t)strtoll(optarg, NULL, 10);
            break;
        case 'T':
            args->T = (size_t)strtoll(optarg, NULL, 10);
            break;
        case 'S':
            args->S = (size_t)strtoll(optarg, NULL, 10);
            break;
        case 'L':
            args->L = (size_t)strtoll(optarg, NULL, 10);
            break;
        case 'F':
            strcpy(args->F, optarg);
            break;
        case ':':
        default:
            printUsage(argv[0]);
            break;
        }
    }
    if ((2 >= args->N) || (args->M <= args->N) ||
        (3 >= args->S) || (1 > args->T) ||
        (3 > args->L) || (args->M <= args->T)) {
            printUsage(argv[0]);
    }
    args->K = 2 * args->L * args->M + 1;
    initHall(hall);
    hall->tables = (uint8_t *)createSharedMem(args->T * sizeof(uint8_t));
    for (size_t i = 0; i < args->T; ++i) {
        hall->tables[i] = 0;
    }
    initHallSemaphores(hall);
    forkCounterPusher(hall);
    forkCooks(hall);
    forkStudents(hall);

    actor.id = 0;
    actor.type = SUPP;
    linkSignalHandler();
    supplierMain(hall);
    waitChilds();
    exitProcess(EXIT_SUCCESS);
}

void printUsage(const char *name)
{
    printStr("Program Usage:\n\t");
    printStr(name);
    printStr(" -N <NUM> -M <NUM> -T <NUM> -S <NUM> -L <NUM> -F <STRING>\n");
    printStr("All arguments are mandatory.\n");
    printStr("\t-N\t>2\tCount of the cooks\n");
    printStr("\t-M\t>N & >T\tCount of the students\n");
    printStr("\t-T\t>=1\tCount of the tables\n");
    printStr("\t-S\t>3\tSize of the counter\n");
    printStr("\t-L\t>=3\tRight of every student to eat\n");
    printStr("\t-F\t   \tInput file path for supplier\n");
    exit(EXIT_FAILURE);
}

void initHall(struct MessHall *hall)
{
    size_t leftPlateCount = hall->args.L * hall->args.M;

    hall->kitchen.ready.P = 0;
    hall->kitchen.ready.C = 0;
    hall->kitchen.ready.D = 0;
    hall->kitchen.left.P = leftPlateCount;
    hall->kitchen.left.C = leftPlateCount;
    hall->kitchen.left.D = leftPlateCount;

    hall->counter.ready.P = 0;
    hall->counter.ready.C = 0;
    hall->counter.ready.D = 0;
    hall->counter.left.P = leftPlateCount;
    hall->counter.left.C = leftPlateCount;
    hall->counter.left.D = leftPlateCount;

    hall->counterLeftCount = 3 * leftPlateCount;
    hall->tablesEmpty = hall->args.T;
    hall->studentCount = 0;
}

void *createSharedMem(size_t size)
{
    char tempName[] = "tempshm_XXXXXX";
    int tfd = mkstemp(tempName);
    if (-1 == tfd) {
        errExit("initSharedMem, mkstemp");
    }
    if (-1 == close(tfd)) {
        errExit("initSharedMem, close, tfd");
    }
    if (-1 == unlink(tempName)) {
        errExit("initSharedMem, unlink, tempName");
    }
    int fd = shm_open(tempName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (-1 == fd) {
        errExit("initSharedMem, shm_open");
    }
    if (-1 == ftruncate(fd, size)) {
        errExit("initSharedMem, ftruncate");
    }
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == addr) {
        errExit("initSharedMem, mmap");
    }
    if (-1 == close(fd)) {
        errExit("initSharedMem, close, fd");
    }
    if (-1 == shm_unlink(tempName)) {
        errExit("initSharedMem, shm_unlink");
    }
    memset(addr, '\0', size);
    return addr;
}

void initHallSemaphores(struct MessHall *hall)
{
    struct Args *args = &hall->args;

    if (-1 == sem_init(&hall->semKitchen.empty, 1, args->K)) {
        errExit("initHallSemaphores, sem_init, &hall->semKitchen.empty");
    }
    if (-1 == sem_init(&hall->semKitchen.full, 1, 0)) {
        errExit("initHallSemaphores, sem_init, &hall->semKitchen.full");
    }
    if (-1 == sem_init(&hall->semKitchen.m, 1, 1)) {
        errExit("initHallSemaphores, sem_init, &hall->semKitchen.m");
    }

    if (-1 == sem_init(&hall->semCounter.empty, 1, args->S)) {
        errExit("initHallSemaphores, sem_init, &hall->semCounter.empty");
    }
    if (-1 == sem_init(&hall->semCounter.full, 1, 0)) {
        errExit("initHallSemaphores, sem_init, &hall->semCounter.full");
    }
    if (-1 == sem_init(&hall->semCounter.m, 1, 1)) {
        errExit("initHallSemaphores, sem_init, &hall->semCounter.m");
    }

    if (-1 == sem_init(&hall->semTables.empty, 1, 0)) {
        errExit("initHallSemaphores, sem_init, &hall->semTables.empty");
    }
    if (-1 == sem_init(&hall->semTables.full, 1, args->T)) {
        errExit("initHallSemaphores, sem_init, &hall->semTables.full");
    }
    if (-1 == sem_init(&hall->semTables.m, 1, 1)) {
        errExit("initHallSemaphores, sem_init, &hall->semTables.m");
    }

    if (-1 == sem_init(&hall->chooseBarrier, 1, 0)) {
        errExit("initHallSemaphores, sem_init, &hall->chooseBarrier");
    }
    if (-1 == sem_init(&hall->addSem, 1, 0)) {
        errExit("initHallSemaphores, sem_init, &hall->addSem");
    }
}

void forkCounterPusher(struct MessHall *hall)
{
    if (0 == fork()) {
        actor.id = 0;
        actor.type = ACTOR_NONE;
        linkSignalHandler();
        counterPusher(hall);
        exitProcess(EXIT_SUCCESS);
    }
}

void forkCooks(struct MessHall *hall)
{
    for (size_t i = 1; i <= hall->args.N; ++i) {
        if (0 == fork()) {
            linkSignalHandler();
            actor.id = i;
            actor.type = COOK;
            cookMain(hall, &actor);
            exitProcess(EXIT_SUCCESS);
        }
    }
}

void forkStudents(struct MessHall *hall)
{
    for (size_t i = 1; i <= hall->args.M; ++i) {
        if (0 == fork()) {
            linkSignalHandler();
            actor.id = i;
            actor.type = STUD;
            studentMain(hall, &actor);
            exitProcess(EXIT_SUCCESS);
        }
    }
}

void linkSignalHandler()
{
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (0 > sigaction(SIGINT, &sa, NULL)) {
        errExit("linkSignalHandler, sigaction, sa");
    }
}

void signalHandler(int signo)
{
    int errnoBackup = errno;
    exitProcess(EXIT_FAILURE);
    errno = errnoBackup;
}

void exitProcess(int exitCode)
{
    int ret;
    close(hall->fd);
    if (SUPP == actor.type) {
        waitChilds();
        destroySemaphores(hall);
    }
    ret = munmap(hall->tables, hall->args.T * sizeof(uint8_t));
    if (-1 == ret) {
        errExit("exitProcess, munmap, tables");
    }
    ret = munmap(hall, sizeof(*hall));
    if (-1 == ret) {
        errExit("exitProcess, munmap, hall");
    }
    _Exit(exitCode);
}

void waitChilds()
{
    for (;;) {
        pid_t pid = waitpid(WAIT_ANY, NULL, 0);
        if (-1 == pid) {
            if (ECHILD == errno) {
                break;
            } else {
                errExit("waitChilds, waitpid, pid");
            }
        }
    }
}

void destroySemaphores(struct MessHall *hall)
{
    if (-1 == sem_destroy(&hall->semKitchen.empty)) {
        errExit("destroySemaphores, sem_destroy, &hall->semKitchen.empty");
    }
    if (-1 == sem_destroy(&hall->semKitchen.full)) {
        errExit("destroySemaphores, sem_destroy, &hall->semKitchen.full");
    }
    if (-1 == sem_destroy(&hall->semKitchen.m)) {
        errExit("destroySemaphores, sem_destroy, &hall->semKitchen.m");
    }

    if (-1 == sem_destroy(&hall->semCounter.empty)) {
        errExit("destroySemaphores, sem_destroy, &hall->semCounter.empty");
    }
    if (-1 == sem_destroy(&hall->semCounter.full)) {
        errExit("destroySemaphores, sem_destroy, &hall->semCounter.full");
    }
    if (-1 == sem_destroy(&hall->semCounter.m)) {
        errExit("destroySemaphores, sem_destroy, &hall->semCounter.m");
    }

    if (-1 == sem_destroy(&hall->semTables.empty)) {
        errExit("destroySemaphores, sem_destroy, &hall->semTables.empty");
    }
    if (-1 == sem_destroy(&hall->semTables.full)) {
        errExit("destroySemaphores, sem_destroy, &hall->semTables.full");
    }
    if (-1 == sem_destroy(&hall->semTables.m)) {
        errExit("destroySemaphores, sem_destroy, &hall->semTables.m");
    }

    if (-1 == sem_destroy(&hall->chooseBarrier)) {
        errExit("destroySemaphores, sem_destroy, &hall->chooseBarrier");
    }
    if (-1 == sem_destroy(&hall->addSem)) {
        errExit("destroySemaphores, sem_destroy, &hall->addSem");
    }
}