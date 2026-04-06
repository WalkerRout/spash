#include "spatial_hasher.h"

#include <assert.h>
#include <string.h>

static uint64_t cell_hash(int32_t cx, int32_t cy) {
  return (uint64_t)cx * 73856093u ^ (uint64_t)cy * 19349663u;
}

static uint64_t
bucket_index(const struct spatial_hasher *sh, const void *item) {
  v2f_t pos = sh->intface.position(item);
  int32_t cx = (int32_t)(pos.x / sh->cell_size);
  int32_t cy = (int32_t)(pos.y / sh->cell_size);
  return cell_hash(cx, cy) % sh->num_buckets;
}

void spatial_hasher_init(
  struct spatial_hasher *sh,
  struct arena *arena,
  float cell_size,
  struct spashable intface,
  const void *items,
  size_t items_len
) {
  assert(sh);
  assert(arena);
  assert(items);

  sh->cell_size = cell_size;
  sh->intface = intface;
  sh->arena = arena;

  // 2x load factor
  sh->num_buckets = items_len * 2;
  if (sh->num_buckets < 16) {
    sh->num_buckets = 16;
  }

  sh->buckets =
    arena_alloc(arena, sh->num_buckets * sizeof(struct spatial_hasher_entry *));
  memset(
    sh->buckets,
    0,
    sh->num_buckets * sizeof(struct spatial_hasher_entry *)
  );

  // insert all items
  const char *base = (const char *)items;
  for (size_t i = 0; i < items_len; ++i) {
    // dead recon the item position
    const void *item = base + i * intface.sizeof_item;
    uint64_t idx = bucket_index(sh, item);

    struct spatial_hasher_entry *entry =
      arena_alloc(arena, sizeof(struct spatial_hasher_entry));
    entry->item = item;
    entry->next = sh->buckets[idx];
    sh->buckets[idx] = entry;
  }
}

const void **spatial_hasher_query(
  const struct spatial_hasher *sh,
  const void *item,
  size_t *out_neighbours_len
) {
  assert(sh);
  assert(item);
  assert(out_neighbours_len);

  v2f_t pos = sh->intface.position(item);
  int32_t cx = (int32_t)(pos.x / sh->cell_size);
  int32_t cy = (int32_t)(pos.y / sh->cell_size);

  // collect from 3x3 neighborhood
  size_t count = 0;
  size_t capacity = 16;
  const void **result = arena_alloc(sh->arena, capacity * sizeof(const void *));

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
            const void **new_result =
              arena_alloc(sh->arena, new_capacity * sizeof(const void *));
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
