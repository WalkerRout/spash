#include "particle.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "mvla/mvla.h"

// abbreve for compound literal syntax
struct particle
particle_new(float radius, v2f_t pos, v2f_t vel, enum species species) {
  return (struct particle) {
    .radius = radius,
    .pos = pos,
    .vel = vel,
    .species = species,
  };
}

uint8_t particle_is_colliding(struct particle a, struct particle b) {
  float x_dist = a.pos.x - b.pos.x;
  float y_dist = a.pos.y - b.pos.y;
  float min_touch_dist = a.radius + b.radius;
  // square everything so we dont have to use sqrt
  return x_dist * x_dist + y_dist * y_dist < min_touch_dist * min_touch_dist;
}
