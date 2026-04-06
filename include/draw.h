#ifndef _DRAW_H
#define _DRAW_H

#include <SDL2/SDL.h>
#include <stdint.h>

void draw_circle(
  SDL_Renderer *renderer,
  int32_t center_x,
  int32_t center_y,
  int32_t radius
);

#endif // _DRAW_H
