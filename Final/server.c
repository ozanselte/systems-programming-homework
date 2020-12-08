#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "helper.h"
#include "graph.h"
#include "workers.h"
#include "server.h"

static struct ServerConfig conf;
volatile sig_atomic_t exitSignal = 0;

int main(int argc, char *argv[])
{
    signalBlock();
    if (11 != argc && 13 != argc) {
        printUsage(argv[0]);
    }
    memset(&conf, '\0', sizeof(struct ServerConfig));
    conf.priority = PRIO_WRITER;
    conf.logFD = -1;
    conf.lockFD = lockInstance(LOCK_PATH);
    char optC;
    while (-1 != (optC = getopt(argc, argv, ":i:p:o:s:x:r:"))) {
        switch (optC) {
        case 'i':
            strcpy(conf.inputPath, optarg);
            break;
        case 'p':
            conf.port = atoi(optarg);
            break;
        case 'o':
            strcpy(conf.logPath, optarg);
            break;
        case 's':
            conf.initCap = atoi(optarg);
            if (2 > conf.initCap) {
                printUsage(argv[0]);
            }
            break;
        case 'x':
            conf.maxCap = atoi(optarg);
            if (2 > conf.maxCap) {
                printUsage(argv[0]);
            }
            break;
        case 'r':
            if (11 == argc) {
                printUsage(argv[0]);
            }
            conf.priority = atoi(optarg);
            if (2 < conf.priority) {
                printUsage(argv[0]);
            }
            break;
        case ':':
        default:
            printUsage(argv[0]);
            break;
        }
    }
    prepareSocket();
    if (-1 == close(conf.sockfd)) {
        errExit("main, close, sockfd");
    }
    conf.sockfd = EMPTY_SOCKET;
    if (-1 == close(conf.lockFD)) {
        errExit("main, close, lockFD");
    }
    conf.logFD = open(conf.logPath, O_RDWR | O_CREAT | O_SYNC, 0644);
    if (-1 == conf.logFD) {
        printStr("Program failed to access the log file!\n");
    } else if (-1 == close(conf.logFD)) {
        errExit("main, close, logFD");
    }
#ifndef F_DEBUG
    becomeDaemon();
#endif
    prepareSocket();
    conf.logFD = open(conf.logPath, O_RDWR | O_CREAT | O_SYNC, 0644);
    if (-1 == conf.logFD) {
        errExit("main, open, logFD");
    }
    if (-1 == ftruncate(conf.logFD, 0)) {
        errExit("main, ftruncate");
    }
    conf.lockFD = lockInstance(LOCK_PATH);
    printBeginLogs();
    readGraphFile();
    if (-1 == chdir("/")) {
        errExit("main, chdir");
    }
    initWorkers(&conf);
    linkSignalHandler();
    if (isPendingSignal()) {
        signalParent(SIGINT);
    }
    signalUnblock();
    while (!exitSignal) {
        processRequests();
    }
    exitGracefully();
    exit(EXIT_SUCCESS);
}

void printUsage(const char *name)
{
    printStr("Program Usage:\n\t");
    printStr(name);
    printStr(" -i <STR> -p <INT> -o <STR> -s <INT> -x <INT> -r [INT/OPTIONAL]\n");
    printStr("\t-i\tthe relative or absolute path to an input file\n");
    printStr("\t-p\tthe port number the server will use\n");
    printStr("\t-o\tthe relative or absolute path of the log file\n");
    printStr("\t-s\tthe number of threads in the pool at startup (>1)\n");
    printStr("\t-x\tthe maximum allowed number of threads(>1)\n");
    printStr("\t-r\tthe prioritization variable 0(r), 1(w), 2(none)\n");
    exit(EXIT_FAILURE);
}

void readGraphFile()
{
    serverLog("Loading graph...\n");
    struct Timer timer;
    startTimer(&timer);
    initGraph(conf.priority);
    int fd = open(conf.inputPath, O_RDONLY | O_SYNC, 0644);
    if (-1 == fd) {
        errExit("readGraph, open");
    }
    char line[LINE_LEN];
    for (;;) {
        readLine(fd, line);
        if ('\0' == line[0]) {
            break;
        } else if ('#' == line[0]) {
            continue;
        }
        int32_t src, dst;
        sscanf(line, "%d %d", &src, &dst);
        processEdge(src, dst);
    }
    if (-1 == close(fd)) {
        errExit("readGraph, close");
    }
    stopTimer(&timer);
    sprintf(line, "Graph loaded in %.2f seconds with %u nodes and %u edges.\n", readTimer(&timer), getNodeCount(), getEdgeCount());
    serverLog(line);
}

void exitGracefully()
{
    if (0 != pthread_mutex_lock(&conf.m)) {
        errExit("exitGracefully, pthread_mutex_lock");
    }
    if (0 != pthread_cond_broadcast(&conf.f)) {
        errExit("exitGracefully, pthread_cond_broadcast");
    }
    if (0 != pthread_cond_broadcast(&conf.r)) {
        errExit("exitGracefully, pthread_cond_broadcast");
    }
    if (0 != pthread_mutex_unlock(&conf.m)) {
        errExit("exitGracefully, pthread_mutex_unlock");
    }
    freeWorkers();
    freeGraph();
    serverLog("All threads have terminated, server shutting down.\n");
    if (-1 != conf.logFD && -1 == close(conf.logFD)) {
        errExit("exitGracefully, close");
    }
    if (-1 == close(conf.lockFD)) {
        errExit("exitGracefully, close");
    }
    if (-1 == close(conf.sockfd)) {
        errExit("exitGracefully, close, sockfd");
    }
    _Exit(EXIT_SUCCESS);
}

void becomeDaemon()
{
    switch (fork()) {
        case -1:
            errExit("becomeDaemon, fork");
            break;
        case 0:
            break;
        default:
            _exit(EXIT_SUCCESS);
    }
    if (-1 == setsid()) {
        errExit("becomeDaemon, setsid");
    }
    switch (fork()) {
        case -1:
            errExit("becomeDaemon, fork");
            break;
        case 0:
            break;
        default:
            _exit(EXIT_SUCCESS);
    }
    umask(0);
#ifndef F_DEBUG
    if (-1 == close(STDIN_FILENO)) {
        errExit("becomeDaemon, close");
    }
    if (-1 == close(STDOUT_FILENO)) {
        errExit("becomeDaemon, close");
    }
    if (-1 == close(STDERR_FILENO)) {
        errExit("becomeDaemon, close");
    }
    int fd = open("/dev/null", O_RDWR);
    if (STDIN_FILENO != fd) {
        errExit("becomeDaemon, open");
    }
    if (STDOUT_FILENO != dup2(STDIN_FILENO, STDOUT_FILENO)) {
        errExit("becomeDaemon, dup2");
    }
    if (STDERR_FILENO != dup2(STDIN_FILENO, STDERR_FILENO)) {
        errExit("becomeDaemon, dup2");
    }
#endif
}

void serverLog(char *str)
{
#ifndef F_DEBUG
    if (-1 == conf.logFD) {
        return;
    }
#endif
    time_t tm = time(NULL);
    char timeStr[LINE_LEN];
    ctime_r(&tm, timeStr);
    timeStr[strlen(timeStr)-1] = '\0';
    char line[2*LINE_LEN];
    sprintf(line, "%s %s", timeStr, str);
#ifndef F_DEBUG
    if (-1 == write(conf.logFD, line, strlen(line))) {
        errExit("serverLog, write, logFD");
    }
#else
    printStr(line);
#endif
}

void printBeginLogs()
{
    char line[2*LINE_LEN];
    serverLog("Executing with parameters:\n");
    sprintf(line, "-i %s\n", conf.inputPath);
    serverLog(line);
    sprintf(line, "-p %u\n", conf.port);
    serverLog(line);
    sprintf(line, "-o %s\n", conf.logPath);
    serverLog(line);
    sprintf(line, "-s %u\n", conf.initCap);
    serverLog(line);
    sprintf(line, "-x %u\n", conf.maxCap);
    serverLog(line);
    sprintf(line, "-r %u\n", conf.priority);
    serverLog(line);
}

void prepareSocket()
{
    if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) {
        errExit("prepareSocket, signal");
    }
    conf.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == conf.sockfd) {
        errExit("prepareSocket, socket");
    }
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(conf.port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&(sa.sin_zero), '\0', 8);
    if (-1 == bind(conf.sockfd, (struct sockaddr *)&sa, sizeof(struct sockaddr))) {
        errExit("prepareSocket, bind");
    }
    if (-1 == listen(conf.sockfd, SOMAXCONN)) {
        errExit("prepareSocket, listen");
    }
}

int processRequests()
{
    char line[LINE_LEN];
    int sockfd = EMPTY_SOCKET;
    struct sockaddr_in clientAddr = {0};
    socklen_t structSize = sizeof(clientAddr);
    double ratio = 0.0;
    sockfd = accept(conf.sockfd, (struct sockaddr *)&clientAddr, &structSize);
    if (exitSignal) {
        exitGracefully();
    }
    if (-1 == sockfd) {
        errExit("processRequests, accept");
    }
    if (0 != pthread_mutex_lock(&conf.m)) {
        errExit("processRequests, pthread_mutex_lock");
    }
    while (conf.currentCap == conf.count) {
        serverLog("No thread is available! Waiting for one.\n");
        pthread_cond_wait(&conf.e, &conf.m);
        if (exitSignal) {
            if (0 != pthread_mutex_unlock(&conf.m)) {
                errExit("processRequests, pthread_mutex_unlock");
            }
            exitGracefully();
        }
    }
    for (uint16_t i = 0; i < conf.currentCap; ++i) {
        if (EMPTY_SOCKET == conf.sockets[i]) {
            conf.sockets[i] = sockfd;
            conf.count++;
            ratio = conf.count / (double)conf.currentCap;
            sprintf(line, "A connection has been delegated to thread id #%u, system load %.1f%%\n",
                i, 100.0*ratio);
            serverLog(line);
            if (0 != pthread_cond_broadcast(&conf.f)) {
                errExit("processRequests, pthread_cond_broadcast");
            }
            break;
        }
    }
    if (0 != pthread_mutex_unlock(&conf.m)) {
        errExit("processRequests, pthread_mutex_unlock");
    }
    return 1;
}

void linkSignalHandler()
{
    struct sigaction sa;
    sa.sa_handler = signalParent;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_flags = SA_RESTART;
    if (0 > sigaction(SIGINT, &sa, NULL)) {
        errExit("linkParentHandler, sigaction");
    }
}

void signalParent(int signo)
{
    int errnoBackup = errno;
    exitSignal = 1;
    serverLog("Termination signal received, waiting for ongoing threads to complete.\n");
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        errExit("signalParent, socket");
    }
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(conf.port);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(&(serverAddr.sin_zero), '\0', 8);
    struct sockaddr *sockAddr = (struct sockaddr *)&serverAddr;
    if (-1 == connect(sockfd, sockAddr, sizeof(struct sockaddr))) {
        errExit("signalParent, connect");
    }
    if (-1 == close(sockfd)) {
        errExit("signalParent, close");
    }
    errno = errnoBackup;
}