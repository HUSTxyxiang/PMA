#include "../include/def.h"
#include "../include/common.h"
#include "../include/PP.h"
#include "../include/SB.h"
#include <stdio.h>

void SBinit(SBdescriptor *sb, int sizeclass)
{
    sb->SizeClass = sizeclass;
    sb->BlockSize = getSize(sizeclass);
    int n = (SB_SIZE - sizeof(SBdescriptor)) / (sb->BlockSize);
    sb->NumOfFreeBlocks = n;
    sb->FirstFreeBlock = 0;
    for(int i=0; i < n-1; i++){
        *(PP *)((void *)sb + sizeof(SBdescriptor) + i * (sb->BlockSize))
        = PMA_getPP((void *)sb + sizeof(SBdescriptor) + (i+1) * (sb->BlockSize));
    }
    *(PP *)((void *)sb + sizeof(SBdescriptor) + (n-1) * (sb->BlockSize)) = NULLPP;        
}

void AddBlockToSB(SBdescriptor *sb, void *block)
{
    *(PP *)block = PMA_getPP((void *)sb + sizeof(SBdescriptor) + (sb->FirstFreeBlock) * (sb->BlockSize));
    sb->FirstFreeBlock = (block - ((void *)sb + sizeof(SBdescriptor))) / (sb->BlockSize);
    sb->NumOfFreeBlocks++;
    if(sb->NumOfFreeBlocks == 1){
        SBremove(sb);
        SBadd(PartialSBListHead[sb->SizeClass], sb);
    }
    else if(sb->NumOfFreeBlocks == (SB_SIZE - sizeof(SBdescriptor)) / (sb->BlockSize)){
        sb->SizeClass = -1;
        sb->BlockSize = -1;
        sb->FirstFreeBlock = -1;
        sb->NumOfFreeBlocks = -1;
        SBremove(sb);
        SBadd(EmptySBListHead, sb);
    }
}

SBdescriptor *GetAnEmptySB()
{
    if(EmptySBListHead->next == PMA_getPP(EmptySBListTail)){
        printf("Error: No empty SB anymore!\n");
        return NULL;
    }
    SBdescriptor *sb = (SBdescriptor *)PMA_getVA(EmptySBListHead->next);
    SBdescriptor *p = (SBdescriptor *)PMA_getVA(sb->prior);
    SBdescriptor *n = (SBdescriptor *)PMA_getVA(sb->next);
    p->next = PMA_getPP((void *)n);
    n->prior = PMA_getPP((void *)p);
    return sb;
}

SBdescriptor *GetContiguousSBs(int n)
{
    if(EmptySBListHead->next == PMA_getPP(EmptySBListTail)){
        printf("Error: No empty SB anymore!\n");
        return NULL;
    }
    SBdescriptor *start = (SBdescriptor *)PMA_getVA(EmptySBListHead->next);
    SBdescriptor *end = start;
    int cnt = 1;
    while(cnt < n){
        if(end->next == PMA_getPP(EmptySBListTail)){
            printf("Error: no %d contiguous SBs!\n", n);
            return NULL;
        }
        if(PMA_getVA(end->next) - (void *)end == SB_SIZE){
            end = (SBdescriptor *)PMA_getVA(end->next);
            cnt++;
        }
        else{
            start = (SBdescriptor *)PMA_getVA(end->next);
            end = start;
            cnt = 1;
        }
    }
    SBdescriptor *pre = (SBdescriptor *)PMA_getVA(start->prior);
    SBdescriptor *nxt = (SBdescriptor *)PMA_getVA(end->next);
    pre->next = PMA_getPP((void *)nxt);
    nxt->prior = PMA_getPP((void *)pre);
    return start;
}

void FreeSB(SBdescriptor *sb)
{
    if(EmptySBListHead->next == PMA_getPP(EmptySBListTail)
        || EmptySBListHead->next > PMA_getPP((void *)sb)){
        SBdescriptor *nxt = PMA_getVA(EmptySBListHead->next);
        sb->prior = PMA_getPP(EmptySBListHead);
        sb->next = PMA_getPP(nxt);
        EmptySBListHead->next = PMA_getPP((void *)sb);
        nxt->prior = PMA_getPP((void *)sb);
    }
    else{
        SBdescriptor *cur = PMA_getVA(EmptySBListHead->next);
        SBdescriptor *nxt = PMA_getVA(cur->next);
        while(nxt != EmptySBListTail && sb > nxt){
            cur = nxt;
            nxt = PMA_getVA(cur->next);
        }
        sb->prior = PMA_getPP((void *)cur);
        sb->next = PMA_getPP((void *)nxt);
        cur->next = PMA_getPP((void *)sb);
        nxt->prior = PMA_getPP((void *)sb);
    }
}

void FreeContiguousSBs(SBdescriptor *sb, int n)
{
    for(int i=0; i < n-1; i++)
        ((SBdescriptor *)((void *)sb + i * SB_SIZE))->next = PMA_getPP((void *)sb + (i+1) * SB_SIZE);
    for(int i=1; i < n; i++)
        ((SBdescriptor *)((void *)sb + i * SB_SIZE))->prior = PMA_getPP((void *)sb + (i-1) * SB_SIZE);
    
    if(EmptySBListHead->next == PMA_getPP(EmptySBListTail)
        || EmptySBListHead->next > PMA_getPP((void *)sb)){
        SBdescriptor *nxt = PMA_getVA(EmptySBListHead->next);
        sb->prior = PMA_getPP(EmptySBListHead);
        ((SBdescriptor *)((void *)sb + (n-1) * SB_SIZE))->next = PMA_getPP(nxt);
        EmptySBListHead->next = PMA_getPP((void *)sb);
        nxt->prior = PMA_getPP((void *)sb + (n-1) * SB_SIZE);
    }
    else{
        SBdescriptor *cur = PMA_getVA(EmptySBListHead->next);
        SBdescriptor *nxt = PMA_getVA(cur->next);
        while(nxt != EmptySBListTail && sb > nxt){
            cur = nxt;
            nxt = PMA_getVA(cur->next);
        }
        sb->prior = PMA_getPP((void *)cur);
        ((SBdescriptor *)((void *)sb + (n-1) * SB_SIZE))->next = PMA_getPP((void *)nxt);
        cur->next = PMA_getPP((void *)sb);
        nxt->prior = PMA_getPP((void *)sb + (n-1) * SB_SIZE);
    }
}

void SBremove(SBdescriptor *sb)
{
    SBdescriptor *pre = (SBdescriptor *)PMA_getVA(sb->prior);
    SBdescriptor *nxt = (SBdescriptor *)PMA_getVA(sb->next);
    pre->next = PMA_getPP((void *)nxt);
    nxt->prior = PMA_getPP((void *)pre);
}

void SBadd(SBdescriptor *header, SBdescriptor *sb)
{
    SBdescriptor *nxt = (SBdescriptor *)PMA_getVA(header->next);
    sb->prior = PMA_getPP((void *)header);
    sb->next = PMA_getPP((void *)nxt);
    header->next = PMA_getPP((void *)sb);
    nxt->prior = PMA_getPP((void *)sb);
}

SBdescriptor *FindAEmptySB(void)
{
    if(EmptySBListHead->next == PMA_getPP(EmptySBListTail)){
        printf("Error: No empty SB anymore!\n");
        return NULL;
    }
    return (SBdescriptor *)PMA_getVA(EmptySBListHead->next);
}

SBdescriptor *FindAPartialSB(int sizeclass)
{
    if(PartialSBListHead[sizeclass]->next == PMA_getPP(PartialSBListTail[sizeclass])){
        return NULL;
    }
    return (SBdescriptor *)PMA_getVA(PartialSBListHead[sizeclass]->next);
}

SBdescriptor *FindAFullSB(int sizeclass)
{
    if(FullSBListHead[sizeclass]->next == PMA_getPP(FullSBListTail[sizeclass])){
        return NULL;
    }
    return (SBdescriptor *)PMA_getVA(FullSBListHead[sizeclass]->next);
}