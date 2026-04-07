#include "world.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mvla/mvla.h"
#include "particle.h"
#include "spatial_hasher.h"

static float random_float_0_to_1(void) {
  return (float)rand() / (float)RAND_MAX;
}

#define PARTICLE_RADIUS (0.0001f)

static struct particle random_particle(void) {
  struct particle p;
  p.radius = PARTICLE_RADIUS;
  p.pos = v2f(random_float_0_to_1(), random_float_0_to_1());
  p.vel = v2f(random_float_0_to_1(), random_float_0_to_1());
  return p;
}

void world_init(struct world *world, size_t num_particles) {
  assert(world);
  assert(num_particles > 0);

  world->particles_len = num_particles;
  world->particles = calloc(num_particles, sizeof(struct particle));
  assert(world->particles);
  // assume this buffer always contains garbage
  world->particles_swap = calloc(num_particles, sizeof(struct particle));
  assert(world->particles_swap);

  for (size_t i = 0; i < num_particles; ++i) {
    world->particles[i] = random_particle();
  }

  for (size_t i = 0; i < WORLD_THREAD_COUNT; ++i) {
    arena_init(&world->thread_arenas[i]);
  }
}

void world_free(struct world *world) {
  assert(world);
  for (size_t i = 0; i < WORLD_THREAD_COUNT; ++i) {
    arena_free(&world->thread_arenas[i]);
  }
  free(world->particles);
  free(world->particles_swap);
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
      // reflect velocity along collision normal (no sqrt)
      float dx = p.pos.x - n->pos.x;
      float dy = p.pos.y - n->pos.y;
      float dist_sq = dx * dx + dy * dy;
      if (dist_sq > 0.0f) {
        float dot = out.vel.x * dx + out.vel.y * dy;
        if (dot < 0.0f) {
          out.vel.x -= 2.0f * dot * dx / dist_sq;
          out.vel.y -= 2.0f * dot * dy / dist_sq;
        }
      }
    }
  }

  // integrate position wrt deltatime
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

struct particle_chunk_task {
  float dt;
  // readonly
  struct particle *buffer;
  // writeonly
  struct particle *swap;
  size_t start;
  size_t end;
  // readonly
  struct spatial_hasher *sh;
  // tls arena for intermediate allocations n shit
  struct arena *arena;
};

static void chunk_particle_update(void *arg) {
  assert(arg);
  struct particle_chunk_task *task = (struct particle_chunk_task *)arg;
  for (size_t i = task->start; i < task->end; ++i) {
    struct particle p = task->buffer[i];
    size_t neighbours_len = 0;
    const void **neighbours =
      spatial_hasher_query(task->sh, task->arena, &p, &neighbours_len);
    task->swap[i] = step_particle(p, neighbours, neighbours_len, task->dt);
  }
  arena_clear(task->arena);
}

static v2f_t particle_position(const void *item) {
  const struct particle *p = (const struct particle *)item;
  return p->pos;
}

// safety: particles_swap is fully initialized
static void swap_buffers(struct world *world) {
  struct particle *temp = world->particles;
  world->particles = world->particles_swap;
  world->particles_swap = temp;
}

void world_step(
  struct world *world,
  struct arena *arena,
  struct thread_pool *pool,
  float dt
) {
  assert(world);
  assert(arena);
  assert(pool);

  // use arena context to create a transient hasher
  struct spatial_hasher sh;
  spatial_hasher_init(
    &sh,
    arena,
    PARTICLE_RADIUS * 20.0f,
    (struct spashable) {
      .sizeof_item = sizeof(struct particle),
      .position = particle_position,
    },
    world->particles,
    world->particles_len
  );

  size_t chunk_size = world->particles_len / WORLD_THREAD_COUNT;
  size_t slack = world->particles_len % WORLD_THREAD_COUNT;
  struct particle_chunk_task tasks[WORLD_THREAD_COUNT];

  size_t curr = 0;
  for (size_t i = 0; i < WORLD_THREAD_COUNT; ++i) {
    size_t start = curr;
    size_t end = start + chunk_size;
    if (i < slack) {
      end += 1;
    }
    tasks[i] = (struct particle_chunk_task) {
      .buffer = world->particles,
      .swap = world->particles_swap,
      .start = start,
      .end = end,
      .sh = &sh,
      .arena = &world->thread_arenas[i],
      .dt = dt,
    };
    thread_pool_add_work(pool, chunk_particle_update, &tasks[i]);
    curr = end;
  }

  thread_pool_wait(pool);
  swap_buffers(world);
}
