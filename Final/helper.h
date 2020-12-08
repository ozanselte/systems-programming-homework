#pragma once

#include <sys/time.h>

//#define F_DEBUG
#define LINE_LEN (4096)
#define SEND_LIMIT (256)

struct Timer {
    struct timeval begin;
    struct timeval end;
};

void printStr(const char *str);
void errExit(const char *msg);
void readLine(int fd, char *line);
void startTimer(struct Timer *timer);
void stopTimer(struct Timer *timer);
double readTimer(struct Timer *timer);
int lockInstance(char *filename);
void signalBlock();
void signalUnblock();
int isPendingSignal();