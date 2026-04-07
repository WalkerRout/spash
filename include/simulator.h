#ifndef _SIMULATOR_H
#define _SIMULATOR_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdint.h>

#include "arena.h"
#include "thread_pool.h"
#include "world.h"

struct simulator_config {
  int32_t window_width;
  int32_t window_height;
  size_t num_particles;
};

enum simulator_state { SIMULATOR_STATE_STOPPED = 0, SIMULATOR_STATE_RUNNING };

struct simulator {
  enum simulator_state state;

  // per tick contexts/monads
  struct arena arena;
  struct thread_pool pool;

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *framebuf;
  TTF_Font *font;
  int32_t win_w;
  int32_t win_h;

  // particle simulation world
  struct world world;
};

void simulator_init(struct simulator *sim, struct simulator_config cfg);
void simulator_free(struct simulator *sim);
void simulator_tick(struct simulator *sim, float dt);
void simulator_draw(struct simulator *sim, uint32_t fps);

#endif // _SIMULATOR_H
