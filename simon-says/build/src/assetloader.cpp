
#include "assetloader.h"
#include "assetdata.h"

#include <ranges>


AssetLoader::~AssetLoader() {
    for (Font *font : std::views::values(fonts)) {
        delete font;
    }
}


#define load_sound(name) \
    if (!sounds[SoundID_##name].load( \
        SoundData_##name, sizeof(SoundData_##name) \
    )) { return false; }

#define load_texture(name) \
    if (!textures[TextureID_##name].create( \
        TextureData_##name, TextureWidth_##name, TextureHeight_##name \
    )) { return false; }

bool AssetLoader::initialize() {
    load_sound(1);
    load_sound(2);
    load_sound(3);
    load_sound(4);

    load_texture(BgWood);
    load_texture(LoadingCircle);
    load_texture(RedPressed);
    load_texture(RedUnpressed);
    load_texture(GreenPressed);
    load_texture(GreenUnpressed);
    load_texture(BluePressed);
    load_texture(BlueUnpressed);
    load_texture(YellowPressed);
    load_texture(YellowUnpressed);
    return true;
}

Sound *AssetLoader::sound(SoundID id) {
    return &sounds[id];
}

Surface *AssetLoader::texture(TextureID id) {
    return &textures[id];
}

Font *AssetLoader::font(int size) {
    if (fonts.contains(size)) {
        return fonts[size];
    }

    Font *font = new Font();
    if (!font->initialize(FontData_Drybrush, sizeof(FontData_Drybrush), size)) {
        return nullptr;
    }

    fonts[size] = font;
    return font;
}
