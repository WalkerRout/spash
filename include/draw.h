#ifndef _DRAW_H
#define _DRAW_H

#include <stdint.h>

#include <SDL2/SDL.h>

void draw_circle(SDL_Renderer* renderer, uint32_t center_x, uint32_t center_y, uint32_t radius);

#endif // _DRAW_H
