#ifndef _PARTICLE_H
#define _PARTICLE_H

#include <stdint.h>

#include "mvla/mvla.h"

struct particle {
  float radius;
  // x: [0, 1]
  // y: [0, 1]
  v2f_t pos;
  // vx: [0, 1]
  // vy: [0, 1]
  v2f_t vel;
};

struct particle particle_new(float radius, v2f_t pos, v2f_t vel);
uint8_t particle_is_colliding(struct particle a, struct particle b);

#endif // _PARTICLE_H
