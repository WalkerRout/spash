#include "draw.h"

void draw_circle(
  SDL_Renderer *renderer,
  int32_t cx,
  int32_t cy,
  int32_t radius
) {
  int32_t x = radius;
  int32_t y = 0;
  int32_t err = 1 - radius;

  while (x >= y) {
    (void)SDL_RenderDrawLine(renderer, cx - x, cy + y, cx + x, cy + y);
    (void)SDL_RenderDrawLine(renderer, cx - x, cy - y, cx + x, cy - y);
    (void)SDL_RenderDrawLine(renderer, cx - y, cy + x, cx + y, cy + x);
    (void)SDL_RenderDrawLine(renderer, cx - y, cy - x, cx + y, cy - x);

    ++y;
    if (err < 0) {
      err += 2 * y + 1;
    } else {
      --x;
      err += 2 * (y - x) + 1;
    }
  }
}
