#pragma once

#define IP_LEN (16)

int main(int argc, char *argv[]);
void printUsage(const char *name);
void clientLog(char *str);
void printConnecting(char *ip, uint16_t port);
void printConnected(uint32_t src, uint32_t dst);