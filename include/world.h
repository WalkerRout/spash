#ifndef _WORLD_H
#define _WORLD_H

#include <stdint.h>

#include "particle.h"

struct world_config {
  size_t num_particles;
};

struct world {
  // swap buffers; see WalkerRout/boids...
  size_t particles_len;
  struct particle *particles; // readonly
  struct particle *particles_swap; // writeonly
};

void world_init(struct world *world, struct world_config cfg);
void world_free(struct world *world);
void world_step(struct world *world);

#endif // _WORLD_H
