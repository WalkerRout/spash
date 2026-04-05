#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "draw.h"
#include "world.h"

#define WIDTH 1280
#define HEIGHT 720

static void blit_particle(struct particle particle, SDL_Renderer *renderer,
                          int32_t win_w, int32_t win_h) {
  int32_t min_dim = win_w;
  if (win_h < min_dim) {
    min_dim = win_h;
  }
  // normalize particle values to world space...
  int32_t radius = (int32_t)(particle.radius * (float)min_dim);
  int32_t px = (int32_t)(particle.pos.x * (float)win_w);
  int32_t py = (int32_t)(particle.pos.y * (float)win_h);

  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  draw_circle(renderer, px, py, radius);
}

static void blit_world(struct world *world, SDL_Renderer *renderer,
                       int32_t win_w, int32_t win_h) {
  for (size_t i = 0; i < world->particles_len; ++i) {
    blit_particle(world->particles[i], renderer, win_w, win_h);
  }
}

enum state { HALTED = 0, RUNNING = 1 };

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  srand((unsigned int)time(NULL));

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    return -1;
  }

  SDL_Window *window = SDL_CreateWindow(
      "spatial hashing", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
  assert(window);

  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  assert(renderer);

  int32_t win_w = 0;
  int32_t win_h = 0;
  SDL_GetRendererOutputSize(renderer, &win_w, &win_h);

  SDL_Texture *framebuf =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET, win_w, win_h);
  assert(framebuf);

  struct world world;
  world_init(&world, (struct world_config){.num_particles = 1000});

  enum state state = RUNNING;
  SDL_Event event = {0};
  while (state == RUNNING) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        state = HALTED;
        break;
      }
    }

    // tick
    world_step(&world, 0.001f);

    // display world
    // bind framebuf, enter context drawing to hw accelerated texture
    SDL_SetRenderTarget(renderer, framebuf);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    blit_world(&world, renderer, win_w, win_h);
    // unbind framebuf, defaults back to window
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, framebuf, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  world_free(&world);

  SDL_DestroyTexture(framebuf);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
