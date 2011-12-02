/*
 * cache.h
 */

#include "csapp.h"

#define MAX_CACHE_SIZE (1 << 20)
typedef struct Cache_Block Cache_Block;
pthread_mutex_t mutex_lock;
typedef struct {
    char request_str[MAXLINE]; 
    char host_str[MAXLINE];
} Request;

typedef struct {
    char header[MAXLINE];
    char *content;
    int content_size;
} Response;

struct Cache_Block{
    unsigned long key;
    char *content;
    int content_size;
    time_t time_stamp;
    Cache_Block *next_block;
    Cache_Block *pre_block;
};

typedef struct {
    int size; 
    Cache_Block *head; 
    Cache_Block *tail;
} Cache;

Cache cache;
void init_cache();
void clean_cache();
Cache_Block* is_in_cache(unsigned long key);
Cache_Block *build_cache_block(Request *request, Response *response);
void add_cache_block(Cache_Block *cache_block);
void delete_link(Cache_Block *cache_block);
void free_cache_block(Cache_Block *cache_block);
void delete_cache_block(Cache_Block *cache_block);
int check_cache(Request *request, Response *response);
void save_to_cache(Request *request, Response *response);
