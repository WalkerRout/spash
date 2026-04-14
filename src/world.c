#include "world.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mvla/mvla.h"
#include "particle.h"
#include "spatial_hasher.h"

#define THREAD_BUMP_RESERVE (16 * 1024 * 1024)

#define PARTICLE_RADIUS (0.003f)

static float random_float_0_to_1(void) {
  return (float)rand() / (float)RAND_MAX;
}

static float random_float_neg1_to_1(void) {
  return 2.0f * random_float_0_to_1() - 1.0f;
}

static struct particle random_particle(void) {
  struct particle p;
  p.radius = PARTICLE_RADIUS;
  p.pos = v2f(
    random_float_0_to_1() * WORLD_WIDTH,
    random_float_0_to_1() * WORLD_HEIGHT
  );
  p.vel = v2f(0.0f, 0.0f);
  p.species = (enum species)(rand() % (int)SPECIES_COUNT);
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

  // init force matrix
  for (size_t i = 0; i < (size_t)SPECIES_COUNT; ++i) {
    for (size_t j = 0; j < (size_t)SPECIES_COUNT; ++j) {
      world->force_matrix[i][j] = random_float_neg1_to_1();
    }
  }

  for (size_t i = 0; i < WORLD_THREAD_COUNT; ++i) {
    bump_allocator_init(&world->thread_bumps[i], THREAD_BUMP_RESERVE);
  }
}

void world_free(struct world *world) {
  assert(world);
  for (size_t i = 0; i < WORLD_THREAD_COUNT; ++i) {
    bump_allocator_free(&world->thread_bumps[i]);
  }
  free(world->particles);
  free(world->particles_swap);
}

// tom mohr's particle life kernel...
static inline float pl_kernel(float r, float a, float beta) {
  if (r < beta) {
    return r / beta - 1.0f;
  }
  if (r < 1.0f) {
    return a * (1.0f - fabsf(2.0f * r - 1.0f - beta) / (1.0f - beta));
  }
  return 0.0f;
}

// interaction radius in world units
#define PL_CUTOFF (0.05f)
// repulsion zone as fraction of cutoff
#define PL_BETA (0.30f)
// global force strength
#define PL_FORCE_SCALE (2.0f)
// velocity damping rate (1/s)
#define PL_FRICTION (8.0f)
// spasher cell must be >=PL_CUTOFF; 3x3 query covers +-1 cell
#define PL_CELL_SIZE (PL_CUTOFF)

static struct particle step_particle(
  const struct particle *p,
  const void **neighbours,
  size_t neighbours_len,
  const float force_matrix[SPECIES_COUNT][SPECIES_COUNT],
  float dt
) {
  struct particle out = *p;

  float ax = 0.0f;
  float ay = 0.0f;

  // precompute constants
  const float damp = expf(-PL_FRICTION * dt);

  for (size_t i = 0; i < neighbours_len; ++i) {
    const struct particle *n = (const struct particle *)neighbours[i];
    float dx = n->pos.x - p->pos.x;
    float dy = n->pos.y - p->pos.y;
    float dist_sq = dx * dx + dy * dy;
    if (dist_sq <= 0.0f || dist_sq >= PL_CUTOFF * PL_CUTOFF) {
      continue;
    }
    float dist = sqrtf(dist_sq);
    float r = dist / PL_CUTOFF;
    float a = force_matrix[p->species][n->species];
    float f = pl_kernel(r, a, PL_BETA);
    // unit vector toward neighbour * force magnitude
    float inv = 1.0f / dist;
    ax += f * dx * inv;
    ay += f * dy * inv;
  }

  ax *= PL_FORCE_SCALE;
  ay *= PL_FORCE_SCALE;

  // semi implicit euler with exponential friction
  out.vel.x = (out.vel.x + ax * dt) * damp;
  out.vel.y = (out.vel.y + ay * dt) * damp;

  // integrate position wrt deltatime
  out.pos.x += out.vel.x * dt;
  out.pos.y += out.vel.y * dt;

  // wall bouncing
  if (out.pos.x < 0.0f) {
    out.pos.x = -out.pos.x;
    out.vel.x = -out.vel.x;
  }
  if (out.pos.x > WORLD_WIDTH) {
    out.pos.x = 2.0f * WORLD_WIDTH - out.pos.x;
    out.vel.x = -out.vel.x;
  }
  if (out.pos.y < 0.0f) {
    out.pos.y = -out.pos.y;
    out.vel.y = -out.vel.y;
  }
  if (out.pos.y > WORLD_HEIGHT) {
    out.pos.y = 2.0f * WORLD_HEIGHT - out.pos.y;
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
  // per-thread bump for intermediate allocations
  struct bump_allocator *bump;
  // readonly force matrix
  const float (*force_matrix)[SPECIES_COUNT];
};

static void chunk_particle_update(void *arg) {
  assert(arg);
  struct particle_chunk_task *task = (struct particle_chunk_task *)arg;
  for (size_t i = task->start; i < task->end; ++i) {
    struct bump_allocator_checkpoint cp = bump_allocator_save(task->bump);
    const struct particle *p = &task->buffer[i];
    size_t neighbours_len = 0;
    const void **neighbours =
      spatial_hasher_query(task->sh, task->bump, p, &neighbours_len);
    task->swap[i] = step_particle(
      p,
      neighbours,
      neighbours_len,
      task->force_matrix,
      task->dt
    );
    bump_allocator_restore(task->bump, cp);
  }
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

#define RADIX_BITS (8)
#define RADIX_SIZE (1 << RADIX_BITS)
#define RADIX_MASK (RADIX_SIZE - 1)

static uint32_t particle_cell_key(const struct particle *p, float cell_size) {
  int32_t cx = (int32_t)(p->pos.x / cell_size);
  int32_t cy = (int32_t)(p->pos.y / cell_size);
  return (uint32_t)cell_hash(cx, cy);
}

static void radix_sort_particles(
  struct particle *particles,
  size_t particles_len,
  struct bump_allocator *bump,
  float cell_size
) {
  uint32_t *keys = bump_allocator_alloc(
    bump,
    particles_len * sizeof(uint32_t),
    sizeof(uint32_t)
  );
  uint32_t *keys_tmp = bump_allocator_alloc(
    bump,
    particles_len * sizeof(uint32_t),
    sizeof(uint32_t)
  );
  uint32_t *indices = bump_allocator_alloc(
    bump,
    particles_len * sizeof(uint32_t),
    sizeof(uint32_t)
  );
  uint32_t *indices_tmp = bump_allocator_alloc(
    bump,
    particles_len * sizeof(uint32_t),
    sizeof(uint32_t)
  );
  struct particle *gathered = bump_allocator_alloc(
    bump,
    particles_len * sizeof(struct particle),
    sizeof(void *)
  );

  for (size_t i = 0; i < particles_len; ++i) {
    keys[i] = particle_cell_key(&particles[i], cell_size);
    indices[i] = (uint32_t)i;
  }

  // fuse histograms, build all 4 in one pass over keys
  // - https://en.wikipedia.org/wiki/Radix_sort#Least_significant_digit
  size_t counts[4][RADIX_SIZE] = {{0}};
  for (size_t i = 0; i < particles_len; ++i) {
    uint32_t k = keys[i];
    counts[0][(k >> 0) & RADIX_MASK]++;
    counts[1][(k >> 8) & RADIX_MASK]++;
    counts[2][(k >> 16) & RADIX_MASK]++;
    counts[3][(k >> 24) & RADIX_MASK]++;
  }

  uint32_t *ksrc = keys, *kdst = keys_tmp;
  uint32_t *isrc = indices, *idst = indices_tmp;

  for (int pass = 0; pass < 4; ++pass) {
    int shift = pass * RADIX_BITS;

    // prefix sum from precomputed histogram
    size_t offsets[RADIX_SIZE];
    offsets[0] = 0;
    for (size_t i = 1; i < RADIX_SIZE; ++i) {
      offsets[i] = offsets[i - 1] + counts[pass][i - 1];
    }

    // scatter key index pairs
    for (size_t i = 0; i < particles_len; ++i) {
      size_t bucket = (ksrc[i] >> shift) & RADIX_MASK;
      size_t j = offsets[bucket]++;
      kdst[j] = ksrc[i];
      idst[j] = isrc[i];
    }

    uint32_t *ktmp = ksrc;
    ksrc = kdst;
    kdst = ktmp;
    uint32_t *itmp = isrc;
    isrc = idst;
    idst = itmp;
  }

  // reordering particles using sorted indices...
  for (size_t i = 0; i < particles_len; ++i) {
    gathered[i] = particles[isrc[i]];
  }
  memcpy(particles, gathered, particles_len * sizeof(struct particle));
}

void world_step(
  struct world *world,
  struct bump_allocator *bump,
  struct thread_pool *pool,
  float dt
) {
  assert(world);
  assert(bump);
  assert(pool);

  // sort particles by cell so spasher chases fewer pages
  radix_sort_particles(
    world->particles,
    world->particles_len,
    bump,
    PL_CELL_SIZE
  );

  // use bump context to create a transient hasher
  struct spatial_hasher sh;
  spatial_hasher_init(
    &sh,
    bump,
    PL_CELL_SIZE,
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
      .bump = &world->thread_bumps[i],
      .dt = dt,
      .force_matrix = world->force_matrix,
    };
    thread_pool_add_work(pool, chunk_particle_update, &tasks[i]);
    curr = end;
  }

  thread_pool_wait(pool);

  // clean bumps; threads dont know about possible existing allocations...
  for (size_t i = 0; i < WORLD_THREAD_COUNT; ++i) {
    bump_allocator_clear(&world->thread_bumps[i]);
  }

  swap_buffers(world);
}
