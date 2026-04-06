#include "world.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mvla/mvla.h"
#include "particle.h"

static float random_float_0_to_1(void) {
  return (float)rand() / (float)RAND_MAX;
}

static struct particle random_particle(void) {
  struct particle p;
  p.radius = 0.002f;
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
  const void **neighbours,
  size_t neighbours_len,
  float dt
) {
  struct particle out = p;

  // particle-particle collisions
  for (size_t i = 0; i < neighbours_len; ++i) {
    const struct particle *n = (const struct particle *)neighbours[i];
    // skip self
    if (memcmp(n, &p, sizeof(struct particle)) == 0) {
      continue;
    }
    // are we colliding with a different particle
    if (particle_is_colliding(p, *n)) {
      // reflect velocity along collision normal
      float dx = p.pos.x - n->pos.x;
      float dy = p.pos.y - n->pos.y;
      float dist = dx * dx + dy * dy;
      if (dist > 0.0f) {
        dist = sqrtf(dist);
        float nx = dx / dist;
        float ny = dy / dist;
        float dot = out.vel.x * nx + out.vel.y * ny;
        if (dot < 0.0f) {
          out.vel.x -= 2.0f * dot * nx;
          out.vel.y -= 2.0f * dot * ny;
        }
      }
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

/*
#define THREAD_COUNT 4

struct particle_chunk_task {
  struct particle *buffer; // READ ONLY
  struct particle *swap; // WRITE ONLY
  size_t start;
  size_t end;
  struct spatial_hasher *sh;
  float dt;
};

static void chunk_particle_update(void *arg) {
  assert(arg);
  struct particle_chunk_task *task = (struct particle_chunk_task *)arg;
  for (size_t i = task->start; i < task->end; ++i) {
    struct particle p = task->buffer[i];

    size_t neighbours_len = 0;
    const void **neighbours =
      spatial_hasher_query(task->sh, &p, &neighbours_len);

    task->swap[i] = step_particle(p, neighbours, neighbours_len, task->dt);
  }
}
*/

void world_step(
  struct world *world,
  struct spatial_hasher *sh,
  struct thread_pool *pool,
  float dt
) {
  assert(world);
  assert(sh);
  (void)pool;

  // todo threading disabled until arena is threadsafe
  for (size_t i = 0; i < world->particles_len; ++i) {
    struct particle p = world->particles[i];
    size_t neighbours_len = 0;
    const void **neighbours = spatial_hasher_query(sh, &p, &neighbours_len);
    world->particles_swap[i] = step_particle(p, neighbours, neighbours_len, dt);
  }
  swap_buffers(world);
}

/*
  // todo fix
  assert(pool);

  size_t chunk_size = world->particles_len / THREAD_COUNT;
  size_t slack = world->particles_len % THREAD_COUNT;
  struct particle_chunk_task tasks[THREAD_COUNT];

  size_t curr = 0;
  for (size_t i = 0; i < THREAD_COUNT; ++i) {
    size_t start = curr;
    size_t end = start + chunk_size;
    if (i < slack) {
      end += 1;
    }

    tasks[i] = (struct particle_chunk_task){
      .buffer = world->particles,
      .swap = world->particles_swap,
      .start = start,
      .end = end,
      .sh = sh,
      .dt = dt,
    };

    tpool_add_work(pool, chunk_particle_update, &tasks[i]);
    curr = end;
  }

  tpool_wait(pool);
  swap_buffers(world);
*/