#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include "mvla/mvla.h"

#include "draw.h"
#include "particle.h"
#include "world.h"

static void render_world(struct world *world, SDL_Renderer *renderer);

static void blit_world(struct world *world);
static void blit_particle(struct particle particle);

static void step_world(struct world *world);
static void step_particle(struct particle *p, struct particle *neighbour_set,
                          size_t neighbour_set_len);

static float random_float(void);
static struct particle random_particle(void);

void world_init(struct world *world, struct world_config cfg) {
  assert(world);
  assert(cfg.num_particles > 0);

  world->width = cfg.width;
  world->height = cfg.height;
  world->particles_len = cfg.num_particles;
  world->particles = malloc(cfg.num_particles * sizeof(struct particle));

  assert(world->particles);

  for (size_t i = 0; i < cfg.num_particles; ++i) {
    world->particles[i] = random_particle();
  }
}

void world_free(struct world *world) {
  assert(world);
  if (world->particles) {
    free(world->particles);
  }
}

enum state { STATE_STOPPED = 0, STATE_RUNNING = 1 };

void world_spin(struct world *world, SDL_Window *window,
                SDL_Renderer *renderer) {
  assert(world);

  (void)window;

  enum state state = STATE_RUNNING;

  size_t ticks = 0;
  SDL_Event event = {0};
  while (state == STATE_RUNNING) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        state = STATE_STOPPED;
        break;
      }
    }
    // tick
    step_world(world);
    ticks += 1;
    (void)ticks;

    // display
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer); // clear the renderer
    render_world(world, renderer);
    SDL_RenderPresent(renderer);
  }
}

static void step_world(struct world *world) {
  assert(world);
  for (size_t i = 0; i < world->particles_len; ++i) {
    // struct particle *neighbours = spatial_hasher_find_neighbours(pi, world);
    struct particle *p = &world->particles[i];
    struct particle *neighbour_set = world->particles;
    size_t neighbour_set_len = world->particles_len;
    step_particle(p, neighbour_set, neighbour_set_len);
  }
}

static void step_particle(struct particle *p, struct particle *neighbour_set,
                          size_t neighbour_set_len) {
  assert(p);
  assert(neighbour_set);
  for (size_t i = 0; i < neighbour_set_len; ++i) {
    struct particle *n = &neighbour_set[i];
    // are we colliding with a different particle
    if (p != n && particle_is_colliding(*p, *n)) {
      // simple velocity swap
      v2f_t temp = p->vel;
      p->vel = n->vel;
      n->vel = temp;
    }
  }
}

static void render_world(struct world *world, SDL_Renderer *renderer) {
  (void)world;
  (void)renderer;
}

static void blit_world(struct world *world) { (void)world; }

static void blit_particle(struct particle particle) { (void)particle; }

static float random_float(void) { return (float)rand() / (float)RAND_MAX; }

static struct particle random_particle(void) {
  struct particle p;
  p.radius = 0.02f;
  p.pos = v2f(random_float(), random_float());
  p.vel = v2f(random_float(), random_float());
  return p;
}
