#ifndef DEF_H
#define DEF_H 1

#define SIZE_CLASS_N 41

#define SB_SIZE 16384LL
#define SHIFT_N 14

#define HEADER_REGION_LEN (128*1024LL)

#define LOG_REGION_LEN (256*1024LL)

#define TPMP_REGION_LEN (640*1024LL)

#define MAP_REGION_LEN (8*1024*1024LL)

#define LOG_N 1024

#define TPMP_N 986

#define MAGIC 0x20230530afdabced

#define NULLPP 0

#define CACHE_LINE_SIZE 64

#define LOG_TYPE_NONE 0
#define LOG_TYPE_MOVSB 1
#define LOG_TYPE_FILL_FLUSH 2
#define LOG_TYPE_ALLOC_FREE 3

#define RUNNING 1
#define COMMITTED 2

#define MAP_SIZE (MAP_REGION_LEN >> 4)

#endif