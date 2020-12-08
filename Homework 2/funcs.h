#ifndef __FUNCS_H__
#define __FUNCS_H__

#define TEMP_TEMPLATE ("tempfile_XXXXXX")
#define MIN_INPUT_BYTES (20)
#define INPUT_BUF_SIZE (20)
#define TEMP_BUF_SIZE (256)
#define OUTPUT_BUF_SIZE (512)
#define MOVE_BUF_SIZE (4096)
#define MIN_TEMP_SIZE (2)

void printUsage();
void errExit(const char *msg);
void wLockFile(int fd);
void wUnlockFile(int fd);
void signalBlock();
void signalUnblock();
int isPendingSignal(int signum);
char *createStr(const char *str);
off_t getFileSize(int fd);
off_t getLine(int oldFD, char *str);
void removeFirstLine(int oldFD, off_t size, size_t lineLen);
void waitSignal();

#endif