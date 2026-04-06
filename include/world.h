#ifndef _WORLD_H
#define _WORLD_H

#include <stddef.h>

#include "alloc.h"
#include "particle.h"

struct world {
  struct allocator *alloc;
  // swap buffers; see WalkerRout/boids...
  size_t particles_len;
  struct particle *particles; // readonly
  struct particle *particles_swap; // writeonly
};

void world_init(
  struct world *world,
  size_t num_particles,
  struct allocator *alloc
);
void world_free(struct world *world);
void world_step(struct world *world, float dt);

#endif // _WORLD_H
