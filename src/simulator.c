#include "simulator.h"

#include <SDL2/SDL.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "particle.h"

void simulator_init(struct simulator *sim, struct simulator_config cfg) {
  assert(sim);

  // mark simulator as live
  sim->state = SIMULATOR_STATE_RUNNING;
  // init contexts
  bump_allocator_init(&sim->bump, 64 * 1024 * 1024);
  thread_pool_init(&sim->pool, WORLD_THREAD_COUNT);
  // init sim world
  world_init(&sim->world, cfg.num_particles);

  // init all sdlslop

  // assuming SDL_Init was called...
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

  // assuming TTF_Init was called...
  sim->font = TTF_OpenFont("C:/Windows/Fonts/consola.ttf", 16);
  assert(sim->font);
}

void simulator_free(struct simulator *sim) {
  assert(sim);
  world_free(&sim->world);
  thread_pool_free(&sim->pool);
  bump_allocator_free(&sim->bump);
  TTF_CloseFont(sim->font);
  TTF_Quit();
  SDL_DestroyTexture(sim->framebuf);
  SDL_DestroyRenderer(sim->renderer);
  SDL_DestroyWindow(sim->window);
}

void simulator_tick(struct simulator *sim, float dt) {
  assert(sim);
  world_step(&sim->world, &sim->bump, &sim->pool, dt);
  bump_allocator_clear(&sim->bump);
}

static void
draw_circle(SDL_Renderer *renderer, int32_t cx, int32_t cy, int32_t radius) {
  int32_t x = radius;
  int32_t y = 0;
  int32_t err = 1 - radius;

  while (x >= y) {
    (void)SDL_RenderDrawLine(renderer, cx - x, cy + y, cx + x, cy + y);
    (void)SDL_RenderDrawLine(renderer, cx - x, cy - y, cx + x, cy - y);
    (void)SDL_RenderDrawLine(renderer, cx - y, cy + x, cx + y, cy + x);
    (void)SDL_RenderDrawLine(renderer, cx - y, cy - x, cx + y, cy - x);

    ++y;
    if (err < 0) {
      err += 2 * y + 1;
    } else {
      --x;
      err += 2 * (y - x) + 1;
    }
  }
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

static void blit_fps(SDL_Renderer *renderer, TTF_Font *font, uint32_t fps) {
  char buf[32];
  (void)snprintf(buf, sizeof(buf), "fps: %u", fps);
  SDL_Color white = {255, 255, 255, 255};
  SDL_Surface *surface = TTF_RenderText_Blended(font, buf, white);
  assert(surface);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_Rect dst = {10, 10, surface->w, surface->h};
  SDL_FreeSurface(surface);
  assert(texture);
  SDL_RenderCopy(renderer, texture, NULL, &dst);
  SDL_DestroyTexture(texture);
}

void simulator_draw(struct simulator *sim, uint32_t fps) {
  assert(sim);
  // bind framebuf, enter context drawing to hw accelerated texture
  SDL_SetRenderTarget(sim->renderer, sim->framebuf);
  SDL_SetRenderDrawColor(sim->renderer, 0, 0, 0, 255);
  SDL_RenderClear(sim->renderer);
  blit_world(sim->renderer, &sim->world, sim->win_w, sim->win_h);
  blit_fps(sim->renderer, sim->font, fps);
  // unbind framebuf, defaults back to window
  SDL_SetRenderTarget(sim->renderer, NULL);
  SDL_RenderCopy(sim->renderer, sim->framebuf, NULL, NULL);
  SDL_RenderPresent(sim->renderer);
}
