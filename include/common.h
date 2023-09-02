#ifndef COMMON_H
#define COMMON_H 1

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "def.h"
#include "SB.h"
#include "map.h"

extern void *HeapBaseAddr;
extern long long HeapSize;

extern void *LogBaseAddr;

extern void *TPMPBaseAddr;

extern void *SBBaseAddr;
extern long long SBN;

extern SBdescriptor *EmptySBListHead;

extern SBdescriptor *EmptySBListTail;

extern SBdescriptor *PartialSBListHead[SIZE_CLASS_N];

extern SBdescriptor *PartialSBListTail[SIZE_CLASS_N];

extern SBdescriptor *FullSBListHead[SIZE_CLASS_N];

extern SBdescriptor *FullSBListTail[SIZE_CLASS_N];

extern pthread_mutex_t LOCK_LOG;

extern pthread_mutex_t LOCK_TPMP;

extern pthread_mutex_t LOCK_EMPTY;

extern pthread_mutex_t LOCK_PART[SIZE_CLASS_N];

extern pthread_mutex_t LOCK_FULL[SIZE_CLASS_N];

extern MAP *map;

void clflush(const void *ptr);

void clflush_range(const void *ptr, size_t len);

void clflushopt(const void *ptr);

void clflushopt_range(const void *ptr, size_t len);

void clwb(const void *ptr);

void clwb_range(const void *ptr, size_t len);

void PMA_persist(void *addr, size_t len);

void PMA_barrier();

double GetTimeInSeconds(void);

#endif