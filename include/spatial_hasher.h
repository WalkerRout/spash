#ifndef _SPATIAL_HASHER_H
#define _SPATIAL_HASHER_H

#include <stddef.h>
#include <stdint.h>

#include "arena.h"
#include "mvla/mvla.h"

struct spashable {
  size_t sizeof_item;
  v2f_t (*position)(const void *item);
};

struct spatial_hasher_entry {
  const void *item;
  struct spatial_hasher_entry *next;
};

struct spatial_hasher {
  float cell_size;
  struct spashable intface;

  size_t num_buckets;
  struct spatial_hasher_entry **buckets;
};

void spatial_hasher_init(
  struct spatial_hasher *sh,
  struct arena *arena,
  float cell_size,
  struct spashable intface,
  const void *items,
  size_t items_len
);

const void **spatial_hasher_query(
  const struct spatial_hasher *sh,
  struct arena *arena,
  const void *item,
  size_t *out_neighbours_len
);

#endif // _SPATIAL_HASHER_H
