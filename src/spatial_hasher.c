#include "spatial_hasher.h"

#include <assert.h>
#include <string.h>

#define ALLOC_ALIGN (sizeof(void *))

static uint64_t
bucket_index(const struct spatial_hasher *sh, const void *item) {
  v2f_t pos = sh->intface.position(item);
  int32_t cx = (int32_t)(pos.x / sh->cell_size);
  int32_t cy = (int32_t)(pos.y / sh->cell_size);
  return cell_hash(cx, cy) % sh->num_buckets;
}

void spatial_hasher_init(
  struct spatial_hasher *sh,
  struct bump_allocator *bump,
  float cell_size,
  struct spashable intface,
  const void *items,
  size_t items_len
) {
  assert(sh);
  assert(bump);
  assert(items);

  sh->cell_size = cell_size;
  sh->intface = intface;

  // 2x load factor
  sh->num_buckets = items_len * 2;
  if (sh->num_buckets < 16) {
    sh->num_buckets = 16;
  }

  size_t buckets_size = sh->num_buckets * sizeof(struct spatial_hasher_entry *);
  sh->buckets = bump_allocator_alloc(bump, buckets_size, ALLOC_ALIGN);
  memset(sh->buckets, 0, buckets_size);

  // insert all items
  const char *base = (const char *)items;
  for (size_t i = 0; i < items_len; ++i) {
    // dead recon the item position
    const void *item = base + i * intface.sizeof_item;
    uint64_t idx = bucket_index(sh, item);

    struct spatial_hasher_entry *entry = bump_allocator_alloc(
      bump,
      sizeof(struct spatial_hasher_entry),
      ALLOC_ALIGN
    );
    entry->item = item;
    entry->next = sh->buckets[idx];
    sh->buckets[idx] = entry;
  }
}

const void **spatial_hasher_query(
  const struct spatial_hasher *sh,
  struct bump_allocator *bump,
  const void *item,
  size_t *out_neighbours_len
) {
  assert(sh);
  assert(bump);
  assert(item);
  assert(out_neighbours_len);

  v2f_t pos = sh->intface.position(item);
  int32_t cx = (int32_t)(pos.x / sh->cell_size);
  int32_t cy = (int32_t)(pos.y / sh->cell_size);

  // collect from 3x3 neighborhood
  size_t count = 0;
  size_t capacity = 32;
  const void **result =
    bump_allocator_alloc(bump, capacity * sizeof(const void *), ALLOC_ALIGN);

  for (int32_t dx = -1; dx <= 1; ++dx) {
    for (int32_t dy = -1; dy <= 1; ++dy) {
      uint64_t hash = cell_hash(cx + dx, cy + dy);
      uint64_t idx = hash % sh->num_buckets;

      struct spatial_hasher_entry *entry = sh->buckets[idx];
      while (entry) {
        // verify the item actually belongs to this cell neighborhood
        v2f_t npos = sh->intface.position(entry->item);
        int32_t ncx = (int32_t)(npos.x / sh->cell_size);
        int32_t ncy = (int32_t)(npos.y / sh->cell_size);
        int32_t ddx = ncx - cx;
        int32_t ddy = ncy - cy;
        if (ddx >= -1 && ddx <= 1 && ddy >= -1 && ddy <= 1) {
          if (count >= capacity) {
            size_t new_capacity = capacity * 2;
            const void **new_result = bump_allocator_alloc(
              bump,
              new_capacity * sizeof(const void *),
              ALLOC_ALIGN
            );
            memcpy(new_result, result, count * sizeof(const void *));
            result = new_result;
            capacity = new_capacity;
          }
          result[count++] = entry->item;
        }
        entry = entry->next;
      }
    }
  }

  *out_neighbours_len = count;
  return result;
}
