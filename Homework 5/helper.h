#pragma once

#define LINE_LEN (1024)

void printStr(const char *str);
void errExit(const char *msg);
size_t strslen(char *str, char sep);
void readLine(int fd, char *line);
void sleepMs(size_t ms);
void linkParentHandler();
void signalHandlerParent(int signo);
void checkExitSignal();
void exitGracefully();
void printStatistics();