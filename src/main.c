#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "alloc.h"
#include "simulator.h"

struct delta_time {
  uint64_t curr;
  uint64_t prev;
};

void delta_time_init(struct delta_time *dt) {
  dt->prev = 0;
  dt->curr = SDL_GetPerformanceCounter();
}

double delta_time_fetch(struct delta_time *dt) {
  dt->prev = dt->curr;
  dt->curr = SDL_GetPerformanceCounter();
  double delta = (double)(dt->curr - dt->prev) * 1000.0
    / (double)SDL_GetPerformanceFrequency();
  return delta;
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  srand((unsigned int)time(NULL));

  assert(SDL_Init(SDL_INIT_VIDEO) == 0);

  struct allocator alloc = allocator_heap();
  struct allocator frame_alloc = allocator_heap();

  struct simulator sim;
  simulator_init(
    &sim,
    (struct simulator_config) {
      .window_width = 2000,
      .window_height = 1500,
      .num_particles = 5000,
    },
    alloc,
    frame_alloc
  );

  struct delta_time dt;
  delta_time_init(&dt);

  SDL_Event event = {0};
  while (sim.running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        sim.running = 0;
      }
    }
    double delta = delta_time_fetch(&dt);
    simulator_tick(&sim, (float)delta / 3000.0f);
    simulator_draw(&sim);
  }

  simulator_free(&sim);
  SDL_Quit();

  return 0;
}
