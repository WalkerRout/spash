#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "simulator.h"

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

  struct simulator sim;
  simulator_init(
    &sim,
    (struct simulator_config) {
      .window_width = 2000,
      .window_height = 1000,
      .num_particles = 100000,
    }
  );

  struct delta_time dt;
  delta_time_init(&dt);

  SDL_Event event = {0};
  while (sim.state == SIMULATOR_STATE_RUNNING) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        sim.state = SIMULATOR_STATE_STOPPED;
      }
    }
    double delta_ms = delta_time_fetch(&dt);
    float delta_s = (float)(delta_ms / 1000.0);
    simulator_tick(&sim, delta_s);
    simulator_draw(&sim, dt.fps);
  }

  simulator_free(&sim);
  SDL_Quit();

  return 0;
}
