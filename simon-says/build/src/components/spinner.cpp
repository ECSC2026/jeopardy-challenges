
#include "components/spinner.h"

#include <cmath>


void Spinner::initialize(AssetLoader *loader) {
    surface = loader->texture(TextureID_LoadingCircle);
    frame = 0;
}

void Spinner::update() {
    frame++;
    if (frame == 10) {
        frame = 0;
    }
}

bool Spinner::draw(Surface *window) {
    if (!draw_frame(window, frame)) return false;
    if (!draw_frame(window, frame + 10)) return false;
    if (!draw_frame(window, frame + 20)) return false;
    if (!draw_frame(window, frame + 30)) return false;
    if (!draw_frame(window, frame + 40)) return false;
    return true;
}

bool Spinner::draw_frame(Surface *window, int frame) {
    float angle = frame * M_PI * 2 / 50;
    int x = window->width / 2 + cosf(angle) * 20;
    int y = window->height / 2 + sinf(angle) * 20 + 50;
    return surface->draw_scaled(window, x - 10, y - 10, 20, 20);
}
