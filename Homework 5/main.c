#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "main.h"
#include "helper.h"
#include "structures.h"
#include "florist.h"

extern size_t floristsLen;
extern struct Florist *florists;
extern size_t flowersLen;
extern char **flowersArr;

int main(int argc, char *argv[])
{
    if (3 != argc) {
        printUsage(argv[0]);
    }
    char inputPath[INPUT_FILE_LEN];
    char optC;
    while (-1 != (optC = getopt(argc, argv, ":i:"))) {
        switch (optC) {
        case 'i':
            strcpy(inputPath, optarg);
            break;
        case ':':
        default:
            printUsage(argv[0]);
            break;
        }
    }
    linkParentHandler();
    printInit(inputPath);
    int fd = open(inputPath, O_RDWR | O_CREAT | O_SYNC, 0644);
    if (-1 == fd) {
        errExit("main, open");
    }
    char line[LINE_LEN];
    for (;;) {
        readLine(fd, line);
        if (0 == strlen(line)) {
            break;
        }
        char name[INPUT_FILE_LEN], flowers[LINE_LEN];
        double x, y, speed;
        size_t len = strslen(line, '(');
        strncpy(name, line, len-1);
        name[len-1] = '\0';
        sscanf(line+len, " (%lf,%lf; %lf) : %[^\n]", &x, &y, &speed, flowers);
        addFlorist(name, x, y, speed, flowers);
    }
    for (size_t i = 0; i < floristsLen; ++i) {
        createFlorist(florists + i);
    }
    blockSignal(SIGINT);
    printCreated(floristsLen);
    for (;;) {
        readLine(fd, line);
        if (0 == strlen(line)) {
            break;
        }
        char name[INPUT_FILE_LEN], flower[INPUT_FILE_LEN];
        double x, y;
        size_t len = strslen(line, '(');
        strncpy(name, line, len-1);
        name[len-1] = '\0';
        sscanf(line+len, " (%lf,%lf): %[^\n]", &x, &y, flower);
        addClient(name, x, y, flower);
    }

    for (size_t i = 0; i < floristsLen; ++i) {
        if (0 != pthread_mutex_lock(&florists[i].m)) {
            errExit("main, pthread_mutex_lock");
        }
        //* +++ CRITICAL BEGINS +++
        florists[i].end = 1;
        //! ---  CRITICAL ENDS  ---
        if (0 != pthread_cond_signal(&florists[i].c)) {
            errExit("main, pthread_cond_signal");
        }
        if (0 != pthread_mutex_unlock(&florists[i].m)) {
            errExit("main, pthread_mutex_unlock");
        }
    }
    exitGracefully();
}

void printUsage(const char *name)
{
    printStr("Program Usage:\n\t");
    printStr(name);
    printStr(" -i <STRING>\n");
    printStr("\t-i\tInput file path\n");
    exit(EXIT_FAILURE);
}

void printInit(char *name)
{
    char str[LINE_LEN];
    sprintf(str, "Florist application initializing from file: %s\n", name);
    printStr(str);
}

void printCreated(size_t num)
{
    char str[LINE_LEN];
    sprintf(str, "%zu florists have been created\nProcessing requests\n", num);
    printStr(str);
}