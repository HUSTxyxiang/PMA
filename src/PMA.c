#include "../include/PMA.h"

void PMA_start(const char *heapfile, size_t size)
{
    printf("Starting the PMA...\n");
    
    int fd;
    if(access(heapfile, F_OK) != 0){
        printf("The heap file \"%s\" does not exist.\n", heapfile);
        fd = open(heapfile, O_RDWR | O_CREAT);
        ftruncate(fd, size);
        printf("The heap file \"%s\" is created.\n", heapfile);
    }
    else{
        fd = open(heapfile, O_RDWR);
        printf("The heap file \"%s\" is opened.\n", heapfile);
    }
    
    HeapSize = size;

    HeapBaseAddr = mmap(NULL, HeapSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    if(HeapBaseAddr == MAP_FAILED){
        printf("Failed to mmap() the heap file!\n");
        exit(0);
    }

    printf("The heap file is mapped at %p\n", HeapBaseAddr);

    EmptySBListHead = (SBdescriptor *)(HeapBaseAddr + 64);
    EmptySBListTail = EmptySBListHead + 1;

    for(int i=0; i < SIZE_CLASS_N; i++){
        PartialSBListHead[i] = EmptySBListTail + 1 + i;
    }

    for(int i=0; i < SIZE_CLASS_N; i++){
        PartialSBListTail[i] = PartialSBListHead[SIZE_CLASS_N-1] + 1 + i;
    }

    for(int i=0; i < SIZE_CLASS_N; i++){
        FullSBListHead[i] = PartialSBListTail[SIZE_CLASS_N-1] + 1 + i;
    }

    for(int i=0; i < SIZE_CLASS_N; i++){
        FullSBListTail[i] = FullSBListHead[SIZE_CLASS_N-1] + 1 + i;
    }

    LogBaseAddr = HeapBaseAddr + HEADER_REGION_LEN;

    TPMPBaseAddr = LogBaseAddr + LOG_REGION_LEN;

    map = malloc(sizeof(MAP));
    map->keys = (PP *)(TPMPBaseAddr + TPMP_REGION_LEN);
    map->values = (long long *)(TPMPBaseAddr + TPMP_REGION_LEN + sizeof(PP) * MAP_SIZE);
    map->size = MAP_SIZE;
    pthread_mutex_init(&(map->lock), NULL);
    

    SBBaseAddr = TPMPBaseAddr + TPMP_REGION_LEN + MAP_REGION_LEN;
    SBN = (size - HEADER_REGION_LEN - LOG_REGION_LEN - TPMP_REGION_LEN - MAP_REGION_LEN) / SB_SIZE;

    if( *(long long *)HeapBaseAddr != MAGIC){
        printf("Initializing the persistent heap...\n");
        HeapInit();
        printf("Done\n");
    }
    else{
        printf("Scanning the remaining logs...\n");
        int cnt = 0;
        LOG *log = LogBaseAddr;
        for(int i=0; i < LOG_N; i++){
            if(log[i].tid != 0 && log[i].state == RUNNING){
                cnt++;
                UNDO(&log[i]);
            }
        }
        if(cnt)
            printf("%d uncommitted transactions have been rolled back.\n", cnt);
        else
            printf("No uncommitted transaction found.\n");
    }

    for(int i=0; i < TPMP_N; i++){
        ((TPMP *)(TPMPBaseAddr + i * sizeof(TPMP)))->tid = 0;
    }

    for(int i=0; i < LOG_N; i++){
        ((LOG *)(LogBaseAddr + i * sizeof(LOG)))->tid = 0;
        ((LOG *)(LogBaseAddr + i * sizeof(LOG)))->state = COMMITTED;
        ((LOG *)(LogBaseAddr + i * sizeof(LOG)))->type = LOG_TYPE_NONE;
    }

    pthread_mutex_init(&LOCK_LOG, NULL);

    pthread_mutex_init(&LOCK_TPMP, NULL);
    
    pthread_mutex_init(&LOCK_EMPTY, NULL);

    for(int i=0; i < SIZE_CLASS_N; i++){
        pthread_mutex_init(&LOCK_PART[i], NULL);
    }

    for(int i=0; i < SIZE_CLASS_N; i++){
        pthread_mutex_init(&LOCK_FULL[i], NULL);
    }

    printf("PMA has started!\n");
}

void HeapInit()
{
    *(long long *)HeapBaseAddr = MAGIC;

    *(void **)(HeapBaseAddr + 8) = HeapBaseAddr;

    *(long long *)(HeapBaseAddr + 16) = HeapSize;

    EmptySBListHead->SizeClass = -1;
    EmptySBListHead->BlockSize = -1;
    EmptySBListHead->FirstFreeBlock = -1;
    EmptySBListHead->NumOfFreeBlocks = -1;
    EmptySBListHead->prior = NULLPP;
    EmptySBListHead->next = PMA_getPP(EmptySBListTail);

    EmptySBListTail->SizeClass = -1;
    EmptySBListTail->BlockSize = -1;
    EmptySBListTail->FirstFreeBlock = -1;
    EmptySBListTail->NumOfFreeBlocks = -1;
    EmptySBListTail->prior = PMA_getPP(EmptySBListHead);
    EmptySBListTail->next = NULLPP;

    for(int i=0; i < SIZE_CLASS_N; i++){
        PartialSBListHead[i]->SizeClass = -1;
        PartialSBListHead[i]->BlockSize = -1;
        PartialSBListHead[i]->NumOfFreeBlocks = -1;
        PartialSBListHead[i]->FirstFreeBlock = -1;
        PartialSBListHead[i]->prior = NULLPP;
        PartialSBListHead[i]->next = PMA_getPP(PartialSBListTail[i]);

        PartialSBListTail[i]->SizeClass = -1;
        PartialSBListTail[i]->BlockSize = -1;
        PartialSBListTail[i]->NumOfFreeBlocks = -1;
        PartialSBListTail[i]->FirstFreeBlock = -1;
        PartialSBListTail[i]->prior = PMA_getPP(PartialSBListHead[i]);
        PartialSBListTail[i]->next = NULLPP;

        FullSBListHead[i]->SizeClass = -1;
        FullSBListHead[i]->BlockSize = -1;
        FullSBListHead[i]->NumOfFreeBlocks = -1;
        FullSBListHead[i]->FirstFreeBlock = -1;
        FullSBListHead[i]->prior = NULLPP;
        FullSBListHead[i]->next = PMA_getPP(FullSBListTail[i]);

        FullSBListTail[i]->SizeClass = -1;
        FullSBListTail[i]->BlockSize = -1;
        FullSBListTail[i]->NumOfFreeBlocks = -1;
        FullSBListTail[i]->FirstFreeBlock = -1;
        FullSBListTail[i]->prior = PMA_getPP(FullSBListHead[i]);
        FullSBListTail[i]->next = NULLPP;
    }

    for(int i=0; i < SBN; i++){
        ((SBdescriptor *)(SBBaseAddr + i * SB_SIZE))->SizeClass = -1;
        ((SBdescriptor *)(SBBaseAddr + i * SB_SIZE))->BlockSize = -1;
        ((SBdescriptor *)(SBBaseAddr + i * SB_SIZE))->FirstFreeBlock = -1;
        ((SBdescriptor *)(SBBaseAddr + i * SB_SIZE))->NumOfFreeBlocks = -1;
    }
    for(int i=1; i < SBN-1; i++){
        ((SBdescriptor *)(SBBaseAddr + i * SB_SIZE))->prior = PMA_getPP(SBBaseAddr + (i-1) * SB_SIZE);
        ((SBdescriptor *)(SBBaseAddr + i * SB_SIZE))->next = PMA_getPP(SBBaseAddr + (i+1) * SB_SIZE);
    }

    ((SBdescriptor *)SBBaseAddr)->prior = PMA_getPP(EmptySBListHead);
    ((SBdescriptor *)SBBaseAddr)->next = PMA_getPP(SBBaseAddr + SB_SIZE);
    
    ((SBdescriptor *)(SBBaseAddr + (SBN-1) * SB_SIZE))->prior = PMA_getPP(SBBaseAddr + (SBN-2) * SB_SIZE);
    ((SBdescriptor *)(SBBaseAddr + (SBN-1) * SB_SIZE))->next = PMA_getPP(EmptySBListTail);

    EmptySBListHead->next = PMA_getPP(SBBaseAddr);
    EmptySBListTail->prior = PMA_getPP(SBBaseAddr + (SBN-1) * SB_SIZE);

    for(int i=0; i < TPMP_N; i++){
        ((TPMP *)(TPMPBaseAddr + i * sizeof(TPMP)))->tid = 0;
        for(int j=0; j < SIZE_CLASS_N; j++){
            ((TPMP *)(TPMPBaseAddr + i * sizeof(TPMP)))->freeLists[j].header = NULLPP;
            ((TPMP *)(TPMPBaseAddr + i * sizeof(TPMP)))->freeLists[j].length = 0;
        }
    }

    for(int i=0; i < LOG_N; i++){
        ((LOG *)(LogBaseAddr + i * sizeof(LOG)))->tid = 0;
        ((LOG *)(LogBaseAddr + i * sizeof(LOG)))->state = COMMITTED;
        ((LOG *)(LogBaseAddr + i * sizeof(LOG)))->type = LOG_TYPE_NONE;
    }

    for(int i=0; i < map->size; i++)
        map->keys[i] = 0;

    PMA_persist(HeapBaseAddr, HEADER_REGION_LEN + LOG_REGION_LEN + TPMP_REGION_LEN + MAP_REGION_LEN);

    for(int i=0; i < SBN; i++)
        PMA_persist(SBBaseAddr + i * SB_SIZE, sizeof(SBdescriptor));

    PMA_barrier();
}


void PMA_exit()
{
    TPMP *tpmp = TPMPBaseAddr;
    for(int i=0; i < TPMP_N; i++){
        if(tpmp[i].tid){
            for(int sc=0; sc < SIZE_CLASS_N; sc++)
                FlushTPMP(&tpmp[i], sc);
        }
    }

    munmap(HeapBaseAddr, HeapSize);

    pthread_mutex_destroy(&LOCK_LOG);

    pthread_mutex_destroy(&LOCK_TPMP);
    
    pthread_mutex_destroy(&LOCK_EMPTY);

    for(int i=0; i < SIZE_CLASS_N; i++){
        pthread_mutex_destroy(&LOCK_PART[i]);
    }

    for(int i=0; i < SIZE_CLASS_N; i++){
        pthread_mutex_destroy(&LOCK_FULL[i]);
    }

    pthread_mutex_destroy(&(map->lock));
    free(map);

    printf("PMA is exited normally.\n");
}

PP PMA_malloc(size_t size)
{
    //printf("Get into PMA_malloc()\n");

    if(size <= 0 || size >= HeapSize){
        printf("Illegal allocation size!\n");
        return NULLPP;
    }

    if(size > SB_SIZE / 2){
        return PMA_malloc_huge(size);
    }

    int sc = getSizeClass(size);

    pthread_t tid = pthread_self();

    TPMP *tpmp = findTPMP(tid);
    if(tpmp == NULL)
        tpmp = createTPMP(tid);
    if(tpmp == NULL){
        printf("Error: Failed to create a TPMP!\n");
        exit(0);
    }

    LOG *log = findLog(tid);
    if(log == NULL)
        log = occupyLog(tid);
    if(log == NULL){
        printf("Error: Failed to create a LOG!\n");
        exit(0);
    }

    /*
    if(tpmp->freeLists[sc].length <= 0){
        SBdescriptor *sb = GetAnEmptySB();
        sb->SizeClass = sc;
        sb->BlockSize = getSize(sc);
        int n = (SB_SIZE - sizeof(SBdescriptor)) / (sb->BlockSize);
        for(int i=0; i < n-1; i++){
            *(PP *)((void *)sb + sizeof(SBdescriptor) + i * (sb->BlockSize))
            = PMA_getPP((void *)sb + sizeof(SBdescriptor) + (i+1) * (sb->BlockSize));
        }
        *(PP *)((void *)sb + sizeof(SBdescriptor) + (n-1) * (sb->BlockSize)) = NULLPP;
        tpmp->freeLists[sc].header = PMA_getPP((void *)sb + sizeof(SBdescriptor));
        tpmp->freeLists[sc].length = n;
        sb->FirstFreeBlock = -1;
        sb->NumOfFreeBlocks = 0;

    }
    */

    if(tpmp->freeLists[sc].length <= 0){
        pthread_mutex_lock(&LOCK_PART[sc]);
        SBdescriptor *sb = FindAPartialSB(sc);
        if(sb == NULL){
            pthread_mutex_unlock(&LOCK_PART[sc]);    
            pthread_mutex_lock(&LOCK_EMPTY);
            sb = FindAEmptySB();
            if(sb == NULL){
                printf("Memory exhausted!\n");
                pthread_mutex_unlock(&LOCK_EMPTY);
                return NULLPP;
            }

            SBinit(sb, sc);
            
            LOG_ADD_FILL_FLUSH(log, sb, tpmp, sc);

            TX_BEGIN(log);

            FillTPMP(sb, tpmp, sc);

            TX_COMMIT(log);

            pthread_mutex_lock(&LOCK_FULL[sc]);
            
            LOG_ADD_MOVSB(
                log,
                sb,
                (SBdescriptor *)PMA_getVA(sb->prior),
                (SBdescriptor *)PMA_getVA(sb->next),
                FullSBListHead[sc],
                (SBdescriptor *)PMA_getVA(FullSBListHead[sc]->next)
            );
            
            TX_BEGIN(log);

            SBremove(sb);
            pthread_mutex_unlock(&LOCK_EMPTY);
            
            SBadd(FullSBListHead[sc], sb);
            pthread_mutex_unlock(&LOCK_FULL[sc]);

            TX_COMMIT(log);
        }
        else{
            LOG_ADD_FILL_FLUSH(log, sb, tpmp, sc);

            TX_BEGIN(log);

            FillTPMP(sb, tpmp, sc);

            TX_COMMIT(log);

            pthread_mutex_lock(&LOCK_FULL[sc]);
            
            LOG_ADD_MOVSB(
                log,
                sb,
                (SBdescriptor *)PMA_getVA(sb->prior),
                (SBdescriptor *)PMA_getVA(sb->next),
                FullSBListHead[sc],
                (SBdescriptor *)PMA_getVA(FullSBListHead[sc]->next)
            );

            TX_BEGIN(log);

            SBremove(sb);
            pthread_mutex_unlock(&LOCK_PART[sc]);
            
            SBadd(FullSBListHead[sc], sb);
            pthread_mutex_unlock(&LOCK_FULL[sc]);

            TX_COMMIT(log);
        }
    }

    LOG_ADD_ALLOC_FREE(log, tpmp, sc);
    
    TX_BEGIN(log);

    PP pp = AllocFromTPMP(tpmp, sc);

    TX_COMMIT(log);

    //printf("Finished PMA_malloc()\n");
    return pp;
}

void PMA_free(PP pp)
{
    //printf("Get into PMA_free()\n");

    if(((pp >> SHIFT_N) << SHIFT_N) == pp){
        PMA_free_huge(pp);
        return;
    }
    
    pthread_t tid = pthread_self();
    TPMP *tpmp = findTPMP(tid);
    LOG *log = findLog(tid);
    
    SBdescriptor *sb = (SBdescriptor *)PMA_getVA((pp >> SHIFT_N) << SHIFT_N);
    int sc = sb->SizeClass;
    
    LOG_ADD_ALLOC_FREE(log, tpmp, sc);

    TX_BEGIN(log);

    FreeToTPMP(tpmp, sc, pp);

    TX_COMMIT(log);

    //printf("Finished PMA_free()\n");
}

PP PMA_malloc_huge(size_t size)
{
    int n = 0;
    if(((size >> SHIFT_N) << SHIFT_N) == size)
        n = size >> SHIFT_N;
    else
        n = (size >> SHIFT_N) + 1;
    pthread_mutex_lock(&LOCK_EMPTY);
    SBdescriptor *startSB = GetContiguousSBs(n);
    pthread_mutex_unlock(&LOCK_EMPTY);
    PP res = PMA_getPP((void *)startSB);
    mapInsert(map, res, n);
    return res;
}

void PMA_free_huge(PP pp)
{
    int n = mapFind(map, pp);
    if(n){
        pthread_mutex_lock(&LOCK_EMPTY);
        FreeContiguousSBs((SBdescriptor *)PMA_getVA(pp), n);
        pthread_mutex_unlock(&LOCK_EMPTY);
        mapDelete(map, pp);
    }
}