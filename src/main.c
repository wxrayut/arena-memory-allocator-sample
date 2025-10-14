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

typedef struct arena {
    unsigned char  *base;
    size_t          size;
    size_t          used;
    pthread_mutex_t mutex;
#if defined(DEBUG)
    int total_allocated;
#endif
} arena;

void arena_init(arena *a, size_t size) {

    a->base = malloc(size);
    a->size = size;
    a->used = 0;
#if defined(DEBUG)
    a->total_allocated = 0;
#endif
    pthread_mutex_init(&a->mutex, NULL);
}

void *arena_malloc(arena *a, size_t size) {

    pthread_mutex_lock(&a->mutex);

    if (a->used + size > a->size) {
        pthread_mutex_unlock(&a->mutex);
        return NULL;
    }

    void *ptr = a->base + a->used;
    a->used += size;

#if defined(DEBUG)
    a->total_allocated++;
#endif

    pthread_mutex_unlock(&a->mutex);

    return ptr;
}

void arena_free(arena *a) {

    if (!a || !a->base) {
        return;
    }

    pthread_mutex_destroy(&a->mutex);
    free(a->base);

    a->base = NULL;
    a->size = 0;
    a->used = 0;
#if defined(DEBUG)
    a->total_allocated = 0;
#endif
}

void *worker1(void *arg) {
    printf("worker 1 is working . . .\n");
    arena *a = (arena *)arg;
    int   *x = arena_malloc(a, 10);
    int   *z = arena_malloc(a, 10);
    return x;
}

void *worker2(void *arg) {
    printf("worker 2 is working . . .\n");
    arena *a = (arena *)arg;
    int   *x = arena_malloc(a, 10);
    return x;
}

int main(int argc, const char *argv[]) {

    arena a;
    arena_init(&a, 32);

    pthread_t thread1, thread2;

    pthread_create(&thread1, NULL, worker1, &a);
    pthread_create(&thread2, NULL, worker2, &a);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

#if defined(DEBUG)
    printf("initialize size: %zu\n", a.size);
    printf("used: %zu\n", a.used);
    if (a.used != 0) {
        printf("free: %zu\n", a.size - a.used);
    }
    printf("total allocated: %d\n", a.total_allocated);
#endif

    arena_free(&a);

    return 0;
}
