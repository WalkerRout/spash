#ifndef _WORLD_H
#define _WORLD_H

#include <stddef.h>

#include "bump_allocator.h"
#include "particle.h"
#include "thread_pool.h"

#define WORLD_THREAD_COUNT (4)

// world dims in worldspace units...
// - particles live in [0, WORLD_WIDTH] and [0, WORLD_HEIGHT]
#define WORLD_WIDTH (10.0f)
#define WORLD_HEIGHT (10.0f)

struct world {
  // swap buffers; see WalkerRout/boids...
  size_t particles_len;
  struct particle *particles; // readonly
  struct particle *particles_swap; // writeonly

  // asymmetric attract/repel coefficients in [-1, 1]
  float force_matrix[SPECIES_COUNT][SPECIES_COUNT];

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
