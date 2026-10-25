
#include "components/button.h"

Button::Button() {
    hint = nullptr;
}

Button::~Button() {
    if (hint) delete hint;
}

bool Button::initialize(
    AssetLoader *loader, TextureID pressed_id, TextureID unpressed_id,
    SoundID sound_id, char digit
) {
    timer = 0;
    
    pressed = loader->texture(pressed_id);
    unpressed = loader->texture(unpressed_id);

    sound = loader->sound(sound_id);

    Font *font = loader->font(48);
    if (!font) {
        return false;
    }

    hint = font->render(digit, 255, 255, 255);
    if (!hint) {
        return false;
    }

    return true;
}

void Button::press() {
    timer = 25;
    sound->play(0);
}

void Button::unpress() {
    timer = 0;
}

void Button::update() {
    if (timer) {
        timer--;
    }
}

bool Button::draw(Surface *window, int x, int y) {
    Surface *surface = timer ? pressed : unpressed;
    if (!surface->draw(
        window, x - surface->width / 2, y - surface->height / 2
    )) {
        return false;
    }

    if (!timer) {
        if (!hint->draw(window, x - hint->width / 2, y - hint->height / 2)) {
            return false;
        }
    }

    return true;
}
