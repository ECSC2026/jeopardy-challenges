
#pragma once

#include "client/facade.h"

#include "system/eventmgr.h"
#include "system/surface.h"

#include "assetloader.h"


class App;

class Scene {
public:
    virtual ~Scene();
    virtual bool initialize(AssetLoader *loader);
    virtual bool update(EventMgr *eventmgr, ClientFacade *client, App *app);
    virtual bool draw(Surface *window);
};
