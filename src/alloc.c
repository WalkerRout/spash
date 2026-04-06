#include "alloc.h"

#include <stdlib.h>

static void *heap_alloc(void *ctx, size_t size) {
  (void)ctx;
  return malloc(size);
}

static void *
heap_realloc(void *ctx, void *ptr, size_t old_size, size_t new_size) {
  (void)ctx;
  (void)old_size;
  return realloc(ptr, new_size);
}

static void heap_free(void *ctx, void *ptr) {
  (void)ctx;
  free(ptr);
}

struct allocator allocator_heap(void) {
  return (struct allocator) {
    .alloc = heap_alloc,
    .realloc = heap_realloc,
    .free = heap_free,
    .ctx = NULL,
  };
}
