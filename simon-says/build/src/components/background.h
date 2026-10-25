
#pragma once

#include "system/surface.h"

#include "assetloader.h"


class Background {
public:
    void initialize(AssetLoader *loader);
    bool draw(Surface *window);

private:
    Surface *surface;
};
