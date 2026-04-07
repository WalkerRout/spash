#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <stdbool.h>
#include <stddef.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <pthread.h>
#endif

typedef void (*thread_func_t)(void *arg);

struct thread_pool {
  struct work *work_first;
  struct work *work_last;
#ifdef _WIN32
  CRITICAL_SECTION work_queue_mutex;
  CONDITION_VARIABLE worker_cond;
  CONDITION_VARIABLE working_cond;
#else
  pthread_mutex_t work_queue_mutex;
  pthread_cond_t worker_cond;
  pthread_cond_t working_cond;
#endif
  size_t working_cnt;
  size_t thread_cnt;
  bool stop;
};

void thread_pool_init(struct thread_pool *tp, size_t num);
void thread_pool_free(struct thread_pool *tp);
bool thread_pool_add_work(
  struct thread_pool *tp,
  thread_func_t func,
  void *arg
);
void thread_pool_wait(struct thread_pool *tp);

#endif // _THREAD_POOL_H
