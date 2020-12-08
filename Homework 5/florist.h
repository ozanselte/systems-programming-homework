#pragma once

#include <pthread.h>
#include "structures.h"

#define MAX_SLEEP_MS (250)
#define MIN_SLEEP_MS (1)

void createFlorist(struct Florist *florist);
void joinThreads();
void *floristMain(void *data);
int floristOperations(struct Florist *florist);
void printDelivered(struct Florist *florist, struct Client *client, size_t duration);
void printClosing(struct Florist *florist);
void blockSignal(int signo);