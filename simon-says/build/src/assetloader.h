
#pragma once

#include "system/font.h"
#include "system/sound.h"
#include "system/surface.h"

#include <map>


enum SoundID {
    SoundID_1,
    SoundID_2,
    SoundID_3,
    SoundID_4,
    SoundID_End
};

enum TextureID {
    TextureID_BgWood,
    TextureID_LoadingCircle,
    TextureID_RedPressed,
    TextureID_RedUnpressed,
    TextureID_GreenPressed,
    TextureID_GreenUnpressed,
    TextureID_BluePressed,
    TextureID_BlueUnpressed,
    TextureID_YellowPressed,
    TextureID_YellowUnpressed,
    TextureID_End
};


class AssetLoader {
public:
    ~AssetLoader();
    
    bool initialize();

    Font *font(int size);
    Sound *sound(SoundID type);
    Surface *texture(TextureID type);

private:
    Sound sounds[SoundID_End];
    Surface textures[TextureID_End];

    std::map<int, Font *> fonts;
};
