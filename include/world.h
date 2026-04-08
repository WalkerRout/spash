#ifndef _WORLD_H
#define _WORLD_H

#include <stddef.h>

#include "bump_allocator.h"
#include "particle.h"
#include "thread_pool.h"

#define WORLD_THREAD_COUNT (20)

struct world {
  // swap buffers; see WalkerRout/boids...
  size_t particles_len;
  struct particle *particles; // readonly
  struct particle *particles_swap; // writeonly

  struct bump_allocator thread_bumps[WORLD_THREAD_COUNT];
};

void world_init(struct world *world, size_t num_particles);
void world_free(struct world *world);
void world_step(
  struct world *world,
  struct bump_allocator *bump,
  struct thread_pool *pool,
  float dt
);

#endif // _WORLD_H
