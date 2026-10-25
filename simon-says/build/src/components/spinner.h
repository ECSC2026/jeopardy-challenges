
#pragma once

#include "system/eventmgr.h"
#include "system/surface.h"

#include "assetloader.h"


class Spinner {
public:
    void initialize(AssetLoader *loader);
    void update();
    bool draw(Surface *window);

private:
    bool draw_frame(Surface *window, int frame);
    
    Surface *surface;
    int frame;
};
