
#pragma once

#include <SDL2/SDL.h>

class Surface {
public:
    Surface();
    ~Surface();

    void initialize(
        SDL_Surface *surface, int width, int height, bool allocated = false
    );
    bool create(void *data, int width, int height);

    bool draw(Surface *target, int x, int y);
    bool draw_scaled(Surface *target, int x, int y, int width, int height);

    SDL_Surface *surface;

    int width, height;

private:
    bool allocated;
};
