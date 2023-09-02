#ifndef PMA_H
#define PMA_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include "def.h"
#include "common.h"
#include "PP.h"
#include "SB.h"
#include "TPMP.h"
#include "sizeclass.h"
#include "log.h"
#include "map.h"

void PMA_start(const char *heapfile, size_t size);

void HeapInit();

void PMA_exit();

PP PMA_malloc(size_t size);

void PMA_free(PP pp);

PP PMA_malloc_huge(size_t size);

void PMA_free_huge(PP pp);

#endif