#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "mvla/mvla.h"

#include "particle.h"
#include "world.h"

static struct particle step_particle(struct particle p,
                                     struct particle *neighbour_set,
                                     size_t neighbour_set_len);

static float random_float(void);
static struct particle random_particle(void);

void world_init(struct world *world, struct world_config cfg) {
  assert(world);
  assert(cfg.num_particles > 0);

  world->particles_len = cfg.num_particles;
  world->particles = malloc(cfg.num_particles * sizeof(struct particle));
  assert(world->particles);
  // assume this buffer always contains garbage
  world->particles_swap = malloc(cfg.num_particles * sizeof(struct particle));
  assert(world->particles_swap);

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

// safety: particles_swap is fully initialized
static void swap_buffers(struct world *world) {
  struct particle *temp = world->particles;
  world->particles = world->particles_swap;
  world->particles_swap = temp;
}

void world_step(struct world *world) {
  assert(world);
  for (size_t i = 0; i < world->particles_len; ++i) {
    // struct particle *neighbours = spatial_hasher_find_neighbours(pi, world);
    struct particle p = world->particles[i];
    struct particle *neighbour_set = world->particles;
    size_t neighbour_set_len = world->particles_len;
    world->particles_swap[i] =
        step_particle(p, neighbour_set, neighbour_set_len);
  }
  swap_buffers(world);
}

static struct particle step_particle(struct particle p,
                                     struct particle *neighbour_set,
                                     size_t neighbour_set_len) {
  assert(neighbour_set);
  struct particle out = p;
  for (size_t i = 0; i < neighbour_set_len; ++i) {
    struct particle *n = &neighbour_set[i];
    // are we colliding with a different particle
    if (particle_is_colliding(p, *n)) {
      // simple velocity swap
      v2f_t temp = out.vel;
      out.vel = n->vel;
      n->vel = temp;
    }
  }
  return out;
}

static float random_float(void) { return (float)rand() / (float)RAND_MAX; }

static struct particle random_particle(void) {
  struct particle p;
  p.radius = 0.007f;
  p.pos = v2f(random_float(), random_float());
  p.vel = v2f(random_float(), random_float());
  return p;
}
