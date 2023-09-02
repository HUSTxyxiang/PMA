#include <stdio.h>
#include "../include/def.h"
#include "../include/common.h"
#include "../include/TPMP.h"

TPMP *createTPMP(pthread_t TID)
{
    pthread_mutex_lock(&LOCK_TPMP);
    TPMP *tpmp = TPMPBaseAddr;
    int hash = (long long)TID % TPMP_N;
    if(tpmp[hash].tid == 0){
        tpmp[hash].tid = TID;
        pthread_mutex_unlock(&LOCK_TPMP);
        return &tpmp[hash];
    }
    for(int i = hash+1; i != hash; i = (i+1) % TPMP_N){
        if(tpmp[i].tid == 0){
            tpmp[i].tid = TID;
            pthread_mutex_unlock(&LOCK_TPMP);
            return &tpmp[i];
        }
    }
    pthread_mutex_unlock(&LOCK_TPMP);
    return NULL;
}

TPMP *findTPMP(pthread_t TID)
{
    TPMP *tpmp = TPMPBaseAddr;
    int hash = (long long)TID % TPMP_N;
    if(tpmp[hash].tid == TID)
        return &tpmp[hash];
    for(int i = hash+1; i != hash; i = (i+1) % TPMP_N){
        if(tpmp[i].tid == TID)
            return &tpmp[i];
    }
    return NULL;
}

PP AllocFromTPMP(TPMP *tpmp, int sizeclass)
{
    if(tpmp->freeLists[sizeclass].length <= 0){
        printf("Error: Alloc from an empty TPMP!\n");
        return NULLPP;
    }
    PP firstFreeBlock = tpmp->freeLists[sizeclass].header;
    tpmp->freeLists[sizeclass].header = *(PP *)PMA_getVA(tpmp->freeLists[sizeclass].header);
    tpmp->freeLists[sizeclass].length--;
    return firstFreeBlock;
}

void FreeToTPMP(TPMP *tpmp, int sc, PP pp)
{
    *(PP *)PMA_getVA(pp) = tpmp->freeLists[sc].header;
    tpmp->freeLists[sc].header = pp;
    tpmp->freeLists[sc].length++;
}

void FillTPMP(SBdescriptor *sb, TPMP *tpmp, int sizeclass)
{
    tpmp->freeLists[sizeclass].header = PMA_getPP((void *)sb + sizeof(SBdescriptor) + (sb->FirstFreeBlock) * (sb->BlockSize));
    tpmp->freeLists[sizeclass].length = sb->NumOfFreeBlocks;
    sb->FirstFreeBlock = -1;
    sb->NumOfFreeBlocks = 0;
}

void FlushTPMP(TPMP *tpmp, int sizeclass)
{
    while(tpmp->freeLists[sizeclass].length){
        PP block = tpmp->freeLists[sizeclass].header;
        SBdescriptor *sb = (SBdescriptor *)PMA_getVA((block >> SHIFT_N) << SHIFT_N);
        tpmp->freeLists[sizeclass].header = *(PP *)PMA_getVA(tpmp->freeLists[sizeclass].header);
        tpmp->freeLists[sizeclass].length--;
        AddBlockToSB(sb, PMA_getVA(block));
    }
}