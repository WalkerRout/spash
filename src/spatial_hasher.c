#include "spatial_hasher.h"

#include <assert.h>

void spatial_hasher_init(
  struct spatial_hasher *sh,
  float cell_size,
  struct spashable intface,
  struct allocator *alloc
) {
  assert(sh);
  assert(alloc);
  sh->cell_size = cell_size;
  sh->intface = intface;
  sh->alloc = alloc;
}

void spatial_hasher_free(struct spatial_hasher *sh) {
  (void)sh;
}

void spatial_hasher_build(
  struct spatial_hasher *sh,
  const void *items,
  size_t items_len
) {
  (void)sh;
  (void)items;
  (void)items_len;
}

const void **spatial_hasher_query(
  const struct spatial_hasher *sh,
  const void *item,
  size_t *out_neighbours_len
) {
  (void)sh;
  (void)item;
  *out_neighbours_len = 0;
  return NULL;
}
