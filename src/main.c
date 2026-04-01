#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "world.h"

#define WIDTH 720
#define HEIGHT 1080

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
  if (!window) {
    return -1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1,
      SDL_RENDERER_ACCELERATED |
          SDL_RENDERER_PRESENTVSYNC); // use default graphics card to do
                                      // rendering rather than CPU
  if (!renderer) {
    SDL_DestroyWindow(window);
    return -1;
  }

  struct world world;
  world_init(&world, (struct world_config){.num_particles = 1000,
                                           .width = WIDTH,
                                           .height = HEIGHT});
  world_spin(&world, window, renderer);
  world_free(&world);

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
