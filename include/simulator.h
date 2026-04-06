#ifndef _SIMULATOR_H
#define _SIMULATOR_H

#include <SDL2/SDL.h>
#include <stdint.h>

#include "alloc.h"
#include "arena.h"
#include "spatial_hasher.h"
#include "thread_pool.h"
#include "world.h"

struct simulator_config {
  int32_t window_width;
  int32_t window_height;
  size_t num_particles;
};

struct simulator {
  struct allocator alloc;
  struct arena arena;

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *framebuf;
  int32_t win_w;
  int32_t win_h;

  struct thread_pool *pool;
  struct spashable intface;
  struct world world;

  uint8_t running;
};

void simulator_init(
  struct simulator *sim,
  struct simulator_config cfg,
  struct allocator alloc
);
void simulator_free(struct simulator *sim);
void simulator_tick(struct simulator *sim, float dt);
void simulator_draw(struct simulator *sim);

#endif // _SIMULATOR_H
