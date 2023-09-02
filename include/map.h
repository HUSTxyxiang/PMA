#ifndef MAP_H
#define MAP_H 1

#include "def.h"
#include "PP.h"
#include <pthread.h>

typedef struct{
    PP *keys;
    long long *values;
    int size;
    pthread_mutex_t lock;
}MAP;

MAP *mapCreate(int size);

void mapDestroy(MAP *map);

void mapInsert(MAP *map, PP key, int value);

int mapFind(MAP *map, PP key);

int mapDelete(MAP *map, PP key);

#endif