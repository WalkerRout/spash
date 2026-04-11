#include "bump_allocator.h"

#include <assert.h>
#include <stdint.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <sys/mman.h>
#endif

#define ALIGNMENT (sizeof(uintptr_t))
#define PAGE_SIZE (4096)

static size_t align_up(size_t val, size_t alignment) {
  return (val + alignment - 1) & ~(alignment - 1);
}

void bump_allocator_init(struct bump_allocator *bump, size_t reserve_bytes) {
  assert(bump);
  assert(reserve_bytes > 0);

  bump->reserved = align_up(reserve_bytes, PAGE_SIZE);
  bump->offset = 0;
  bump->committed = 0;
  bump->generation = 0;

#ifdef _WIN32
  bump->base = VirtualAlloc(NULL, bump->reserved, MEM_RESERVE, PAGE_READWRITE);
#else
  bump->base =
    mmap(NULL, bump->reserved, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (bump->base == MAP_FAILED) {
    bump->base = NULL;
  }
#endif

  assert(bump->base);
}

void bump_allocator_free(struct bump_allocator *bump) {
  assert(bump);
  if (bump->base) {
#ifdef _WIN32
    VirtualFree(bump->base, 0, MEM_RELEASE);
#else
    munmap(bump->base, bump->reserved);
#endif
  }
  bump->base = NULL;
  bump->offset = 0;
  bump->committed = 0;
  bump->reserved = 0;
  bump->generation = 0;
}

void *bump_allocator_alloc(
  struct bump_allocator *bump,
  size_t size_bytes,
  size_t alignment
) {
  assert(bump);
  assert(bump->base);
  assert(alignment > 0 && (alignment & (alignment - 1)) == 0);

  size_t aligned_offset = align_up(bump->offset, alignment);
  size_t new_offset = aligned_offset + size_bytes;
  assert(new_offset <= bump->reserved);

  if (new_offset > bump->committed) {
    size_t needed = align_up(new_offset - bump->committed, PAGE_SIZE);
#ifdef _WIN32
    void *result = VirtualAlloc(
      bump->base + bump->committed,
      needed,
      MEM_COMMIT,
      PAGE_READWRITE
    );
    assert(result);
#else
    int result =
      mprotect(bump->base + bump->committed, needed, PROT_READ | PROT_WRITE);
    assert(result == 0);
#endif
    bump->committed += needed;
  }

  void *ptr = bump->base + aligned_offset;
  bump->offset = new_offset;
  return ptr;
}

void bump_allocator_clear(struct bump_allocator *bump) {
  assert(bump);
  bump->offset = 0;
  bump->generation += 1;
}

struct bump_allocator_checkpoint bump_allocator_save(struct bump_allocator *bump) {
  assert(bump);
  return (struct bump_allocator_checkpoint) {
    .offset = bump->offset,
    .generation = bump->generation,
  };
}

void bump_allocator_restore(
  struct bump_allocator *bump,
  struct bump_allocator_checkpoint checkpoint
) {
  assert(bump);
  assert(checkpoint.generation == bump->generation);
  assert(checkpoint.offset <= bump->offset);
  bump->offset = checkpoint.offset;
}
