
#pragma once

#include "client/facade.h"

#include "scenes/connectionscene.h"
#include "scenes/errorscene.h"
#include "scenes/gamescene.h"
#include "scenes/scene.h"

#include "system/eventmgr.h"
#include "system/window.h"

#include "assetloader.h"


enum class SceneType {
    Connecting,
    ConnectionFailure,
    Game
};


class App {
public:
    bool initialize(const char *ip, int port);

    void run();

    void change_scene(SceneType type);

    void reconnect();
    void start_game();

private:
    void reset();
    bool update();

    Window window;
    EventMgr eventmgr;

    ClientFacade client;
    AssetLoader loader;

    ConnectionScene connection_scene;
    ErrorScene error_scene;
    GameScene game_scene;

    Scene *active_scene;
};
