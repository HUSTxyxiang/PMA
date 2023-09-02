#ifndef TPMP_H
#define TPMP_H 1

#include <pthread.h>
#include "PP.h"
#include "sizeclass.h"

typedef struct{// The descriptor of a thread-private memory pool
    
    pthread_t tid;  // The owner thread of this TPMP.
    
    struct{
        
        PP header;  // Header pointer to the free block list.
        
        size_t length; // Length of the free block list.
    
    } freeLists[SIZE_CLASS_N];// One free list for one size class. 

} TPMP;

TPMP *createTPMP(pthread_t TID);

TPMP *findTPMP(pthread_t TID);

PP AllocFromTPMP(TPMP *tpmp, int sizeclass);

void FreeToTPMP(TPMP *tpmp, int sc, PP pp);

void FillTPMP(SBdescriptor *sb, TPMP *tpmp, int sizeclass);

void FlushTPMP(TPMP *tpmp, int sizeclass);

#endif