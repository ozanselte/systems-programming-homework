#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "helper.h"
#include "server.h"
#include "graph.h"

static struct Graph graph = {.count=0, .cap=0, .indices=NULL, .nodes=NULL};
static uint8_t priority;

void initGraph(uint8_t prio)
{
    graph.count = 0;
    graph.cap = FIRST_CAP;
    graph.edgeCount = 0;
    graph.indices = (uint32_t *)calloc(graph.cap, sizeof(uint32_t));
    graph.nodes = (struct Node *)calloc(graph.cap, sizeof(struct Node));
    if (NULL == graph.indices || NULL == graph.nodes) {
        errExit("initGraph, calloc");
    }
    if (0 != pthread_mutex_init(&graph.m, NULL)) {
        errExit("initGraph, pthread_mutex_init");
    }
    if (0 != pthread_cond_init(&graph.okHigh, NULL)) {
        errExit("initGraph, pthread_cond_init");
    }
    if (0 != pthread_cond_init(&graph.okLow, NULL)) {
        errExit("initGraph, pthread_cond_init");
    }
    graph.aL = 0;
    graph.aH = 0;
    graph.wL = 0;
    graph.wH = 0;
    priority = prio;
}

void expandGraph()
{
    graph.cap *= 2;
    graph.indices = (uint32_t *)realloc(graph.indices, graph.cap*sizeof(uint32_t));
    graph.nodes = (struct Node *)realloc(graph.nodes, graph.cap*sizeof(struct Node));
    if (NULL == graph.indices || NULL == graph.nodes) {
        errExit("expandGraph, realloc");
    }
}

void initNode(uint32_t pos)
{
    struct Node *node = &graph.nodes[pos];
    node->count = 0;
    node->cap = 0;
    node->edges = (uint32_t *)NULL;
    node->cacheCount = 0;
    node->cache = (uint32_t *)NULL;
    node->paths = (uint32_t **)NULL;
}

void expandNode(uint32_t pos)
{
    struct Node *node = &graph.nodes[pos];
    node->cap *= 2;
    node->edges = (uint32_t *)realloc(node->edges, node->cap*sizeof(uint32_t));
    if (NULL == node->edges) {
        errExit("expandNode, calloc");
    }
}

void addPathCache(uint32_t srcId, uint32_t dstId, uint32_t *path)
{
    switch (priority) {
        case PRIO_READER: lowPriorityLock(); break;
        case PRIO_WRITER: highPriorityLock(); break;
        case PRIO_NONE: highPriorityLock(); break;
        default: errExit("addPathCache switch, lock");
    }
    uint32_t src = findNode(srcId);
    uint32_t dst = findNode(dstId);
    if (-1 == src || -1 == dst) {
        errExit("addPathCache, path error");
    }

    struct Node *node = &graph.nodes[src];
    node->cacheCount++;
    if (1 == node->cacheCount) {
        node->cache = (uint32_t *)malloc(sizeof(uint32_t));
        node->paths = (uint32_t **)malloc(sizeof(uint32_t *));
        if (NULL == node->cache || NULL == node->paths) {
            errExit("addPathCache, malloc");
        }
    } else {
        node->cache = (uint32_t *)realloc(node->cache, node->cacheCount*sizeof(uint32_t));
        node->paths = (uint32_t **)realloc(node->paths, node->cacheCount*sizeof(uint32_t *));
        if (NULL == node->cache || NULL == node->paths) {
            errExit("addPathCache, realloc");
        }
    }
    uint32_t idx = node->cacheCount - 1;
    node->cache[idx] = dst;
    if (NULL == path) {
        node->paths[idx] = (uint32_t *)malloc(sizeof(uint32_t));
        if (NULL == node->paths[idx]) {
            errExit("addPathCache, malloc");
        }
        node->paths[idx][0] = 0;
    } else {
        node->paths[idx] = path;
    }
    switch (priority) {
        case PRIO_READER: lowPriorityUnlock(WRITER); break;
        case PRIO_WRITER: highPriorityUnlock(WRITER); break;
        case PRIO_NONE: highPriorityUnlock(READER); break;
        default: errExit("addPathCache switch, unlock");
    }
}

uint32_t *getPathCache(uint32_t srcId, uint32_t dstId)
{
    switch (priority) {
        case PRIO_READER: highPriorityLock(); break;
        case PRIO_WRITER: lowPriorityLock(); break;
        case PRIO_NONE: highPriorityLock(); break;
        default: errExit("addPathCache switch, lock");
    }
    uint32_t src = findNode(srcId);
    uint32_t dst = findNode(dstId);
    if (-1 == src || -1 == dst) {
        return NULL;
    }
    struct Node *node = &graph.nodes[src];
    uint32_t *path = NULL;
    for (uint32_t i = 0; i < node->cacheCount; ++i) {
        if (dst == node->cache[i]) {
            path = node->paths[i];
            break;
        }
    }
    switch (priority) {
        case PRIO_READER: highPriorityUnlock(READER); break;
        case PRIO_WRITER: lowPriorityUnlock(READER); break;
        case PRIO_NONE: highPriorityUnlock(READER); break;
        default: errExit("addPathCache switch, unlock");
    }
    return path;
}

void initNodeEdges(uint32_t pos)
{
    struct Node *node = &graph.nodes[pos];
    node->count = 0;
    node->cap = FIRST_CAP;
    node->edges = (uint32_t *)calloc(node->cap, sizeof(uint32_t));
    if (NULL == node->edges) {
        errExit("initNodeEdges, calloc");
    }
}

uint32_t createNode(uint32_t id)
{
    graph.indices[graph.count] = id;
    initNode(graph.count);
    ++graph.count;
    if (graph.count == graph.cap) {
        expandGraph();
    }
    return graph.count-1;
}

void createEdge(uint32_t src, uint32_t dst)
{
    if (0 == graph.nodes[src].cap) {
        initNodeEdges(src);
    }
    graph.nodes[src].edges[graph.nodes[src].count] = dst;
    ++graph.nodes[src].count;
    if (graph.nodes[src].count == graph.nodes[src].cap) {
        expandNode(src);
    }
    ++graph.edgeCount;
}

int64_t findNode(uint32_t id)
{
    for (uint32_t i = 0; i < graph.count; ++i) {
        if (id == graph.indices[i]) {
            return i;
        }
    }
    return -1;
}

void checkEdge(uint32_t src, uint32_t dst)
{
    struct Node *node = &graph.nodes[src];
    for (uint32_t i = 0; i < node->count; ++i) {
        if (dst == node->edges[i]) {
            return;
        }
    }
    createEdge(src, dst);
}

void processEdge(uint32_t srcId, uint32_t dstId)
{
    int64_t src = findNode(srcId);
    if (-1 == src) {
        src = createNode(srcId);
    }
    int64_t dst = findNode(dstId);
    if (-1 == dst) {
        dst = createNode(dstId);
    }
    if (src == dst) {
        return;
    }
    checkEdge(src, dst);
}

void freeGraph()
{
    for (uint32_t i = 0; i < graph.count; ++i) {
        if (NULL != graph.nodes[i].edges) {
            free(graph.nodes[i].edges);
        }
        if (0 != graph.nodes[i].cacheCount) {
            free(graph.nodes[i].cache);
            for (uint32_t j = 0; j < graph.nodes[i].cacheCount; ++j) {
                free(graph.nodes[i].paths[j]);
            }
            free(graph.nodes[i].paths);
        }
    }
    if (NULL != graph.indices) {
        free(graph.indices);
    }
    if (NULL != graph.nodes) {
        free(graph.nodes);
    }
    if (0 != pthread_mutex_destroy(&graph.m)) {
        errExit("freeGraph, pthread_mutex_destroy");
    }
    if (0 != pthread_cond_destroy(&graph.okHigh)) {
        errExit("freeGraph, pthread_cond_destroy");
    }
    if (0 != pthread_cond_destroy(&graph.okLow)) {
        errExit("freeGraph, pthread_cond_destroy");
    }
}

uint32_t getNodeCount()
{
    return graph.count;
}

uint32_t getEdgeCount()
{
    return graph.edgeCount;
}

uint32_t *search(uint32_t srcId, uint32_t dstId)
{
    uint32_t src = findNode(srcId);
    uint32_t dst = findNode(dstId);
    if (-1 == src || -1 == dst) {
        return NULL;
    }
    struct Queue *tmp;
    struct Queue *queue = (struct Queue *)malloc(sizeof(struct Queue));
    if (NULL == queue) {
        errExit("search, malloc");
    }
    queue->next = NULL;
    queue->path = (uint32_t *)calloc(2, sizeof(uint32_t));
    if (NULL == queue->path) {
        errExit("search, calloc");
    }
    queue->path[0] = 1;
    queue->path[1] = src;
    uint8_t *vst = (uint8_t *)calloc(graph.count+7/8, sizeof(uint8_t));
    if (NULL == vst) {
        errExit("search, calloc");
    }
    SET_BIT(vst, src);
    uint32_t *path = bfs(&queue, vst, dst);
    while (NULL != queue) {
        tmp = queue;
        queue = queue->next;
        free(tmp->path);
        free(tmp);
    }
    free(vst);
    if (NULL == path) {
        return NULL;
    }
    path = realloc(path, sizeof(uint32_t)*(path[0] + 2));
    if (NULL == path) {
        errExit("search, realloc");
    }
    path[0]++;
    path[path[0]] = dst;
    for (uint32_t i = 1; i <= path[0]; ++i) {
        path[i] = graph.indices[path[i]];
    }
    return path;
}

uint32_t *bfs(struct Queue **head, uint8_t *vst, uint32_t dst)
{
    struct Queue *q = *head, *p;
    struct Node *node;
    uint32_t pos;
    while (NULL != q) {
        pos = q->path[q->path[0]];
        node = &graph.nodes[pos];
        if (dst == pos) {
            *head = q->next;
            uint32_t *path = (uint32_t *)malloc(sizeof(uint32_t));
            if (NULL == path) {
                errExit("bfs, malloc");
            }
            path[0] = 0;
            free(q->path);
            free(q);
            return path;
        }
        for (uint32_t i = 0; i < node->count; ++i) {
            if (0 == GET_BIT(vst, node->edges[i])) {
                SET_BIT(vst, node->edges[i]);
                if (dst == node->edges[i]) {
                    *head = q->next;
                    uint32_t *path = q->path;
                    free(q);
                    return path;
                }
                p = q;
                while (NULL != p->next) {
                    p = p->next;
                }
                p->next = (struct Queue *)malloc(sizeof(struct Queue));
                if (NULL == p) {
                    errExit("bfs, malloc");
                }
                p = p->next;
                p->next = NULL;
                p->path = (uint32_t *)calloc(q->path[0] + 2, sizeof(uint32_t));
                if (NULL == p->path) {
                    errExit("bfs, calloc");
                }
                p->path[0] = q->path[0] + 1;
                memcpy(p->path+1, q->path+1, sizeof(uint32_t)*q->path[0]);
                p->path[p->path[0]] = node->edges[i];
            }
        }
        *head = q->next;
        free(q->path);
        free(q);
        q = *head;
    }
    return NULL;
}

void highPriorityLock()
{
    if (0 != pthread_mutex_lock(&graph.m)) {
        errExit("highPriorityLock, pthread_mutex_lock");
    }
    while (0 < graph.aH + graph.aL) {
        graph.wH++;
        pthread_cond_wait(&graph.okHigh, &graph.m);
        graph.wH--;
    }
    graph.aH++;
    if (0 != pthread_mutex_unlock(&graph.m)) {
        errExit("highPriorityLock, pthread_mutex_unlock");
    }
}

void highPriorityUnlock(int caller)
{
    if (0 != pthread_mutex_lock(&graph.m)) {
        errExit("highPriorityUnlock, pthread_mutex_lock");
    }
    graph.aH--;
    if (0 < graph.wH) {
        if (0 != pthread_cond_signal(&graph.okHigh)) {
            errExit("highPriorityUnlock, pthread_cond_signal");
        }
    } else if (0 < graph.wL) {
        if (WRITER == caller && 0 != pthread_cond_broadcast(&graph.okLow)) {
            errExit("highPriorityUnlock, pthread_cond_broadcast");
        } else if (READER == caller && 0 != pthread_cond_signal(&graph.okLow)) {
            errExit("highPriorityUnlock, pthread_cond_signal");
        }
    }
    if (0 != pthread_mutex_unlock(&graph.m)) {
        errExit("highPriorityUnlock, pthread_mutex_unlock");
    }
}

void lowPriorityLock()
{
    if (0 != pthread_mutex_lock(&graph.m)) {
        errExit("lowPriorityLock, pthread_mutex_lock");
    }
    while (0 < graph.aH + graph.wH) {
        graph.wL++;
        pthread_cond_wait(&graph.okLow, &graph.m);
        graph.wL--;
    }
    graph.aL++;
    if (0 != pthread_mutex_unlock(&graph.m)) {
        errExit("lowPriorityLock, pthread_mutex_unlock");
    }
}

void lowPriorityUnlock(int caller)
{
    if (0 != pthread_mutex_lock(&graph.m)) {
        errExit("lowPriorityUnlock, pthread_mutex_lock");
    }
    graph.aL--;
    if (0 == graph.aL && 0 < graph.wH) {
        if (WRITER == caller && 0 != pthread_cond_broadcast(&graph.okHigh)) {
            errExit("lowPriorityUnlock, pthread_cond_broadcast");
        } else if (READER == caller && 0 != pthread_cond_signal(&graph.okHigh)) {
            errExit("lowPriorityUnlock, pthread_cond_signal");
        }
    }
    if (0 != pthread_mutex_unlock(&graph.m)) {
        errExit("lowPriorityUnlock, pthread_mutex_unlock");
    }
}