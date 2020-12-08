#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>

#include "helper.h"
#include "server.h"
#include "graph.h"
#include "workers.h"

static struct ServerConfig *conf;
extern volatile sig_atomic_t exitSignal;

void initWorkers(struct ServerConfig *ptr)
{
    conf = ptr;
    conf->count = 0;
    conf->currentCap = conf->initCap;
    conf->sockets = (int *)calloc(conf->currentCap, sizeof(int));
    conf->threads = (pthread_t *)calloc(conf->currentCap, sizeof(pthread_t));
    if (NULL == conf->sockets || NULL == conf->threads) {
        errExit("initWorkers, calloc");
    }
    if (0 != pthread_create(&conf->pooler, NULL, poolerMain, NULL)) {
        errExit("initWorkers, pthread_create");
    }
    if (0 != pthread_mutex_init(&conf->m, NULL)) {
        errExit("initWorkers, pthread_mutex_init");
    }
    if (0 != pthread_cond_init(&conf->e, NULL)) {
        errExit("initWorkers, pthread_cond_init");
    }
    if (0 != pthread_cond_init(&conf->f, NULL)) {
        errExit("initWorkers, pthread_cond_init");
    }
    if (0 != pthread_cond_init(&conf->r, NULL)) {
        errExit("initWorkers, pthread_cond_init");
    }
    for (uint16_t i = 0; i < conf->currentCap; ++i) {
        conf->sockets[i] = EMPTY_SOCKET;
        createWorker(i);
    }
}

void createWorker(uint16_t i)
{
    if (0 != pthread_create(&conf->threads[i], NULL, workerMain, (void *)(uintptr_t)i)) {
        errExit("createWorker, pthread_create");
    }
}

void freeWorkers()
{
    void *res;
    for (uint16_t i = 0; i < conf->currentCap; ++i) {
        if (0 != pthread_join(conf->threads[i], &res)) {
            errExit("freeWorkers, pthread_join");
        }
    }
    if (0 != pthread_join(conf->pooler, &res)) {
        errExit("freeWorkers, pthread_join");
    }
    if (0 != pthread_mutex_destroy(&conf->m)) {
        errExit("freeWorkers, pthread_mutex_destroy");
    }
    if (0 != pthread_cond_destroy(&conf->e)) {
        errExit("freeWorkers, pthread_cond_destroy");
    }
    if (0 != pthread_cond_destroy(&conf->f)) {
        errExit("freeWorkers, pthread_cond_destroy");
    }
    if (0 != pthread_cond_destroy(&conf->r)) {
        errExit("freeWorkers, pthread_cond_destroy");
    }
    free(conf->threads);
    free(conf->sockets);
}

void *poolerMain(void *data)
{
    char line[LINE_LEN];
    sprintf(line, "A pool of %u threads has been created\n", conf->currentCap);
    serverLog(line);
    blockSignal(SIGINT);
    if (0 != pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL)) {
        errExit("workerMain, pthread_setcanceltype");
    }
    while (!exitSignal) {
        poolerLoop();
    }
    return NULL;
}

void poolerLoop()
{
    char line[LINE_LEN];
    if (0 != pthread_mutex_lock(&conf->m)) {
        errExit("poolerLoop, pthread_mutex_lock");
    }
    pthread_cond_wait(&conf->f, &conf->m);
    if (exitSignal) {
        if (0 != pthread_cond_broadcast(&conf->r)) {
            errExit("poolerLoop, pthread_cond_broadcast");
        }
        if (0 != pthread_mutex_unlock(&conf->m)) {
            errExit("poolerLoop, pthread_mutex_unlock");
        }
        return;
    }
    double ratio = conf->count / (double)conf->currentCap;
    if (conf->maxCap > conf->currentCap && 0.75 <= ratio) {
        enlargePool();
        sprintf(line, "System load %.0f%%, pool extended to %u threads\n", 100*ratio, conf->currentCap);
        serverLog(line);
    }
    if (0 != pthread_cond_broadcast(&conf->r)) {
        errExit("poolerLoop, pthread_cond_broadcast");
    }
    if (0 != pthread_mutex_unlock(&conf->m)) {
        errExit("poolerLoop, pthread_mutex_unlock");
    }
}

void *workerMain(void *data)
{
    uint16_t idx = (uintptr_t)data;
    blockSignal(SIGINT);
    if (0 != pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL)) {
        errExit("workerMain, pthread_setcanceltype");
    }
    while (!exitSignal) {
        workerLoop(idx);
    }
    return NULL;
}

void workerLoop(uint16_t idx)
{
    char line[LINE_LEN];
    workerLog(idx, "waiting for connection\n");
    if (0 != pthread_mutex_lock(&conf->m)) {
        errExit("workerLoop, pthread_mutex_lock");
    }
    while (EMPTY_SOCKET == conf->sockets[idx]) {
        pthread_cond_wait(&conf->r, &conf->m);
        if (exitSignal) {
            if (0 != pthread_mutex_unlock(&conf->m)) {
                errExit("workerLoop, pthread_mutex_unlock");
            }
            return;
        }
    }
    int sockfd = conf->sockets[idx];
    if (0 != pthread_mutex_unlock(&conf->m)) {
        errExit("workerLoop, pthread_mutex_unlock");
    }
    uint32_t point[2];
    if (sizeof(point) > recv(sockfd, point, sizeof(point), 0)) {
        errExit("workerLoop, recv, point");
    }
    sprintf(line, "searching database for a path from node %u to node %u\n", point[0], point[1]);
    workerLog(idx, line);
    uint32_t *path = getPathCache(point[0], point[1]);
    if (NULL == path) {
        sprintf(line, "no path in database, calculating %u->%u\n", point[0], point[1]);
        workerLog(idx, line);
        path = search(point[0], point[1]);
        if (NULL == path) {
            sprintf(line, "path not possible from node %u to %u\n", point[0], point[1]);
            workerLog(idx, line);
            workerLog(idx, "responding to client and adding path to database\n");
            addPathCache(point[0], point[1], path);
        } else {
            char *ptr = line;
            ptr += sprintf(ptr, "path calculated: ");
            for (uint32_t i = 1; i <= path[0]; ++i) {
                if (1 != i) {
                    ptr += sprintf(ptr, "->");
                }
                ptr += sprintf(ptr, "%u", path[i]);
            }
            ptr += sprintf(ptr, "\n");
            workerLog(idx, line);
            workerLog(idx, "responding to client and adding path to database\n");
            addPathCache(point[0], point[1], path);
        }
    } else {
        if (0 == path[0]) {
            sprintf(line, "path found in database: path not possible from node %u to %u\n", point[0], point[1]);
            workerLog(idx, line);
            workerLog(idx, "responding to client\n");
        } else {
            char *ptr = line;
            ptr += sprintf(ptr, "path found in database: ");
            for (uint32_t i = 1; i <= path[0]; ++i) {
                if (1 != i) {
                    ptr += sprintf(ptr, "->");
                }
                ptr += sprintf(ptr, "%u", path[i]);
            }
            ptr += sprintf(ptr, "\n");
            workerLog(idx, line);
            workerLog(idx, "responding to client\n");
        }
    }
    sendPath(sockfd, path);
    if (0 != pthread_mutex_lock(&conf->m)) {
        errExit("workerLoop, pthread_mutex_lock");
    }
    if (-1 == close(sockfd)) {
        errExit("workerLoop, close");
    }
    conf->sockets[idx] = EMPTY_SOCKET;
    conf->count--;
    if (0 != pthread_cond_broadcast(&conf->e)) {
        errExit("workerLoop, pthread_cond_broadcast");
    }
    if (0 != pthread_mutex_unlock(&conf->m)) {
        errExit("workerLoop, pthread_mutex_unlock");
    }
}

void workerLog(uint16_t idx, char *str)
{
    char line[LINE_LEN];
    sprintf(line, "Thread #%u: %s", idx, str);
    serverLog(line);
}

void enlargePool()
{
    uint16_t newCap = ((double)conf->currentCap * 1.25) + 0.5;
    if (newCap >= conf->maxCap) {
        newCap = conf->maxCap;
    }
    conf->sockets = (int *)realloc(conf->sockets, newCap*sizeof(int));
    conf->threads = (pthread_t *)realloc(conf->threads, newCap*sizeof(pthread_t));
    if (NULL == conf->sockets || NULL == conf->threads) {
        errExit("enlargePool, realloc");
    }
    for (; conf->currentCap < newCap; ++conf->currentCap) {
        conf->sockets[conf->currentCap] = EMPTY_SOCKET;
        conf->threads[conf->currentCap] = 0;
        createWorker(conf->currentCap);
        if (0 != pthread_cond_broadcast(&conf->e)) {
            errExit("enlargePool, pthread_cond_broadcast");
        }
    }
}

void blockSignal(int signo)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, signo);
    if (0 > pthread_sigmask(SIG_BLOCK, &set, NULL)) {
        errExit("blockSignal, pthread_sigmask");
    }
}

void sendPath(int sockfd, uint32_t *path)
{
    uint32_t zero = 0;
    if (NULL == path) {
        if (sizeof(uint32_t) > send(sockfd, &zero, sizeof(uint32_t), 0)) {
            errExit("sendPath, send, zero");
        }
    } else if (0 == path[0]) {
        if (sizeof(uint32_t) > send(sockfd, &zero, sizeof(uint32_t), 0)) {
            errExit("sendPath, send, zero");
        }
    } else {
        if (sizeof(path[0]) > send(sockfd, path, sizeof(path[0]), 0)) {
            errExit("sendPath, send, path[0]");
        }
        for (uint32_t i = 0; i < path[0]; i += SEND_LIMIT) {
            uint64_t size = path[0] - i;
            if (SEND_LIMIT < size) {
                size = SEND_LIMIT;
            }
            size *= sizeof(uint32_t);
            if (size > send(sockfd, path+1+i, size, 0)) {
                errExit("sendPath, send, path");
            }
        }
    }
}