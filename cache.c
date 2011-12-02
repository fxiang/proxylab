/*
 * cache.c
 */

#include "cache.h"
#include "crc32.h"

Cache_Block* is_in_cache(unsigned long key) {
    Cache_Block *cache_block = cache.head;
    while (cache_block != NULL) {
        if (cache_block->key == key) {
            #ifdef DEBUG
            printf("in cache ! key: %lu\n", key);
            #endif
            return cache_block;
        }
        cache_block = cache_block->next_block;
    }
    return NULL;
}

Cache_Block *build_cache_block(Request *request, Response *response) {
    Cache_Block *cache_block;
    cache_block = (Cache_Block*)Malloc(sizeof(Cache_Block));
    cache_block->time_stamp = time(NULL);
    cache_block->key = 
      crc32( request->request_str, strlen(request->request_str));
    cache_block->content_size = response->content_size;
    cache_block->content = response->content;
    cache_block->next_block = NULL;
    cache_block->pre_block = NULL;
    #ifdef DEBUG
    printf("time_stamp:%ld\n", cache_block->time_stamp);
    printf("hash key:%lu\n", cache_block->key);
    #endif
    return cache_block;
}

void add_cache_block(Cache_Block *cache_block) {
    if (cache_block == NULL) {
        fprintf(stderr, "error in add_cache_block, try to add empty block\n");
        abort();
    }
    if (cache.head == NULL) {
        cache.head = cache_block;
        cache.tail = cache_block;
    } else {
        if (cache.head->pre_block == NULL) {
            cache.head->pre_block = cache_block;
            cache_block->next_block = cache.head;
            cache.head = cache_block;
        } else {
            fprintf(stderr, "add_cache_block error\n");
            abort();
        }
    }
    cache.size += cache_block->content_size;
}

void delete_link(Cache_Block *cache_block) {
    Cache_Block *pre_block = cache_block->pre_block;
    Cache_Block *next_block = cache_block->next_block;
    if (cache_block == NULL) {
        fprintf(stderr, "delete a empty block\n");
    }
    if (pre_block != NULL) {
        pre_block->next_block = next_block;
    } else {
        if (cache.head == cache_block) {
            cache.head = next_block; 
            if (cache.head != NULL)
                cache.head->pre_block = NULL;
        } else {
            fprintf(stderr, "delete_cache_block error, delete head block\n");
        }
    }

    if (next_block != NULL) {
        next_block->pre_block = pre_block;
    } else {
        if (cache.tail == cache_block) {
            cache.tail = pre_block;
            if (cache.tail != NULL)
                cache.tail->next_block = NULL; 
        } else {
            fprintf(stderr, "delete_cache_block error, delete tail block\n");
        }
    }
    cache_block->pre_block = NULL;
    cache_block->next_block = NULL;
}

void free_cache_block(Cache_Block *cache_block) {
    free(cache_block->content);
    free(cache_block);
}

void delete_cache_block(Cache_Block *cache_block) {
    delete_link(cache_block);
    free_cache_block(cache_block);
    cache.size -= cache_block->content_size;
}

int check_cache(Request *request, Response *response) {

    /* entering critical section lock ! */
    pthread_mutex_lock(&mutex_lock);
    unsigned long key = crc32(request->request_str, 
            strlen(request->request_str));    
    Cache_Block *cache_block = NULL;
    if ((cache_block = is_in_cache(key)) != NULL) {
        if (cache_block == NULL) {
            printf("return NULL block\n");
            abort();
        }
        response->content = cache_block->content; 
        response->content_size = cache_block->content_size;
        cache_block->time_stamp = time(NULL); 

        delete_link(cache_block);
        add_cache_block(cache_block);
        pthread_mutex_unlock(&mutex_lock);
        return 1;
    } else {
        pthread_mutex_unlock(&mutex_lock);
        return 0;
    }
}

void save_to_cache(Request *request, Response *response) {
    #ifdef DEBUG
    printf("save_to_cache function");
    #endif
    Cache_Block *cache_block;
    cache_block = build_cache_block(request, response);

    /* entering critical area, add lock ! */
    pthread_mutex_lock(&mutex_lock);

    #ifdef DEBUG
    printf("thread in critical area\n");
    printf("length: %d\n", response->content_size);
    #endif

    if (cache.size + response->content_size > MAX_CACHE_SIZE) {
        while (cache.size + response->content_size > MAX_CACHE_SIZE) {
            delete_cache_block(cache.tail);
        }
    }
    add_cache_block(cache_block);
    pthread_mutex_unlock(&mutex_lock);

    #ifdef DEBUG
    printf("leave save_to_cache\n");
    #endif
}

/*
 * init_cache - initialize global variable cache
 */
void init_cache() {
    cache.head = NULL;
    cache.tail = NULL;
    cache.size = 0;
}

/*
 * clean_cache - free the cache memory(not impelmented)
 */
void clean_cache() {

}
