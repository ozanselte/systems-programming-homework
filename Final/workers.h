#pragma once

#include <pthread.h>

#include "server.h"

void initWorkers(struct ServerConfig *ptr);
void createWorker(uint16_t i);
void freeWorkers();
void *poolerMain(void *data);
void poolerLoop();
void *workerMain(void *data);
void workerLoop(uint16_t idx);
void workerLog(uint16_t idx, char *str);
void enlargePool();
void blockSignal(int signo);
void sendPath(int sockfd, uint32_t *path);