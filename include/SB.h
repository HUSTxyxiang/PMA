#ifndef SB_H
#define SB_H 1

#include "def.h"
#include "common.h"
#include "PP.h"
#include "sizeclass.h"

typedef struct{// The descriptor of a superblock
    
    int SizeClass;      // The size class it belongs to (from 0 to 40, -1 for empty SB)
    
    int BlockSize;      // The size of small blocks inside
    
    int FirstFreeBlock; // The index of the first free block
    
    int NumOfFreeBlocks;// The number of free blocks
    
    PP  prior;          // pointer to the prior SB in the SB list
    
    PP  next;           // pointer to the next SB in the SB list

} SBdescriptor;

void SBinit(SBdescriptor *sb, int sizeclass);

SBdescriptor *GetAnEmptySB();

SBdescriptor *GetContiguousSBs(int n);

void FreeSB(SBdescriptor *sb);

void FreeContiguousSBs(SBdescriptor *sb, int n);

void SBadd(SBdescriptor *header, SBdescriptor *sb);

void SBremove(SBdescriptor *sb);

SBdescriptor *FindAEmptySB(void);

SBdescriptor *FindAPartialSB(int sizeclass);

SBdescriptor *FindAFullSB(int sizeclass);

void AddBlockToSB(SBdescriptor *sb, void *block);

#endif