#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "structures.h"
#include "helper.h"

size_t floristsLen = 0;
struct Florist *florists = NULL;

size_t flowersLen = 0;
char **flowersArr = NULL;

void addFlorist(char *name, double x, double y, double speed, char *flowers)
{
    if (0 == florists) {
        florists = (struct Florist *)malloc(sizeof(struct Florist));
    } else {
        florists = (struct Florist *)realloc(
            florists,
            (floristsLen+1) * sizeof(struct Florist)
        );
    }
    struct Florist *florist = florists + floristsLen;
    strcpy(florist->name, name);
    florist->x = x;
    florist->y = y;
    if (0 >= speed) {
        errExit("Speed must be larger than zero.");
    }
    florist->speed = speed;
    florist->end = 0;
    florist->sales = 0;
    florist->totalTime = 0;
    florist->queue = NULL;
    size_t commas = 1;
    char *ptr = flowers;
    while ('\0' != *ptr) {
        if (',' == *ptr) {
            commas++;
        }
        ptr++;
    }
    florist->flowers = (size_t *)calloc(commas+1, sizeof(size_t));
    florist->flowers[0] = commas;
    for (size_t i = 1; i <= commas; ++i) {
        size_t keylen = strslen(flowers, ',');
        int isAttended = 0;
        for (size_t j = 0; j < flowersLen; ++j) {
            if (!strncmp(flowersArr[j], flowers, keylen)) {
                florist->flowers[i] = j;
                isAttended = 1;
            }
        }
        if (!isAttended) {
            if (0 == flowersLen) {
                flowersArr = (char **)malloc(sizeof(char *));
            } else {
                flowersArr = (char **)realloc(flowersArr, (flowersLen+1) * sizeof(char *));
            }
            flowersArr[flowersLen] = (char *)calloc(keylen+1, sizeof(char));
            strncpy(flowersArr[flowersLen], flowers, keylen);
            florist->flowers[i] = flowersLen;
            flowersLen++;
        }
        flowers += keylen;
        if (',' == *flowers) {
            flowers += 2;
        }
    }
    if (0 != pthread_mutex_init(&florist->m, NULL)) {
        errExit("addFlorist, pthread_mutex_init");
    }
    if (0 != pthread_cond_init(&florist->c, NULL)) {
        errExit("addFlorist, pthread_cond_init");
    }
    floristsLen++;
}

void addClient(char *name, double x, double y, char *flower)
{
    struct Client *client = (struct Client *)malloc(sizeof(struct Client));
    strcpy(client->name, name);
    client->x = x;
    client->y = y;
    client->next = NULL;
    client->flower = findFlowerId(flower);
    struct Florist *florist = findNearest(client->flower, x, y);
    client->distance = distance(client->x, client->y, florist->x, florist->y);

    if (0 != pthread_mutex_lock(&florist->m)) {
        errExit("addClient, pthread_mutex_lock");
    }
    //* +++ CRITICAL BEGINS +++
    if (NULL == florist->queue) {
        florist->queue = client;
    } else {
        struct Client *node = florist->queue;
        while (NULL != node->next) {
            node = node->next;
        }
        node->next = client;
    }
    //! ---  CRITICAL ENDS  ---
    if (0 != pthread_cond_signal(&florist->c)) {
        errExit("addClient, pthread_cond_signal");
    }
    if (0 != pthread_mutex_unlock(&florist->m)) {
        errExit("addClient, pthread_mutex_unlock");
    }
}

double distance(double x1, double y1, double x2, double y2)
{
    double dx = fabs(x1 - x2);
    double dy = fabs(y1 - y2);
    if (dx > dy) {
        return dx;
    }
    return dy;
}

size_t findFlowerId(char *name)
{
    for (size_t i = 0; i < flowersLen; ++i) {
        if(!strcmp(flowersArr[i], name)) {
            return i;
        }
    }
    errExit("Flower not found!");
    return 0;
}

struct Florist *findNearest(size_t flower, double x, double y)
{
    struct Florist *nearest;
    double dist = DBL_MAX;
    int isAttended = 0;
    for (size_t i = 0; i < floristsLen; ++i) {
        struct Florist *florist = &florists[i];
        for (size_t j = 1; j <= florist->flowers[0]; ++j) {
            if (flower == florist->flowers[j]) {
                double d = distance(x, y, florist->x, florist->y);
                if (d < dist) {
                    dist = d;
                    nearest = florist;
                    isAttended = 1;
                    break;
                }
            }
        }
    }
    if (0 == isAttended) {
        errExit("Florist not found!");
    }
    return nearest;
}

void freeStructures()
{
    for (size_t i = 0; i < floristsLen; ++i) {
        if (0 != pthread_mutex_destroy(&florists[i].m)) {
            errExit("freeStructures, pthread_mutex_destroy");
        }
        if (0 != pthread_cond_destroy(&florists[i].c)) {
            errExit("freeStructures, pthread_cond_destroy");
        }
        freeQueue(florists[i].queue);
        free(florists[i].flowers);
    }
    free(florists);
    floristsLen = 0;
    florists = NULL;

    for (size_t i = 0; i < flowersLen; ++i) {
        free(flowersArr[i]);
    }
    free(flowersArr);
    flowersLen = 0;
    flowersArr = NULL;
}

void freeQueue(struct Client *client)
{
    if (NULL != client) {
        if (NULL != client->next) {
            freeQueue(client->next);
        }
        free(client);
    }
}