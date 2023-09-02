#include "../include/map.h"
#include <stdio.h>
#include <stdlib.h>

MAP *mapCreate(int size)
{
    MAP *map = malloc(sizeof(MAP));
    map->keys = malloc(size * sizeof(PP));
    map->values = malloc(size * sizeof(int));
    map->size = size;
    pthread_mutex_init(&(map->lock), NULL);
    for(int i=0; i < size; i++)
        map->keys[i] = 0;
    return map;
}

void mapDestroy(MAP *map)
{
    free(map->keys);
    free(map->values);
    pthread_mutex_destroy(&(map->lock));
    free(map);
}

void mapInsert(MAP *map, PP key, int value)
{
    pthread_mutex_lock(&(map->lock));
    int hash = key % (map->size);
    if(map->keys[hash] == 0){
        map->keys[hash] = key;
        map->values[hash] = value;
    }
    else{
        int i;
        for(i = hash+1; i != hash; i = (i+1) % (map->size)){
            if(map->keys[i] == 0){
                map->keys[i] = key;
                map->values[i] = value;
                break;
            }
        }
        if(i == hash){
            printf("Error: map is full!\n");
            exit(0);
        }
    }
    pthread_mutex_unlock(&(map->lock));
}

int mapFind(MAP *map, PP key)
{
    int hash = key % (map->size);
    if(map->keys[hash] == key)
        return map->values[hash];
    for(int i = hash+1; i != hash; i = (i+1) % (map->size)){
        if(map->keys[i] == key)
            return map->values[i];
    }
    return 0;
}

int mapDelete(MAP *map, PP key)
{
    pthread_mutex_lock(&(map->lock));
    int hash = key % (map->size);
    if(map->keys[hash] == key){
        map->keys[hash] = 0;
    }
    else{
        for(int i = hash+1; i != hash; i = (i+1) % (map->size)){
            if(map->keys[i] == key){
                map->keys[i] = 0;
                break;
            }
        }
    }
    pthread_mutex_unlock(&(map->lock));
}