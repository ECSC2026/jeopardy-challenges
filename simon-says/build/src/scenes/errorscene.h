
#pragma once

#include "components/background.h"

#include "scenes/scene.h"

#include "system/surface.h"

#include "assetloader.h"


class ErrorScene : public Scene {
public:
    ErrorScene();
    ~ErrorScene();

    bool initialize(AssetLoader *loader);
    bool update(EventMgr *eventmgr, ClientFacade *client, App *app);
    bool draw(Surface *window);

private:
    Background background;

    Surface *text1;
    Surface *text2;
};
