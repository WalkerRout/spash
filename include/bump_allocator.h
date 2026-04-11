#ifndef _BUMP_ALLOCATOR_H
#define _BUMP_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

struct bump_allocator {
  size_t offset;
  size_t committed;
  size_t reserved;
  size_t generation;
  uint8_t *base;
};

struct bump_allocator_checkpoint {
  size_t offset;
  size_t generation;
};

void bump_allocator_init(struct bump_allocator *bump, size_t reserve_bytes);
void bump_allocator_free(struct bump_allocator *bump);
void *bump_allocator_alloc(
  struct bump_allocator *bump,
  size_t size_bytes,
  size_t alignment
);
void bump_allocator_clear(struct bump_allocator *bump);
struct bump_allocator_checkpoint
bump_allocator_save(struct bump_allocator *bump);
void bump_allocator_restore(
  struct bump_allocator *bump,
  struct bump_allocator_checkpoint checkpoint
);

#endif // _BUMP_ALLOCATOR_H
