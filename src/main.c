#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "simulator_sdl.h"

struct delta_time {
  uint64_t curr;
  uint64_t prev;
  uint32_t frames;
  uint32_t fps;
  double accumulator;
};

void delta_time_init(struct delta_time *dt) {
  dt->prev = 0;
  dt->curr = SDL_GetPerformanceCounter();
  dt->frames = 0;
  dt->fps = 0;
  dt->accumulator = 0.0;
}

double delta_time_fetch(struct delta_time *dt) {
  dt->prev = dt->curr;
  dt->curr = SDL_GetPerformanceCounter();
  double delta_ms = (double)(dt->curr - dt->prev) * 1000.0
    / (double)SDL_GetPerformanceFrequency();

  dt->frames++;
  dt->accumulator += delta_ms;
  if (dt->accumulator >= 1000.0) {
    dt->fps = dt->frames;
    dt->frames = 0;
    dt->accumulator = 0.0;
  }

  return delta_ms;
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  srand((unsigned int)time(NULL));

  assert(TTF_Init() == 0);
  assert(SDL_Init(SDL_INIT_VIDEO) == 0);

  struct simulator_sdl sim;
  simulator_sdl_init(
    &sim,
    (struct simulator_sdl_config) {
      .window_width =  1200,
      .window_height = 700,
      .num_particles = 70000,
      .pixels_per_world_unit = 500.0f,
    }
  );

  struct delta_time dt;
  delta_time_init(&dt);

  SDL_Event event = {0};
  while (sim.state == SIMULATOR_SDL_STATE_RUNNING) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        sim.state = SIMULATOR_SDL_STATE_STOPPED;
      }
    }
    double delta_ms = delta_time_fetch(&dt);
    float delta_s = (float)(delta_ms / 1000.0);
    simulator_sdl_handle_input(&sim, delta_s);
    simulator_sdl_tick(&sim, delta_s);
    simulator_sdl_draw(&sim, dt.fps);
  }

  simulator_sdl_free(&sim);
  SDL_Quit();

  return 0;
}
