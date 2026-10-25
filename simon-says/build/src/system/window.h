
#pragma once

#include "system/eventmgr.h"
#include "system/surface.h"

#include <SDL2/SDL.h>

class Window {
public:
    Window();
    ~Window();

    bool initialize(
        int width, int height, const char *title, bool resizable = false
    );

    bool clear(int r, int g, int b);
    bool swap();

    bool update(EventMgr *eventmgr);

    Surface surface;

private:
    SDL_Window *window;
    SDL_Surface *window_surface;
};
