#include "world.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "mvla/mvla.h"
#include "particle.h"

static float random_float_0_to_1(void) {
  return (float)rand() / (float)RAND_MAX;
}

static struct particle random_particle(void) {
  struct particle p;
  p.radius = 0.007f;
  p.pos = v2f(random_float_0_to_1(), random_float_0_to_1());
  p.vel = v2f(random_float_0_to_1(), random_float_0_to_1());
  return p;
}

void world_init(
  struct world *world,
  size_t num_particles,
  struct allocator *alloc
) {
  assert(world);
  assert(num_particles > 0);
  assert(alloc);

  world->alloc = alloc;
  world->particles_len = num_particles;
  world->particles =
    alloc->alloc(alloc->ctx, num_particles * sizeof(struct particle));
  assert(world->particles);
  // assume this buffer always contains garbage
  world->particles_swap =
    alloc->alloc(alloc->ctx, num_particles * sizeof(struct particle));
  assert(world->particles_swap);

  for (size_t i = 0; i < num_particles; ++i) {
    world->particles[i] = random_particle();
  }
}

void world_free(struct world *world) {
  assert(world);
  if (world->particles) {
    world->alloc->free(world->alloc->ctx, world->particles);
  }
  if (world->particles_swap) {
    world->alloc->free(world->alloc->ctx, world->particles_swap);
  }
}

static struct particle step_particle(
  struct particle p,
  struct particle *neighbour_set,
  size_t neighbour_set_len,
  float dt
) {
  assert(neighbour_set);
  struct particle out = p;

  // particle-particle collisions
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

  // integrate position
  out.pos = v2f_add(out.pos, v2f(out.vel.x * dt, out.vel.y * dt));

  // wall bouncing
  if (out.pos.x < 0.0f) {
    out.pos.x = -out.pos.x;
    out.vel.x = -out.vel.x;
  }
  if (out.pos.x > 1.0f) {
    out.pos.x = 2.0f - out.pos.x;
    out.vel.x = -out.vel.x;
  }
  if (out.pos.y < 0.0f) {
    out.pos.y = -out.pos.y;
    out.vel.y = -out.vel.y;
  }
  if (out.pos.y > 1.0f) {
    out.pos.y = 2.0f - out.pos.y;
    out.vel.y = -out.vel.y;
  }

  return out;
}

// safety: particles_swap is fully initialized
static void swap_buffers(struct world *world) {
  struct particle *temp = world->particles;
  world->particles = world->particles_swap;
  world->particles_swap = temp;
}

void world_step(struct world *world, float dt) {
  assert(world);
  for (size_t i = 0; i < world->particles_len; ++i) {
    struct particle p = world->particles[i];
    struct particle *neighbour_set = world->particles;
    size_t neighbour_set_len = world->particles_len;
    world->particles_swap[i] =
      step_particle(p, neighbour_set, neighbour_set_len, dt);
  }
  swap_buffers(world);
}
