#ifndef _PARTICLE_H
#define _PARTICLE_H

#include <stdint.h>

#include "mvla/mvla.h"

enum species {
  SPECIES_RED = 0,
  SPECIES_GREEN,
  SPECIES_BLUE,
  SPECIES_YELLOW,
  SPECIES_MAGENTA,
  SPECIES_CYAN,
  // sentinel; must remain last
  SPECIES_COUNT,
};

struct particle {
  float radius;
  // x: [0, 1]
  // y: [0, 1]
  v2f_t pos;
  v2f_t vel;
  enum species species;
};

struct particle
particle_new(float radius, v2f_t pos, v2f_t vel, enum species species);
uint8_t particle_is_colliding(struct particle a, struct particle b);

#endif // _PARTICLE_H
