
#include "client/facade.h"

#include "scenes/connectionscene.h"

#include "app.h"
#include "debug.h"


ConnectionScene::ConnectionScene() {
    text = nullptr;
}

ConnectionScene::~ConnectionScene() {
    if (text) {
        delete text;
    }
}

bool ConnectionScene::initialize(AssetLoader *loader) {
    background.initialize(loader);
    spinner.initialize(loader);

    Font *font = loader->font(48);
    if (!font) {
        return false;
    }

    text = font->render("Establishing connection...", 255, 255, 255);
    if (!text) {
        return false;
    }

    return true;
}

bool ConnectionScene::update(
    EventMgr *eventmgr, ClientFacade *client, App *app
) {
    spinner.update();

    ClientResponse response;
    if (client->check_response(&response)) {
        if (response.type == ClientResponseType::ConnectionSuccess) {
            DEBUG_PRINT("Connection succeeded, starting game");
            app->start_game();
        }
        else if (response.type == ClientResponseType::ConnectionFailed) {
            DEBUG_PRINT("Connection failed, switching to error scene");
            app->change_scene(SceneType::ConnectionFailure);
        }
        else {
            return false;
        }
    }

    return true;
}

bool ConnectionScene::draw(Surface *window) {
    if (!background.draw(window)) return false;
    if (!spinner.draw(window)) return false;

    int x = window->width / 2 - text->width / 2;
    int y = window->height / 2 - text->height / 2;
    if (!text->draw(window, x, y - 50)) return false;

    return true;
}
