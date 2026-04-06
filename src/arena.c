#include "arena.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static struct region *new_region(size_t capacity);
static void free_region(struct region *region);
static void clear_region(struct region *region);

void arena_init(struct arena *arena) {
  assert(arena);
  arena->beg = NULL;
  arena->end = NULL;
}

void arena_free(struct arena *arena) {
  assert(arena);
  struct region *region = arena->beg;
  while (region) {
    struct region *next = region->next;
    free_region(region);
    region = next;
  }
  arena->beg = NULL;
  arena->end = NULL;
}

void *arena_alloc(struct arena *arena, size_t size_bytes) {
  assert(arena);
  size_t size = (size_bytes + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);

  if (arena->end == NULL) {
    size_t capacity = REGION_DEFAULT_CAPACITY;
    if (capacity < size)
      capacity = size;
    arena->end = new_region(capacity);
    arena->beg = arena->end;
  }

  while (arena->end->offset + size > arena->end->capacity
         && arena->end->next != NULL) {
    arena->end = arena->end->next;
  }

  if (arena->end->offset + size > arena->end->capacity) {
    size_t capacity = REGION_DEFAULT_CAPACITY;
    if (capacity < size)
      capacity = size;
    arena->end->next = new_region(capacity);
    arena->end = arena->end->next;
  }

  void *p = arena->end->data + arena->end->offset;
  arena->end->offset += size;
  return p;
}

void arena_clear(struct arena *arena) {
  assert(arena);
  for (struct region *curr = arena->beg; curr != NULL; curr = curr->next) {
    clear_region(curr);
  }
  arena->end = arena->beg;
}

static struct region *new_region(size_t capacity) {
  size_t bytes = sizeof(struct region) + capacity * sizeof(uintptr_t);
  struct region *region = calloc(1, bytes);
  assert(region);
  region->next = NULL;
  region->capacity = capacity;
  region->offset = 0;
  return region;
}

static void free_region(struct region *region) {
  free(region);
}

static void clear_region(struct region *region) {
  region->offset = 0;
}
