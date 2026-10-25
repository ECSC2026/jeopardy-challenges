
#pragma once

#include "system/surface.h"

#include <SDL2/SDL_ttf.h>


class Font {
public:
    Font();
    ~Font();

    bool initialize(const uint8_t *data, size_t size, int fontsize);
    Surface *render(const char *text, uint8_t r, uint8_t g, uint8_t b);
    Surface *render(char text, uint8_t r, uint8_t g, uint8_t b);

private:
    TTF_Font *font;
};
