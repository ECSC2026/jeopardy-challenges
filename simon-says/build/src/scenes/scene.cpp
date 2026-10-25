
#include "scenes/scene.h"

Scene::~Scene() {}

bool Scene::initialize(AssetLoader *loader) { return true; }
bool Scene::update(EventMgr *eventmgr, ClientFacade *client, App *app) {
    return true;
}
bool Scene::draw(Surface *window) { return true; }
