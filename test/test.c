#include "../include/PMA.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <libpmemobj.h>
#include <pthread.h>

#define MAXT 256

int SIZE;
int ITR;
int N;
int T;

PP p[MAXT][10000];
pthread_t tid[MAXT];
int num[MAXT];

void *test(void *arg)
{
    int k = *(int *)arg;
    int n = N / T;
    for(int itr=0; itr<ITR; itr++){
        for(int i=0; i<n; i++){
            p[k][i] = PMA_malloc(SIZE);
        }
        for(int i=0; i<n; i++){
            PMA_free(p[k][i]);
        }
    }
}

int main(int argc, char *argv[])
{
    if(argc != 5){
        printf("Usage : %s [SIZE] [N] [ITRS] [T]\n", argv[0]);
        printf("\tSIZE - size of each object\n");
        printf("\t   N - number of total objects\n");
        printf("\tITRS - repeating iterations\n");
        printf("\t   T - number of woking threads\n");
        
        return 0;
    }

    SIZE = atoi(argv[1]);
    N = atoi(argv[2]);
    ITR = atoi(argv[3]);
    T = atoi(argv[4]);

    
    for(int i=0; i < T; i++)
        num[i] = i;

    PMA_start("/mnt/pmem0/PMApool",4*1024*1024*1024UL);

    printf("Allocating %d objects (%dB) with %d threads, repeating for %d iterations\n", N, SIZE, T, ITR);
    
    double t1 = GetTimeInSeconds();

    for(int i = 0; i < T; i++)
        pthread_create( &tid[i], NULL, test, (void *)&num[i]);
    
    for(int i = 0; i < T; i++)
        pthread_join( tid[i], NULL);

    double t2 = GetTimeInSeconds();

    PMA_exit();

    printf("Execution time = %lfs\n", t2-t1);
    return 0;
}