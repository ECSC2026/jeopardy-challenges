
#include "components/background.h"

void Background::initialize(AssetLoader *loader) {
    surface = loader->texture(TextureID_BgWood);
}

bool Background::draw(Surface *window) {
    int xoffs = window->width / 2 % surface->width - surface->width;
    int yoffs = window->height / 2 % surface->height - surface->height;
    for (int y = yoffs; y < window->height; y += surface->height) {
        for (int x = xoffs; x < window->width; x += surface->width) {
            if (!surface->draw(window, x, y)) return false;
        }
    }
    return true;
}
