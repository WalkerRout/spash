#ifndef _SPATIAL_HASHER_H
#define _SPATIAL_HASHER_H

#include <stddef.h>
#include <stdint.h>

#include "alloc.h"
#include "mvla/mvla.h"

struct spashable {
  size_t sizeof_item;
  v2f_t (*position)(const void *item);
  uint64_t (*hash)(const void *item, float cell_size);
};

struct spatial_hasher {
  float cell_size;
  struct spashable intface;
  struct allocator *alloc; // frame allocator; reset each tick
};

void spatial_hasher_init(
  struct spatial_hasher *sh,
  float cell_size,
  struct spashable intface,
  struct allocator *alloc
);
void spatial_hasher_free(struct spatial_hasher *sh);
void spatial_hasher_build(
  struct spatial_hasher *sh,
  const void *items,
  size_t items_len
);
const void **spatial_hasher_query(
  const struct spatial_hasher *sh,
  const void *item,
  size_t *out_neighbours_len
);

#endif // _SPATIAL_HASHER_H
