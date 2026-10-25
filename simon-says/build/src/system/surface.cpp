
#include "system/surface.h"

#include <stdexcept>

#include <SDL2/SDL.h>

Surface::Surface() {
    surface = nullptr;
    allocated = false;
}

Surface::~Surface() {
    if (surface && allocated) {
        SDL_FreeSurface(surface);
    }
}

void Surface::initialize(
    SDL_Surface *surface, int width, int height, bool allocated
) {
    this->surface = surface;
    this->width = width;
    this->height = height;
    this->allocated = allocated;
}

bool Surface::create(void *data, int width, int height) {
    surface = SDL_CreateRGBSurfaceWithFormatFrom(
        data, width, height, 32, width * 4, SDL_PIXELFORMAT_RGBA32
    );
    if (!surface) {
        return false;
    }

    allocated = true;
    this->width = width;
    this->height = height;
    return true;
}

bool Surface::draw(Surface *target, int x, int y) {
    SDL_Rect rect = {x, y, target->width, target->height};
    return !SDL_BlitSurface(
        surface, NULL, target->surface, &rect
    );
}

bool Surface::draw_scaled(
    Surface *target, int x, int y, int width, int height
) {
    SDL_Rect rect = {x, y, width, height};
    return !SDL_BlitScaled(
        surface, NULL, target->surface, &rect
    );
}
