#ifndef _ARENA_H
#define _ARENA_H

#include <stddef.h>
#include <stdint.h>

#define REGION_DEFAULT_CAPACITY (4096)

struct region {
  struct region *next;
  size_t capacity;
  size_t offset;
  uintptr_t data[];
};

struct arena {
  struct region *beg;
  struct region *end;
};

void arena_init(struct arena *arena);
void arena_free(struct arena *arena);
void *arena_alloc(struct arena *arena, size_t size_bytes);
void arena_clear(struct arena *arena);

#endif // _ARENA_H
