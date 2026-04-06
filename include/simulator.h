#ifndef _SIMULATOR_H
#define _SIMULATOR_H

#include <SDL2/SDL.h>
#include <stdint.h>

#include "alloc.h"
#include "spatial_hasher.h"
#include "world.h"

struct simulator_config {
  int32_t window_width;
  int32_t window_height;
  size_t num_particles;
};

struct simulator {
  struct allocator alloc;
  struct allocator frame_alloc;

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *framebuf;
  int32_t win_w;
  int32_t win_h;

  struct world world;
  struct spatial_hasher sh;

  uint8_t running;
};

void simulator_init(
  struct simulator *sim,
  struct simulator_config cfg,
  struct allocator alloc,
  struct allocator frame_alloc
);
void simulator_free(struct simulator *sim);
void simulator_tick(struct simulator *sim, float dt);
void simulator_draw(struct simulator *sim);

#endif // _SIMULATOR_H
