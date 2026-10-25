
#pragma once

#include "components/background.h"
#include "components/button.h"

#include "scenes/scene.h"

#include "system/font.h"
#include "system/surface.h"

#include "assetloader.h"


#define MAX_COLORS 1024


enum class GameState {
    WaitForResponse,
    PlayColors,
    WaitUserInput,
    Delay,
    Error,
    Success,
    Flag
};


class GameScene : public Scene {
public:
    GameScene();
    ~GameScene();

    void reset();
    bool initialize(AssetLoader *loader);
    bool update(EventMgr *eventmgr, ClientFacade *client, App *app);
    bool draw(Surface *window);

private:
    bool wait_for_response(ClientFacade *client, App *app);
    void play_colors();

    void wait_user_input(EventMgr *eventmgr, ClientFacade *client);
    void enter_color(Color color, ClientFacade *client);
    void send_solution(ClientFacade *client);

    int color_to_index(Color color);

    GameState state;
    GameState next_state;

    Background background;

    Button buttons[4];

    Color colors[MAX_COLORS];
    uint32_t input[MAX_COLORS];
    int count;
    int input_size;

    int play_index;
    int play_frame;

    int timer;

    Font *font;

    Surface *attention;
    Surface *repeat;
    Surface *level;
    Surface *error_text;
    Surface *try_again;
    Surface *congrats;
    Surface *flag;
};
