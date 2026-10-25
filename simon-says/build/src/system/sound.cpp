
#include "system/sound.h"

#include "debug.h"


Sound::Sound() {
    sound = nullptr;
}

Sound::~Sound() {
    if (sound) {
        Mix_FreeChunk(sound);
    }
}

bool Sound::load(const void *data, size_t size) {
    SDL_RWops *stream = SDL_RWFromConstMem(data, size);
    if (!stream) {
        return false;
    }

    sound = Mix_LoadWAV_RW(stream, true);
    if (!sound) {
        DEBUG_PRINT("Mix_LoadWAV_RW failed: %s", SDL_GetError());
        return false;
    }

    return true;
}

void Sound::play(int channel) {
    Mix_PlayChannel(channel, sound, 0);
}
