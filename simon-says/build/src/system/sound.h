
#pragma once

#include <SDL2/SDL_mixer.h>


class Sound {
public:
    Sound();
    ~Sound();

    bool load(const void *data, size_t size);

    void play(int channel = -1);

private:
    Mix_Chunk *sound;
};
