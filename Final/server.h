#pragma once

#include <stdint.h>
#include <pthread.h>

#include "helper.h"

#define EMPTY_SOCKET (-2)
#define PRIO_READER (0)
#define PRIO_WRITER (1)
#define PRIO_NONE (2)
#define LOCK_PATH "/tmp/cse344_final_lock.pid"

struct ServerConfig {
    char inputPath[LINE_LEN], logPath[LINE_LEN];
    uint16_t port;
    int logFD, inFD, lockFD;
    uint16_t initCap;
    uint16_t maxCap;
    uint16_t currentCap;
    uint16_t count;
    uint8_t priority;
    int sockfd;
    int *sockets;
    pthread_t pooler;
    pthread_t *threads;
    pthread_mutex_t m;
    pthread_cond_t e, f, r;
};

int main(int argc, char *argv[]);
void printUsage(const char *name);
void readGraphFile();
void exitGracefully();
void becomeDaemon();
void serverLog(char *str);
void printBeginLogs();
void prepareSocket();
int processRequests();
void linkSignalHandler();
void signalParent(int signo);