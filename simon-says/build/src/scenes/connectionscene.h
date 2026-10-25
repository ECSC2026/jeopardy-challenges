
#pragma once

#include "components/background.h"
#include "components/spinner.h"

#include "scenes/scene.h"

#include "system/surface.h"

#include "assetloader.h"


class ConnectionScene : public Scene {
public:
    ConnectionScene();
    ~ConnectionScene();

    bool initialize(AssetLoader *loader);
    bool update(EventMgr *eventmgr, ClientFacade *client, App *app);
    bool draw(Surface *window);

private:
    Background background;
    Spinner spinner;

    Surface *text;
};
