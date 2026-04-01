#include "draw.h"

void draw_circle(SDL_Renderer *renderer, uint32_t center_x, uint32_t center_y,
                 uint32_t radius) {
  const int32_t diameter = (radius * 2);

  int32_t x = (radius - 1);
  int32_t y = 0;
  int32_t tx = 1;
  int32_t ty = 1;
  int32_t error = (tx - diameter);

  while (x >= y) {
    //  Each of the following renders an octant of the circle
    (void)SDL_RenderDrawPoint(renderer, center_x + x, center_y - y);
    (void)SDL_RenderDrawPoint(renderer, center_x + x, center_y + y);
    (void)SDL_RenderDrawPoint(renderer, center_x - x, center_y - y);
    (void)SDL_RenderDrawPoint(renderer, center_x - x, center_y + y);
    (void)SDL_RenderDrawPoint(renderer, center_x + y, center_y - x);
    (void)SDL_RenderDrawPoint(renderer, center_x + y, center_y + x);
    (void)SDL_RenderDrawPoint(renderer, center_x - y, center_y - x);
    (void)SDL_RenderDrawPoint(renderer, center_x - y, center_y + x);

    if (error <= 0) {
      ++y;
      error += ty;
      ty += 2;
    }

    if (error > 0) {
      --x;
      tx += 2;
      error += (tx - diameter);
    }
  }
}
