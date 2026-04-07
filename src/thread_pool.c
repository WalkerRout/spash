#include "thread_pool.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef _WIN32
  #include <process.h>
#endif

struct work {
  thread_func_t func;
  void *arg;
  struct work *next;
};

static struct work *work_init(thread_func_t func, void *arg);
static void work_free(struct work *work);
static struct work *work_get(struct thread_pool *tp);

#ifdef _WIN32
static unsigned __stdcall worker(void *arg);
#else
static void *worker(void *arg);
#endif

void thread_pool_init(struct thread_pool *tp, size_t num) {
  assert(tp);
  if (num == 0)
    num = 2;

  tp->thread_cnt = num;
  tp->working_cnt = 0;
  tp->stop = false;
  tp->work_first = NULL;
  tp->work_last = NULL;

#ifdef _WIN32
  InitializeCriticalSection(&tp->work_queue_mutex);
  InitializeConditionVariable(&tp->worker_cond);
  InitializeConditionVariable(&tp->working_cond);
#else
  pthread_mutex_init(&tp->work_queue_mutex, NULL);
  pthread_cond_init(&tp->worker_cond, NULL);
  pthread_cond_init(&tp->working_cond, NULL);
#endif

  for (size_t i = 0; i < num; i++) {
#ifdef _WIN32
    uintptr_t handle = _beginthreadex(NULL, 0, worker, tp, 0, NULL);
    CloseHandle((HANDLE)handle);
#else
    pthread_t thread;
    pthread_create(&thread, NULL, worker, tp);
    pthread_detach(thread);
#endif
  }
}

void thread_pool_free(struct thread_pool *tp) {
  assert(tp);

  {
#ifdef _WIN32
    EnterCriticalSection(&tp->work_queue_mutex);
#else
    pthread_mutex_lock(&tp->work_queue_mutex);
#endif
    struct work *work = tp->work_first;
    while (work != NULL) {
      struct work *work_next = work->next;
      work_free(work);
      work = work_next;
    }
    tp->work_first = NULL;
    tp->work_last = NULL;
    tp->stop = true;

#ifdef _WIN32
    WakeAllConditionVariable(&tp->worker_cond);
    LeaveCriticalSection(&tp->work_queue_mutex);
#else
    pthread_cond_broadcast(&tp->worker_cond);
    pthread_mutex_unlock(&tp->work_queue_mutex);
#endif
  }

  thread_pool_wait(tp);

#ifdef _WIN32
  DeleteCriticalSection(&tp->work_queue_mutex);
#else
  pthread_mutex_destroy(&tp->work_queue_mutex);
  pthread_cond_destroy(&tp->worker_cond);
  pthread_cond_destroy(&tp->working_cond);
#endif
}

bool thread_pool_add_work(
  struct thread_pool *tp,
  thread_func_t func,
  void *arg
) {
  assert(tp);

  struct work *work = work_init(func, arg);
  if (work == NULL)
    return false;

  {
#ifdef _WIN32
    EnterCriticalSection(&tp->work_queue_mutex);
#else
    pthread_mutex_lock(&tp->work_queue_mutex);
#endif
    if (tp->work_first == NULL) {
      tp->work_first = work;
      tp->work_last = tp->work_first;
    } else {
      tp->work_last->next = work;
      tp->work_last = work;
    }
#ifdef _WIN32
    WakeAllConditionVariable(&tp->worker_cond);
    LeaveCriticalSection(&tp->work_queue_mutex);
#else
    pthread_cond_broadcast(&tp->worker_cond);
    pthread_mutex_unlock(&tp->work_queue_mutex);
#endif
  }

  return true;
}

void thread_pool_wait(struct thread_pool *tp) {
  assert(tp);

  {
#ifdef _WIN32
    EnterCriticalSection(&tp->work_queue_mutex);
#else
    pthread_mutex_lock(&tp->work_queue_mutex);
#endif
    for (;;) {
      bool still_waiting = tp->work_first != NULL
        || (!tp->stop && tp->working_cnt != 0)
        || (tp->stop && tp->thread_cnt != 0);
      if (still_waiting) {
#ifdef _WIN32
        SleepConditionVariableCS(
          &tp->working_cond,
          &tp->work_queue_mutex,
          INFINITE
        );
#else
        pthread_cond_wait(&tp->working_cond, &tp->work_queue_mutex);
#endif
      } else {
        break;
      }
    }
#ifdef _WIN32
    LeaveCriticalSection(&tp->work_queue_mutex);
#else
    pthread_mutex_unlock(&tp->work_queue_mutex);
#endif
  }
}

static struct work *work_init(thread_func_t func, void *arg) {
  if (func == NULL)
    return NULL;
  struct work *work = malloc(sizeof(*work));
  work->func = func;
  work->arg = arg;
  work->next = NULL;
  return work;
}

static void work_free(struct work *work) {
  free(work);
}

static struct work *work_get(struct thread_pool *tp) {
  assert(tp);

  struct work *work = tp->work_first;
  if (work == NULL)
    return NULL;

  if (work->next == NULL) {
    tp->work_first = NULL;
    tp->work_last = NULL;
  } else {
    tp->work_first = work->next;
  }

  return work;
}

#ifdef _WIN32
static unsigned __stdcall worker(void *arg) {
#else
static void *worker(void *arg) {
#endif
  assert(arg != NULL);
  struct thread_pool *tp = arg;

  for (;;) {
    struct work *work;
    {
#ifdef _WIN32
      EnterCriticalSection(&tp->work_queue_mutex);
      while (tp->work_first == NULL && !tp->stop)
        SleepConditionVariableCS(
          &tp->worker_cond,
          &tp->work_queue_mutex,
          INFINITE
        );
      if (tp->stop)
        break;
      work = work_get(tp);
      tp->working_cnt++;
      LeaveCriticalSection(&tp->work_queue_mutex);
#else
      pthread_mutex_lock(&tp->work_queue_mutex);
      while (tp->work_first == NULL && !tp->stop)
        pthread_cond_wait(&tp->worker_cond, &tp->work_queue_mutex);
      if (tp->stop)
        break;
      work = work_get(tp);
      tp->working_cnt++;
      pthread_mutex_unlock(&tp->work_queue_mutex);
#endif
    }

    if (work != NULL) {
      work->func(work->arg);
      work_free(work);
    }

    {
#ifdef _WIN32
      EnterCriticalSection(&tp->work_queue_mutex);
      tp->working_cnt--;
      if (!tp->stop && tp->working_cnt == 0 && tp->work_first == NULL) {
        WakeConditionVariable(&tp->working_cond);
      }
      LeaveCriticalSection(&tp->work_queue_mutex);
#else
      pthread_mutex_lock(&tp->work_queue_mutex);
      tp->working_cnt--;
      if (!tp->stop && tp->working_cnt == 0 && tp->work_first == NULL) {
        pthread_cond_signal(&tp->working_cond);
      }
      pthread_mutex_unlock(&tp->work_queue_mutex);
#endif
    }
  }

  tp->thread_cnt--;
#ifdef _WIN32
  WakeConditionVariable(&tp->working_cond);
  LeaveCriticalSection(&tp->work_queue_mutex);
  return 0;
#else
  pthread_cond_signal(&tp->working_cond);
  pthread_mutex_unlock(&tp->work_queue_mutex);
  return NULL;
#endif
}
