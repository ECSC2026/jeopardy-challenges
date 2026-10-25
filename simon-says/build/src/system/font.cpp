
#include "system/font.h"

#include "debug.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>


Font::Font() {
    font = nullptr;
}

Font::~Font() {
    if (font) {
        TTF_CloseFont(font);
    }
}

bool Font::initialize(const uint8_t *data, size_t size, int fontsize) {
    SDL_RWops *stream = SDL_RWFromConstMem(data, size);
    if (!stream) {
        return false;
    }

    font = TTF_OpenFontRW(stream, true, fontsize);
    if (!font) {
        DEBUG_PRINT("TTF_OpenFontRW failed: %s", SDL_GetError());
        return false;
    }

    return true;
}

Surface *Font::render(const char *text, uint8_t r, uint8_t g, uint8_t b) {
    SDL_Color color = {r, g, b, 255};
    SDL_Surface *sdl = TTF_RenderUTF8_Solid(font, text, color);
    if (!sdl) {
        return nullptr;
    }
    
    Surface *surface = new Surface();
    surface->initialize(sdl, sdl->w, sdl->h, true);
    return surface;
}

Surface *Font::render(char text, uint8_t r, uint8_t g, uint8_t b) {
    SDL_Color color = {r, g, b, 255};
    SDL_Surface *sdl = TTF_RenderGlyph_Solid(font, text, color);
    if (!sdl) {
        return nullptr;
    }
    
    Surface *surface = new Surface();
    surface->initialize(sdl, sdl->w, sdl->h);
    return surface;
}
