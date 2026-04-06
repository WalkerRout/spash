#ifndef _ALLOC_H
#define _ALLOC_H

#include <stddef.h>

struct allocator {
  void *(*alloc)(void *ctx, size_t size);
  void *(*realloc)(void *ctx, void *ptr, size_t old_size, size_t new_size);
  void (*free)(void *ctx, void *ptr);
  void *ctx;
};

struct allocator allocator_heap(void);

#endif // _ALLOC_H
