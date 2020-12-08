#pragma once

#include <unistd.h>
#include <pthread.h>

#define NAME_LEN (128)

struct Client {
    char name[NAME_LEN];
    double x;
    double y;
    double distance;
    size_t flower;
    struct Client *next;
};

struct Florist {
    pthread_t id;
    char name[NAME_LEN];
    double x;
    double y;
    double speed;
    int end;
    size_t sales;
    size_t totalTime;
    size_t *flowers;
    struct Client *queue;
    pthread_mutex_t m;
    pthread_cond_t c;
};

void addFlorist(char *name, double x, double y, double speed, char *flowers);
void addClient(char *name, double x, double y, char *flower);
double distance(double x1, double y1, double x2, double y2);
size_t findFlowerId(char *name);
struct Florist *findNearest(size_t flower, double x, double y);
void freeStructures();
void freeQueue(struct Client *client);