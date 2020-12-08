#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "main.h"
#include "deps.h"

uint8_t *ingreds;
int sems;

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
    ingreds = (uint8_t *)calloc(INGR_COUNT, sizeof(uint8_t));
    initSemaphores();
    pthread_t chefs[6];
    struct ChefSpecs chefSpecs[6] = {
        {.a=INGR_M, .b=INGR_F, .id=1},
        {.a=INGR_M, .b=INGR_W, .id=2},
        {.a=INGR_M, .b=INGR_S, .id=3},
        {.a=INGR_F, .b=INGR_W, .id=4},
        {.a=INGR_F, .b=INGR_S, .id=5},
        {.a=INGR_W, .b=INGR_S, .id=6},
    };
    srand(time(NULL));
    for (size_t i = 0; i < 6; ++i) {
        chefs[i] = createChef(&chefSpecs[i]);
    }

    wholesalerMain(inputPath);

    joinThreads(chefs, 6);
    destroySemaphores();
    free(ingreds);
}

void printUsage(const char *name)
{
    printStr("Program Usage:\n\t");
    printStr(name);
    printStr(" -i <STRING>\n");
    printStr("\t-i\tInput file path\n");
    exit(EXIT_FAILURE);
}

void joinThreads(pthread_t *chefs, size_t N)
{
    for (size_t i = 0; i < N; ++i) {
        if (-1 == pthread_join(chefs[i], NULL)) {
            errExit("joinThreads, pthread_join");
        }
    }
}