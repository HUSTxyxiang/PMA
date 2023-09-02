#include "../include/common.h"

void *HeapBaseAddr;
long long HeapSize;

void *LogBaseAddr;

void *TPMPBaseAddr;

void *SBBaseAddr;
long long SBN;

SBdescriptor *EmptySBListHead;

SBdescriptor *EmptySBListTail;

SBdescriptor *PartialSBListHead[SIZE_CLASS_N];

SBdescriptor *PartialSBListTail[SIZE_CLASS_N];

SBdescriptor *FullSBListHead[SIZE_CLASS_N];

SBdescriptor *FullSBListTail[SIZE_CLASS_N];

pthread_mutex_t LOCK_LOG;

pthread_mutex_t LOCK_TPMP;

pthread_mutex_t LOCK_EMPTY;

pthread_mutex_t LOCK_PART[SIZE_CLASS_N];

pthread_mutex_t LOCK_FULL[SIZE_CLASS_N];

MAP *map;


void clflush(const void *ptr) {
    asm volatile("clflush %0" : "+m" (ptr));
}

void clflush_range(const void *ptr, size_t len) {
    unsigned long long start = (((unsigned long long)ptr) >> 6) << 6;
    for (; start < (unsigned long long)ptr + len; start += CACHE_LINE_SIZE) {
        clflush((void*)start);
    }
}

void clflushopt(const void *ptr) {
    asm volatile("clflushopt %0" : "+m" (ptr));
}

void clflushopt_range(const void *ptr, size_t len) {
    unsigned long long start = (((unsigned long long)ptr) >> 6) << 6;
    for (; start < (unsigned long long)ptr + len; start += CACHE_LINE_SIZE) {
        clflushopt((void*)start);
    }
}

void clwb(const void *ptr) {
    asm volatile("clwb %0" : "+m" (ptr));
}

void clwb_range(const void *ptr, size_t len) {
    unsigned long long start = (((unsigned long long)ptr) >> 6) << 6;
    for (; start < (unsigned long long)ptr + len; start += CACHE_LINE_SIZE) {
        clwb((void*)start);
    }
}

void PMA_persist(void *addr, size_t len)
{
    clwb_range(addr, len);
}

void PMA_barrier()
{
    asm volatile("sfence":::"memory");
}

double GetTimeInSeconds(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + (double)t.tv_usec / 1000000.0;
}