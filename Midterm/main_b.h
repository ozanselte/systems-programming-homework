#ifndef __MAIN_H__
#define __MAIN_H__

#include "deps_b.h"

void printUsage(const char *name);
void initHall(struct MessHall *hall);
void *createSharedMem(size_t size);
void initHallSemaphores(struct MessHall *hall);
void forkCounterPusher(struct MessHall *hall);
void forkCooks(struct MessHall *hall);
void forkStudents(struct MessHall *hall);
void linkSignalHandler();
void signalHandler(int signo);
void exitProcess(int exitCode);
void waitChilds();
void destroySemaphores(struct MessHall *hall);

#endif