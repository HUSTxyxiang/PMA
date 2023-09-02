#include "../include/log.h"
#include "../include/SB.h"
#include "../include/TPMP.h"

LOG *occupyLog(pthread_t TID)
{
    pthread_mutex_lock(&LOCK_LOG);
    LOG *logs = LogBaseAddr;
    int hash = (long long)TID % LOG_N;
    if(logs[hash].tid == 0){
        logs[hash].tid = TID;
        PMA_persist((void *)&(logs[hash].tid), sizeof(pthread_t));
        PMA_barrier();
        pthread_mutex_unlock(&LOCK_LOG);
        return &logs[hash];
    }
    for(int i = hash+1; i != hash; i = (i+1) % LOG_N){
        if(logs[i].tid == 0){
            logs[i].tid = TID;
            PMA_persist((void *)&(logs[i].tid), sizeof(pthread_t));
            PMA_barrier();
            pthread_mutex_unlock(&LOCK_LOG);
            return &logs[i];
        }
    }
    pthread_mutex_unlock(&LOCK_LOG);
    return NULL;
}

LOG *findLog(pthread_t TID)
{
    LOG *logs = LogBaseAddr;
    int hash = (long long)TID % LOG_N;
    if(logs[hash].tid == TID){
        return &logs[hash];
    }
    for(int i = hash+1; i != hash; i = (i+1) % LOG_N){
        if(logs[i].tid == TID)
            return &logs[i];
    }
    return NULL;
}

void TX_BEGIN(LOG *log)
{
    log->state = RUNNING;
    PMA_persist((void *)&(log->state), sizeof(log->state));
    PMA_barrier();
}

void TX_COMMIT(LOG *log)
{
    log->state = COMMITTED;
    PMA_persist((void *)&(log->state), sizeof(log->state));
    PMA_barrier();
}

void LOG_ADD_MOVSB(
    LOG *log,
    SBdescriptor *sb,
    SBdescriptor *oldprior,
    SBdescriptor *oldnext,
    SBdescriptor *newprior,
    SBdescriptor *newnext
)
{
    log->type = LOG_TYPE_MOVSB;

    log->entry[0].addr = PMA_getPP((void *)sb);
    log->entry[0].len = sizeof(SBdescriptor);
    memcpy((void *)(log->entry[0].value), (void *)sb, sizeof(SBdescriptor));
    
    log->entry[1].addr = PMA_getPP((void *)oldprior);
    log->entry[1].len = sizeof(SBdescriptor);
    memcpy((void *)(log->entry[1].value), (void *)oldprior, sizeof(SBdescriptor));
    
    log->entry[2].addr = PMA_getPP((void *)oldnext);
    log->entry[2].len = sizeof(SBdescriptor);
    memcpy((void *)(log->entry[2].value), (void *)oldnext, sizeof(SBdescriptor));
    
    log->entry[3].addr = PMA_getPP((void *)newprior);
    log->entry[3].len = sizeof(SBdescriptor);
    memcpy((void *)(log->entry[3].value), (void *)newprior, sizeof(SBdescriptor));
    
    log->entry[4].addr = PMA_getPP((void *)newnext);
    log->entry[4].len = sizeof(SBdescriptor);
    memcpy((void *)(log->entry[4].value), (void *)newnext, sizeof(SBdescriptor));
    
    PMA_persist((void *)log, sizeof(LOG));
    PMA_barrier();
}

void LOG_ADD_FILL_FLUSH(
    LOG *log,
    SBdescriptor *sb,
    TPMP *tpmp,
    int sizeclass
)
{
    log->type = LOG_TYPE_FILL_FLUSH;
    
    log->entry[0].addr = PMA_getPP((void *)sb);
    log->entry[0].len = sizeof(SBdescriptor);
    memcpy((void *)(log->entry[0].value), (void *)sb, sizeof(SBdescriptor));
    
    log->entry[1].addr = PMA_getPP((void *)&(tpmp->freeLists[sizeclass]));
    log->entry[1].len = sizeof(tpmp->freeLists[sizeclass]);
    memcpy((void *)(log->entry[1].value), (void *)&(tpmp->freeLists[sizeclass]), sizeof(tpmp->freeLists[sizeclass]));

    PMA_persist((void *)log, sizeof(LOG));
    PMA_barrier();
}

void LOG_ADD_ALLOC_FREE(
    LOG *log,
    TPMP *tpmp,
    int sizeclass
)
{
    log->type = LOG_TYPE_ALLOC_FREE;
    
    log->entry[0].addr = PMA_getPP((void *)&(tpmp->freeLists[sizeclass]));
    log->entry[0].len = sizeof(tpmp->freeLists[sizeclass]);
    memcpy((void *)(log->entry[0].value), (void *)&(tpmp->freeLists[sizeclass]), sizeof(tpmp->freeLists[sizeclass]));
    
    PMA_persist((void *)log, sizeof(LOG));
    PMA_barrier();
}

void UNDO(LOG *log)
{
    switch(log->type){
        case LOG_TYPE_MOVSB:
            UNDO_MOVSB(log);
            break;
        case LOG_TYPE_FILL_FLUSH:
            UNDO_FILL_FLUSH(log);
            break;
        case LOG_TYPE_ALLOC_FREE:
            UNDO_ALLOC_FREE(log);
            break;
        default:
            break;
    }
    TX_COMMIT(log);
}

void UNDO_MOVSB(LOG *log)
{
    SBdescriptor *sb = PMA_getVA(log->entry[0].addr);
    memcpy((void *)sb, (void *)(log->entry[0].value), log->entry[0].len);
    PMA_persist((void *)sb, log->entry[0].len);

    SBdescriptor *oldprior = PMA_getVA(log->entry[1].addr);
    memcpy((void *)oldprior, (void *)(log->entry[1].value), log->entry[1].len);
    PMA_persist((void *)oldprior, log->entry[1].len);

    SBdescriptor *oldnext = PMA_getVA(log->entry[2].addr);
    memcpy((void *)oldnext, (void *)(log->entry[2].value), log->entry[2].len);
    PMA_persist((void *)oldnext, log->entry[2].len);

    SBdescriptor *newprior = PMA_getVA(log->entry[3].addr);
    memcpy((void *)newprior, (void *)(log->entry[3].value), log->entry[3].len);
    PMA_persist((void *)newprior, log->entry[3].len);

    SBdescriptor *newnext = PMA_getVA(log->entry[4].addr);
    memcpy((void *)newnext, (void *)(log->entry[4].value), log->entry[4].len);
    PMA_persist((void *)newnext, log->entry[4].len);

    PMA_barrier();
}

void UNDO_FILL_FLUSH(LOG *log)
{
    SBdescriptor *sb = PMA_getVA(log->entry[0].addr);
    memcpy((void *)sb, (void *)(log->entry[0].value), log->entry[0].len);
    PMA_persist((void *)sb, log->entry[0].len);

    void *TPMPlist = PMA_getVA(log->entry[1].addr);
    memcpy(TPMPlist, (void *)(log->entry[1].value), log->entry[1].len);
    PMA_persist(TPMPlist, log->entry[1].len);

    PMA_barrier();
}

void UNDO_ALLOC_FREE(LOG *log)
{
    void *addr = PMA_getVA(log->entry[0].addr);
    size_t len = log->entry[0].len;
    memcpy(addr, log->entry[0].value, len);
    PMA_persist(addr, len);
    PMA_barrier();
}