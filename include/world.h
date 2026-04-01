#ifndef _WORLD_H
#define _WORLD_H

#include <stdint.h>

#include "particle.h"

struct world_config {
  size_t width;
  size_t height;
  size_t num_particles;
};

struct world {
  size_t width;
  size_t height;
  size_t particles_len;
  struct particle *particles;
};

void world_init(struct world *world, struct world_config cfg);
void world_free(struct world *world);
void world_spin(struct world *world, SDL_Window *window, SDL_Renderer *renderer);

#endif // _WORLD_H
