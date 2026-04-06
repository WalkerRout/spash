#include "simulator.h"

#include <SDL2/SDL.h>
#include <assert.h>
#include <stdint.h>

#include "draw.h"
#include "particle.h"

void simulator_init(
  struct simulator *sim,
  struct simulator_config cfg,
  struct allocator alloc,
  struct allocator frame_alloc
) {
  assert(sim);

  sim->alloc = alloc;
  sim->frame_alloc = frame_alloc;
  sim->running = 1;

  sim->window = SDL_CreateWindow(
    "spatial hashing",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    cfg.window_width,
    cfg.window_height,
    SDL_WINDOW_SHOWN
  );
  assert(sim->window);

  sim->renderer = SDL_CreateRenderer(
    sim->window,
    -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );
  assert(sim->renderer);

  SDL_GetRendererOutputSize(sim->renderer, &sim->win_w, &sim->win_h);

  sim->framebuf = SDL_CreateTexture(
    sim->renderer,
    SDL_PIXELFORMAT_RGBA8888,
    SDL_TEXTUREACCESS_TARGET,
    sim->win_w,
    sim->win_h
  );
  assert(sim->framebuf);

  world_init(&sim->world, cfg.num_particles, &sim->alloc);
}

void simulator_free(struct simulator *sim) {
  assert(sim);
  world_free(&sim->world);
  SDL_DestroyTexture(sim->framebuf);
  SDL_DestroyRenderer(sim->renderer);
  SDL_DestroyWindow(sim->window);
}

void simulator_tick(struct simulator *sim, float dt) {
  assert(sim);
  // todo populate buckets
  world_step(&sim->world, dt);
  // todo clear frame allocator/equiv
}

static void blit_particle(
  SDL_Renderer *renderer,
  struct particle particle,
  int32_t win_w,
  int32_t win_h
) {
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

static void blit_world(
  SDL_Renderer *renderer,
  struct world *world,
  int32_t win_w,
  int32_t win_h
) {
  for (size_t i = 0; i < world->particles_len; ++i) {
    blit_particle(renderer, world->particles[i], win_w, win_h);
  }
}

void simulator_draw(struct simulator *sim) {
  assert(sim);
  // bind framebuf, enter context drawing to hw accelerated texture
  SDL_SetRenderTarget(sim->renderer, sim->framebuf);
  SDL_SetRenderDrawColor(sim->renderer, 0, 0, 0, 255);
  SDL_RenderClear(sim->renderer);
  blit_world(sim->renderer, &sim->world, sim->win_w, sim->win_h);
  // unbind framebuf, defaults back to window
  SDL_SetRenderTarget(sim->renderer, NULL);
  SDL_RenderCopy(sim->renderer, sim->framebuf, NULL, NULL);
  SDL_RenderPresent(sim->renderer);
}
