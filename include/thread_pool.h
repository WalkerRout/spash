#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <stdbool.h>
#include <stddef.h>

typedef void (*thread_func_t)(void *arg);

struct thread_pool;

struct thread_pool *tpool_new(size_t num);
void tpool_free(struct thread_pool *tp);
bool tpool_add_work(struct thread_pool *tp, thread_func_t func, void *arg);
void tpool_wait(struct thread_pool *tp);

#endif // _THREAD_POOL_H
