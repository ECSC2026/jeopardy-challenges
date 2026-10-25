
#include "system/window.h"

#include <stdexcept>


Window::Window() {
    window = nullptr;
}

Window::~Window() {
    if (window) {
        SDL_DestroyWindow(window);
    }
}

bool Window::initialize(
    int width, int height, const char *title, bool resizable
) {
    int flags = 0;
    if (resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }

	window = SDL_CreateWindow(
        title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, flags
    );
    if (!window) {
        return false;
    }

    window_surface = SDL_GetWindowSurface(window);
    if (!window_surface) {
        return false;
    }

    surface.initialize(window_surface, width, height);
    return true;
}

bool Window::update(EventMgr *eventmgr) {
    if (!eventmgr->resized) {
        return true;
    }

    window_surface = SDL_GetWindowSurface(window);
    if (!window_surface) {
        return false;
    }

    surface.initialize(
        window_surface, eventmgr->resize_width, eventmgr->resize_height
    );
    return true;
}

bool Window::clear(int r, int g, int b) {
    uint32_t color = SDL_MapRGB(window_surface->format, r, g, b);
    return !SDL_FillRect(window_surface, NULL, color);
}

bool Window::swap() {
    return !SDL_UpdateWindowSurface(window);
}
