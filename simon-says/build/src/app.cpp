
#include "scenes/connectionscene.h"

#include "app.h"
#include "debug.h"

#include <unistd.h>


bool App::initialize(const char *ip, int port) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        DEBUG_PRINT("SDL_Init failed: %s", SDL_GetError());
		return false;
	}

    if (TTF_Init() < 0) {
        DEBUG_PRINT("TTF_Init failed: %s", SDL_GetError());
        return false;
    }

    if (Mix_OpenAudio(48000, AUDIO_S16LSB, 1, 480) != 0) {
        DEBUG_PRINT("MIX_OpenAudio failed: %s", SDL_GetError());
        return false;
    }
	
	Mix_AllocateChannels(4);

    if (!window.initialize(1024, 720, "Simon Says", true)) {
        return false;
    }

    if (!loader.initialize()) {
        return false;
    }

    if (!connection_scene.initialize(&loader)) return false;
    if (!error_scene.initialize(&loader)) return false;
    if (!game_scene.initialize(&loader)) return false;

    client.initialize(ip, port);

    change_scene(SceneType::Connecting);

    return true;
}

void App::change_scene(SceneType type) {
    if (type == SceneType::Connecting) {
        active_scene = &connection_scene;
    }
    else if (type == SceneType::ConnectionFailure) {
        active_scene = &error_scene;
    }
    else if (type == SceneType::Game) {
        active_scene = &game_scene;
    }
}

void App::reconnect() {
    client.reconnect();
    change_scene(SceneType::Connecting);
}

void App::start_game() {
    client.reset_game();
    game_scene.reset();
    change_scene(SceneType::Game);
}

void App::run() {
    while (update());
}

bool App::update() {
    if (!eventmgr.update()) return false;

    if (!window.update(&eventmgr)) return false;
    
    if (!window.clear(0, 0, 0)) return false;

    if (!active_scene->update(&eventmgr, &client, this)) return false;
    if (!active_scene->draw(&window.surface)) return false;

    if (!window.swap()) return false;

    usleep(20000);

    return true;
}
