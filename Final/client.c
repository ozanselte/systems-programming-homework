#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "helper.h"
#include "client.h"

int main(int argc, char *argv[])
{
    if (9 != argc) {
        printUsage(argv[0]);
    }
    char ipAddr[IP_LEN];
    uint16_t port;
    uint32_t points[2];
    char optC;
    while (-1 != (optC = getopt(argc, argv, ":a:p:s:d:"))) {
        switch (optC) {
        case 'a':
            strcpy(ipAddr, optarg);
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 's':
            points[0] = atol(optarg);
            break;
        case 'd':
            points[1] = atol(optarg);
            break;
        case ':':
        default:
            printUsage(argv[0]);
            break;
        }
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        errExit("main, socket");
    }
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(ipAddr);
	memset(&(serverAddr.sin_zero), '\0', 8);

    printConnecting(ipAddr, port);
    struct sockaddr *sockAddr = (struct sockaddr *)&serverAddr;
    if (-1 == connect(sockfd, sockAddr, sizeof(struct sockaddr))) {
        errExit("main, connect");
    }
    printConnected(points[0], points[1]);
    if (sizeof(points) > send(sockfd, points, sizeof(points), 0)) {
        errExit("main, send");
    }

    struct Timer timer;
    startTimer(&timer);
    uint32_t count = 0;
    uint32_t *nodes = NULL;
    if (sizeof(count) > recv(sockfd, &count, sizeof(count), 0)) {
        errExit("main, recv, count");
    }
    if (count) {
        nodes = (uint32_t *)calloc(count, sizeof(uint32_t));
        for (uint32_t i = 0; i < count; i += SEND_LIMIT) {
            uint64_t size = count - i;
            if (SEND_LIMIT < size) {
                size = SEND_LIMIT;
            }
            size *= sizeof(uint32_t);
            if (size > recv(sockfd, nodes+i, size, 0)) {
                errExit("main, recv, nodes");
            }
        }
    }
    stopTimer(&timer);
    char line[LINE_LEN], *ptr;
    ptr = line;
    ptr += sprintf(ptr, "Server's response (%d): ", (int)getpid());
    if (count) {
        for (uint32_t i = 0; i < count ; ++i) {
            if (0 != i) {
                ptr += sprintf(ptr, "->");
            }
            ptr += sprintf(ptr, "%u", nodes[i]);
        }
        free(nodes);
        ptr += sprintf(ptr, ", arrived in %.2fseconds.\n", readTimer(&timer));
    } else {
        ptr += sprintf(ptr, "NO PATH, arrived in %.2fseconds, shutting down\n", readTimer(&timer));
    }
    clientLog(line);
    if (-1 == close(sockfd)) {
        errExit("main, close");
    }
    exit(EXIT_SUCCESS);
}

void printUsage(const char *name)
{
    printStr("Program Usage:\n\t");
    printStr(name);
    printStr(" -a <STR> -p <INT> -s <INT> -d <INT>\n");
    printStr("\t-a\tIP address of the machine running the server\n");
    printStr("\t-p\tport number at which the server waits for connections\n");
    printStr("\t-s\tsource node of the requested path\n");
    printStr("\t-d\tdestination node of the requested path\n");
    exit(EXIT_FAILURE);
}

void clientLog(char *str)
{
    time_t tm = time(NULL);
    char timeStr[LINE_LEN];
    ctime_r(&tm, timeStr);
    timeStr[strlen(timeStr)-1] = '\0';
    char line[2*LINE_LEN];
    sprintf(line, "%s %s", timeStr, str);
    printStr(line);
}

void printConnecting(char *ip, uint16_t port)
{
    pid_t pid = getpid();
    char line[LINE_LEN];
    sprintf(line, "Client (%d) connecting to %s:%u\n", (int)pid, ip, port);
    clientLog(line);
}

void printConnected(uint32_t src, uint32_t dst)
{
    pid_t pid = getpid();
    char line[LINE_LEN];
    sprintf(line, "Client (%d) connected and requesting a path from %u to %u\n", (int)pid, src, dst);
    clientLog(line);
}