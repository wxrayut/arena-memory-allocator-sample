//
// Author: wxrayut.
// Github: https://github.com/wxrayut.
// Date: 10-14-2025 [mm-dd-yyyy]
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <pthread.h>

#define DEBUG

#define ALIGNMENT 8
#define DEFAULT_BLOCK_SIZE 32

typedef struct _block {
    void *base;
    size_t size;
    size_t used;
    struct _block *next;
} block;

typedef struct arena {
    block *head;
    block *current;
    pthread_mutex_t mutex;
#if defined(DEBUG)
    int total_blocks;
    int total_allocated;
#endif
} arena;


static inline size_t align_up(size_t x) {
    return (x + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

static size_t next_block_size(block *prev, size_t min_size) {
    size_t prev_size = prev ? prev->size : DEFAULT_BLOCK_SIZE;
    size_t size = prev_size * 2;
    if (size < min_size) size = min_size;
    if (size < DEFAULT_BLOCK_SIZE) size = DEFAULT_BLOCK_SIZE;
    return size;
}

void arena_init(arena *a, size_t size) {
    a->head = NULL;
    a->current = NULL;
#if defined(DEBUG)
    a->total_blocks = 0;
    a->total_allocated = 0;
#endif
    pthread_mutex_init(&a->mutex, NULL);

    size = (size > DEFAULT_BLOCK_SIZE) ? size : DEFAULT_BLOCK_SIZE;

    block *b = malloc(sizeof(block));
    b->base = malloc(size);
    b->size = size;
    b->used = 0;
    b->next = NULL;

    a->head = b;
    a->current = b;

#if defined(DEBUG)
    a->total_blocks++;
#endif
}

void *arena_malloc(arena *a, size_t size) {
    pthread_mutex_lock(&a->mutex);

    size = align_up(size);

    block *b = a->current;
    size_t offset = align_up(b->used);
    size_t free_space = b->size - offset;

    if (free_space < size && free_space > 0) {
        size_t extra_needed = size - free_space;
        void *new_base = realloc(b->base, b->size + extra_needed);
        if (!new_base) {
            pthread_mutex_unlock(&a->mutex);
            return NULL;
        }
        b->base = new_base;
        b->size += extra_needed;
        free_space = size;
    } else if (free_space < size) {
        size_t block_size = next_block_size(b, size);
        block *new_block = malloc(sizeof(block));
        new_block->base = malloc(block_size);
        new_block->size = block_size;
        new_block->used = 0;
        new_block->next = NULL;

        b->next = new_block;
        a->current = new_block;
        b = new_block;
        offset = 0;

#if defined(DEBUG)
        a->total_blocks++;
#endif
    }

    void *ptr = b->base + offset;
    b->used = offset + size;

#if defined(DEBUG)
    a->total_allocated++;
#endif

    pthread_mutex_unlock(&a->mutex);
    return ptr;
}

void arena_free(arena *a) {
    if (!a) return;

    pthread_mutex_lock(&a->mutex);

    block *b = a->head;
    while (b) {
        block *next = b->next;
        free(b->base);
        free(b);
        b = next;
    }

    a->head = NULL;
    a->current = NULL;

#if defined(DEBUG)
    a->total_blocks = 0;
    a->total_allocated = 0;
#endif

    pthread_mutex_unlock(&a->mutex);
    pthread_mutex_destroy(&a->mutex);
}

void debug(arena *a) {
#if defined(DEBUG)
    block *b = a->head;
    int index = 0;
    printf("total blocks: %d\n", a->total_blocks);
    printf("total allocated: %d\n", a->total_allocated);
    while (b) {
        printf("block[%d] used=%zu / size=%zu / free=%zu\n",
               index, b->used, b->size, b->size - b->used);
        b = b->next;
        index++;
    }
#endif
}

void *worker1(void *arg) {
    arena *a = (arena *)arg;
    int *x = arena_malloc(a, 10);
    int *y = arena_malloc(a, 10);
    int *z = arena_malloc(a, 10);
    int *zz = arena_malloc(a, 2);
    return x;
}

void *worker2(void *arg) {
    arena *a = (arena *)arg;
    int *x = arena_malloc(a, 10);
    int *y = arena_malloc(a, 10);
    int *z = arena_malloc(a, 10);
    int *zz = arena_malloc(a, 8);
    return x;
}

int main() {
    arena a;
    arena_init(&a, 0);

    pthread_t t1, t2;
    pthread_create(&t1, NULL, worker1, &a);
    pthread_create(&t2, NULL, worker2, &a);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    debug(&a);
    arena_free(&a);

    return 0;
}
