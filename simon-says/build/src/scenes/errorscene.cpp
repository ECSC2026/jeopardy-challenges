
#include "client/facade.h"

#include "scenes/errorscene.h"

#include "app.h"
#include "debug.h"


ErrorScene::ErrorScene() {
    text1 = nullptr;
    text2 = nullptr;
}

ErrorScene::~ErrorScene() {
    if (text1) delete text1;
    if (text2) delete text2;
}

bool ErrorScene::initialize(AssetLoader *loader) {
    background.initialize(loader);

    Font *font = loader->font(48);
    if (!font) {
        return false;
    }

    text1 = font->render("A communication error occurred.", 255, 255, 255);
    if (!text1) {
        return false;
    }

    text2 = font->render("Press enter to retry...", 255, 255, 255);
    if (!text2) {
        return false;
    }

    return true;
}

bool ErrorScene::update(
    EventMgr *eventmgr, ClientFacade *client, App *app
) {
    if (eventmgr->pressed[Key_Enter]) {
        app->reconnect();
    }
    return true;
}

bool ErrorScene::draw(Surface *window) {
    if (!background.draw(window)) return false;

    int x = window->width / 2 - text1->width / 2;
    int y = window->height / 2 - text1->height / 2;
    if (!text1->draw(window, x, y - 40)) return false;

    x = window->width / 2 - text2->width / 2;
    y = window->height / 2 - text2->height / 2;
    if (!text2->draw(window, x, y + 40)) return false;

    return true;
}
