
#pragma once


#include "system/sound.h"
#include "system/surface.h"

#include "assetloader.h"


class Button {
public:
    Button();
    ~Button();

    bool initialize(
        AssetLoader *loader, TextureID pressed_id, TextureID unpressed_id,
        SoundID sound_id, char digit
    );

    void press();
    void unpress();
    
    void update();
    bool draw(Surface *window, int x, int y);

private:
    int timer;
    Surface *pressed;
    Surface *unpressed;
    Sound *sound;
    Surface *hint;
};
