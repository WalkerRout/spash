#ifndef _SIMULATOR_SDL_H
#define _SIMULATOR_SDL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdint.h>

#include "bump_allocator.h"
#include "mvla/mvla.h"
#include "thread_pool.h"
#include "world.h"

struct simulator_sdl_config {
  int32_t window_width;
  int32_t window_height;
  size_t num_particles;
  // how many screen pixels represent one world unit
  float pixels_per_world_unit;
};

enum simulator_sdl_state {
  SIMULATOR_SDL_STATE_STOPPED = 0,
  SIMULATOR_SDL_STATE_RUNNING,
};

struct camera {
  // top-left of the view in world coords
  v2f_t pos;
  // width/height of the view in world coords
  v2f_t view_size;
};

struct simulator_sdl {
  enum simulator_sdl_state state;

  // per tick contexts/monads
  struct bump_allocator bump;
  struct thread_pool pool;

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *framebuf;
  TTF_Font *font;
  int32_t win_w;
  int32_t win_h;

  // camera for scrolling across the world
  struct camera cam;
  float pan_speed; // world units per second

  // particle simulation world
  struct world world;
};

void simulator_sdl_init(
  struct simulator_sdl *sim,
  struct simulator_sdl_config cfg
);
void simulator_sdl_free(struct simulator_sdl *sim);
void simulator_sdl_handle_input(struct simulator_sdl *sim, float dt);
void simulator_sdl_tick(struct simulator_sdl *sim, float dt);
void simulator_sdl_draw(struct simulator_sdl *sim, uint32_t fps);

#endif // _SIMULATOR_SDL_H
