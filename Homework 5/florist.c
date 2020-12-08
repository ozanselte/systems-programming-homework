#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "florist.h"
#include "helper.h"
#include "structures.h"

extern size_t floristsLen;
extern struct Florist *florists;
extern size_t flowersLen;
extern char **flowersArr;

void createFlorist(struct Florist *florist)
{
    if (0 != pthread_create(&florist->id, NULL, floristMain, florist)) {
        errExit("createFlorist, pthread_create");
    }
}

void joinThreads()
{
    void *res;
    for (size_t i = 0; i < floristsLen; ++i) {
        if (0 != pthread_join(florists[i].id, &res)) {
            errExit("joinThreads, pthread_join");
        }
    }
}

void *floristMain(void *data)
{
    struct Florist *florist = (struct Florist *)data;
    if (0 != pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL)) {
        errExit("floristMain, pthread_setcanceltype");
    }
    while (floristOperations(florist)) {
        checkExitSignal();
    }
    return NULL;
}

int floristOperations(struct Florist *florist)
{
    if (0 != pthread_mutex_lock(&florist->m)) {
        errExit("floristOperations, pthread_mutex_lock");
    }
    while (NULL == florist->queue) {
        if (florist->end) {
            printClosing(florist);
            if (0 != pthread_mutex_unlock(&florist->m)) {
                errExit("floristOperations, pthread_mutex_unlock");
            }
            return 0;
        }
        checkExitSignal();
        pthread_cond_wait(&florist->c, &florist->m);
    }
    //* +++ CRITICAL BEGINS +++
    struct Client client;
    memcpy(&client, florist->queue, sizeof(struct Client));
    free(florist->queue);
    florist->queue = client.next;
    //! ---  CRITICAL ENDS  ---
    if (0 != pthread_mutex_unlock(&florist->m)) {
        errExit("floristOperations, pthread_mutex_unlock");
    }
    unsigned int state = time(NULL) ^ getpid() ^ pthread_self();
    size_t dur = rand_r(&state);
    dur %= (MAX_SLEEP_MS - MIN_SLEEP_MS);
    dur += MIN_SLEEP_MS;
    dur += florist->speed * client.distance;
    sleepMs(dur);
    printDelivered(florist, &client, dur);
    florist->sales++;
    florist->totalTime += dur;
    return 1;
}

void printDelivered(struct Florist *florist, struct Client *client, size_t duration)
{
    char str[LINE_LEN];
    sprintf(str, "Florist %s has delivered a %s to %s in %zums\n",
        florist->name,
        flowersArr[client->flower],
        client->name,
        duration
    );
    printStr(str);
}

void printClosing(struct Florist *florist)
{
    char str[LINE_LEN];
    sprintf(str, "%s closing shop.\n", florist->name);
    printStr(str);
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