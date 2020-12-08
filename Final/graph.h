#pragma once

#include <pthread.h>

#define FIRST_CAP (8)
#define READER (0)
#define WRITER (1)

#define GET_BIT(a,n) (((a)[(n)/8] >> (7 - ((n)%8))) & 1)
#define SET_BIT(a,n) ((a)[(n)/8] |= 1 << (7 - ((n)%8)))
#define CLR_BIT(a,n) ((a)[(n)/8] &= ~(1 << (7 - ((n)%8))))

struct Queue {
    struct Queue *next;
    uint32_t *path;
};

struct Node {
    uint32_t count, cap;
    uint32_t *edges;
    uint32_t cacheCount;
    uint32_t *cache;
    uint32_t **paths;
};

struct Graph {
    uint32_t count, cap, edgeCount;
    uint32_t *indices;
    struct Node *nodes;
    pthread_mutex_t m;
    pthread_cond_t okHigh, okLow;
    uint16_t aL, aH, wL, wH;
};

void initGraph(uint8_t prio);
void expandGraph();
void initNode(uint32_t pos);
void expandNode(uint32_t pos);
void initNodeEdges(uint32_t pos);
void addPathCache(uint32_t srcId, uint32_t dstId, uint32_t *path);
uint32_t *getPathCache(uint32_t srcId, uint32_t dstId);
uint32_t createNode(uint32_t id);
void createEdge(uint32_t src, uint32_t dst);
int64_t findNode(uint32_t id);
void checkEdge(uint32_t src, uint32_t dst);
void processEdge(uint32_t srcId, uint32_t dstId);
void freeGraph();
uint32_t getNodeCount();
uint32_t getEdgeCount();
uint32_t *search(uint32_t srcId, uint32_t dstId);
uint32_t *bfs(struct Queue **head, uint8_t *vst, uint32_t dst);
void highPriorityLock();
void highPriorityUnlock(int caller);
void lowPriorityLock();
void lowPriorityUnlock(int caller);