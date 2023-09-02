#ifndef LOG_H
#define LOG_H 1

#include "def.h"
#include "common.h"
#include "TPMP.h"
#include <pthread.h>

typedef struct{
    PP addr;
    long long len;
    char value[32];
} LOGENTRY;

typedef struct{
    pthread_t tid;
    int state;
    int type;
    LOGENTRY entry[5];
} LOG;


LOG *occupyLog(pthread_t TID);

LOG *findLog(pthread_t TID);

void TX_BEGIN(LOG *log);

void TX_COMMIT(LOG *log);

void LOG_ADD_MOVSB(
    LOG *log,
    SBdescriptor *sb,
    SBdescriptor *oldprior,
    SBdescriptor *oldnext,
    SBdescriptor *newprior,
    SBdescriptor *newnext
);

void LOG_ADD_FILL_FLUSH(
    LOG *log,
    SBdescriptor *sb,
    TPMP *tpmp,
    int sizeclass
);

void LOG_ADD_ALLOC_FREE(
    LOG *log,
    TPMP *tpmp,
    int sizeclass
);

void UNDO(LOG *log);

void UNDO_MOVSB(LOG *log);

void UNDO_FILL_FLUSH(LOG *log);

void UNDO_ALLOC_FREE(LOG *log);

#endif

