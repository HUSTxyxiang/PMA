#include "../include/PP.h"
#include "../include/common.h"

PP PMA_getPP(void *va)
{
    return va - HeapBaseAddr;
}

void *PMA_getVA(PP pp)
{
    return pp + HeapBaseAddr;
}