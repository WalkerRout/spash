#include "simulator_sdl.h"

#include <SDL2/SDL.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "particle.h"

static const SDL_Color SPECIES_COLORS[SPECIES_COUNT] = {
  {255,  80,  80, 255}, // red
  { 80, 200, 120, 255}, // green
  { 80, 140, 255, 255}, // blue
  {255, 200,  60, 255}, // yellow
  {200,  80, 220, 255}, // magenta
  { 60, 220, 220, 255}, // cyan
};

void simulator_sdl_init(
  struct simulator_sdl *sim,
  struct simulator_sdl_config cfg
) {
  assert(sim);

  // mark simulator as live
  sim->state = SIMULATOR_SDL_STATE_RUNNING;
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
#ifdef _WIN32
  sim->font = TTF_OpenFont("C:/Windows/Fonts/consola.ttf", 16);
#else
  sim->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 16);
#endif
  assert(sim->font);

  // camera derived from window size and configured pixel density
  sim->cam.view_size.x = (float)sim->win_w / cfg.pixels_per_world_unit;
  sim->cam.view_size.y = (float)sim->win_h / cfg.pixels_per_world_unit;
  sim->cam.pos = v2f(
    (WORLD_WIDTH - sim->cam.view_size.x) / 2.0f,
    (WORLD_HEIGHT - sim->cam.view_size.y) / 2.0f
  );
  sim->pan_speed = 1.5f;
}

void simulator_sdl_free(struct simulator_sdl *sim) {
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

void simulator_sdl_handle_input(struct simulator_sdl *sim, float dt) {
  assert(sim);
  const uint8_t *keys = SDL_GetKeyboardState(NULL);
  float step = sim->pan_speed * dt;
  if (keys[SDL_SCANCODE_W]) {
    sim->cam.pos.y -= step;
  }
  if (keys[SDL_SCANCODE_S]) {
    sim->cam.pos.y += step;
  }
  if (keys[SDL_SCANCODE_A]) {
    sim->cam.pos.x -= step;
  }
  if (keys[SDL_SCANCODE_D]) {
    sim->cam.pos.x += step;
  }
  if (keys[SDL_SCANCODE_R]) {
    size_t n = sim->world.particles_len;
    world_free(&sim->world);
    world_init(&sim->world, n);
  }

  // clamp so the view never leaves the world
  float max_x = WORLD_WIDTH - sim->cam.view_size.x;
  float max_y = WORLD_HEIGHT - sim->cam.view_size.y;
  if (max_x < 0.0f) {
    max_x = 0.0f;
  }
  if (max_y < 0.0f) {
    max_y = 0.0f;
  }
  if (sim->cam.pos.x < 0.0f) {
    sim->cam.pos.x = 0.0f;
  }
  if (sim->cam.pos.y < 0.0f) {
    sim->cam.pos.y = 0.0f;
  }
  if (sim->cam.pos.x > max_x) {
    sim->cam.pos.x = max_x;
  }
  if (sim->cam.pos.y > max_y) {
    sim->cam.pos.y = max_y;
  }
}

void simulator_sdl_tick(struct simulator_sdl *sim, float dt) {
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
  v2f_t cam_pos,
  float sx_per_wx,
  float sy_per_wy
) {
  int32_t radius = (int32_t)(particle.radius * sx_per_wx);
  int32_t px = (int32_t)((particle.pos.x - cam_pos.x) * sx_per_wx);
  int32_t py = (int32_t)((particle.pos.y - cam_pos.y) * sy_per_wy);

  SDL_Color c =
    SPECIES_COLORS[(size_t)particle.species % (size_t)SPECIES_COUNT];
  SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
  draw_circle(renderer, px, py, radius);
}

static void blit_world(
  SDL_Renderer *renderer,
  struct world *world,
  struct camera cam,
  int32_t win_w,
  int32_t win_h
) {
  float sx_per_wx = (float)win_w / cam.view_size.x;
  float sy_per_wy = (float)win_h / cam.view_size.y;
  float min_x = cam.pos.x;
  float max_x = cam.pos.x + cam.view_size.x;
  float min_y = cam.pos.y;
  float max_y = cam.pos.y + cam.view_size.y;

  for (size_t i = 0; i < world->particles_len; ++i) {
    struct particle p = world->particles[i];
    // AABB frustum cull
    if (p.pos.x + p.radius < min_x) {
      continue;
    }
    if (p.pos.x - p.radius > max_x) {
      continue;
    }
    if (p.pos.y + p.radius < min_y) {
      continue;
    }
    if (p.pos.y - p.radius > max_y) {
      continue;
    }
    blit_particle(renderer, p, cam.pos, sx_per_wx, sy_per_wy);
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

void simulator_sdl_draw(struct simulator_sdl *sim, uint32_t fps) {
  assert(sim);
  // bind framebuf, enter context drawing to hw accelerated texture
  SDL_SetRenderTarget(sim->renderer, sim->framebuf);
  SDL_SetRenderDrawColor(sim->renderer, 0, 0, 0, 255);
  SDL_RenderClear(sim->renderer);
  blit_world(sim->renderer, &sim->world, sim->cam, sim->win_w, sim->win_h);
  blit_fps(sim->renderer, sim->font, fps);
  // unbind framebuf, defaults back to window
  SDL_SetRenderTarget(sim->renderer, NULL);
  SDL_RenderCopy(sim->renderer, sim->framebuf, NULL, NULL);
  SDL_RenderPresent(sim->renderer);
}
