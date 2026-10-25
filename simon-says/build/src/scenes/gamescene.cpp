
#include "client/facade.h"

#include "scenes/gamescene.h"

#include "app.h"
#include "debug.h"

#include <arpa/inet.h>
#include <bearssl.h>


GameScene::GameScene() {
    attention = nullptr;
    repeat = nullptr;
    level = nullptr;
    error_text = nullptr;
    try_again = nullptr;
    congrats = nullptr;
    flag = nullptr;
}

GameScene::~GameScene() {
    if (attention) delete attention;
    if (repeat) delete repeat;
    if (level) delete level;
    if (error_text) delete error_text;
    if (try_again) delete try_again;
    if (congrats) delete congrats;
    if (flag) delete flag;
}

bool GameScene::initialize(AssetLoader *loader) {
    background.initialize(loader);

    if (!buttons[0].initialize(
        loader, TextureID_RedPressed, TextureID_RedUnpressed, SoundID_1, '1'
    )) return false;

    if (!buttons[1].initialize(
        loader, TextureID_GreenPressed, TextureID_GreenUnpressed, SoundID_2, '2'
    )) return false;

    if (!buttons[2].initialize(
        loader, TextureID_BluePressed, TextureID_BlueUnpressed, SoundID_3, '3'
    )) return false;

    if (!buttons[3].initialize(
        loader, TextureID_YellowPressed, TextureID_YellowUnpressed,
        SoundID_4, '4'
    )) return false;

    font = loader->font(48);
    if (!font) {
        return false;
    }

    attention = font->render("Pay attention", 255, 255, 255);
    if (!attention) {
        return false;
    }

    repeat = font->render("Repeat!", 255, 255, 255);
    if (!repeat) {
        return false;
    }

    error_text = font->render("Wrong!", 255, 255, 255);
    if (!error_text) {
        return false;
    }

    try_again = font->render("Press enter to try again", 255, 255, 255);
    if (!try_again) {
        return false;
    }

    congrats = font->render("Congratulations!", 255, 255, 255);
    if (!congrats) {
        return false;
    }

    return true;
}

void GameScene::reset() {
    if (level) {
        delete level;
        level = nullptr;
    }

    state = GameState::WaitForResponse;
    count = 0;
    play_index = 0;
    play_frame = 0;
}

bool GameScene::update(
    EventMgr *eventmgr, ClientFacade *client, App *app
) {
    for (int i = 0; i < 4; i++) {
        buttons[i].update();
    }

    if (state == GameState::WaitForResponse) {
        return wait_for_response(client, app);
    }
    else if (state == GameState::PlayColors) {
        play_colors();
    }
    else if (state == GameState::WaitUserInput) {
        wait_user_input(eventmgr, client);
    }
    else if (state == GameState::Delay) {
        timer--;
        if (timer == 0) {
            state = next_state;
        }
    }
    else if (state == GameState::Error) {
        if (eventmgr->pressed[Key_Enter]) {
            client->reset_game();
            reset();
        }
    }
    else if (state == GameState::Success) {
        return wait_for_response(client, app);
    }
    return true;
}

bool GameScene::draw(Surface *window) {
    if (!background.draw(window)) return false;

    int center_x = window->width / 2;
    int center_y = window->height / 2;

    if (state == GameState::Flag) {
        if (!congrats->draw(
            window,
            center_x - congrats->width / 2,
            center_y - congrats->height / 2 - 75
        )) {
            return false;
        }

        if (!flag->draw(
            window,
            center_x - flag->width / 2,
            center_y - flag->height / 2 + 75
        )) {
            return false;
        }

        return true;
    }

    for (int i = 0; i < 4; i++) {
        if (!buttons[i].draw(window, center_x - 120 + 80 * i, center_y)) {
            return false;
        }
    }

    Surface *upper = nullptr;
    Surface *lower = nullptr;

    if (state == GameState::Error) {
        upper = error_text;
    }
    else {
        upper = level;
    }

    if (state == GameState::PlayColors) {
        lower = attention;
    }
    else if (state == GameState::WaitUserInput || state == GameState::Delay) {
        lower = repeat;
    }
    else if (state == GameState::Error) {
        lower = try_again;
    }

    if (upper) {
        if (!upper->draw(
            window,
            center_x - upper->width / 2,
            center_y - upper->height / 2 - 150
        )) {
            return false;
        }
    }

    if (lower) {
        if (!lower->draw(
            window,
            center_x - lower->width / 2,
            center_y - lower->height / 2 + 150
        )) {
            return false;
        }
    }

    return true;
}

bool GameScene::wait_for_response(ClientFacade *client, App *app) {
    ClientResponse response;
    if (client->check_response(&response)) {
        if (response.type == ClientResponseType::ConnectionFailed) {
            DEBUG_PRINT("Connection failed, switching to error scene");
            app->change_scene(SceneType::ConnectionFailure);
        }
        else if (response.type == ClientResponseType::ColorReceived) {
            DEBUG_PRINT("Received color from server");
            
            if (count >= MAX_COLORS) {
                return false;
            }

            colors[count++] = response.color;
            state = GameState::PlayColors;

            char text[32];
            snprintf(text, sizeof(text), "Level %i", count);

            if (level) {
                delete level;
            }

            level = font->render(text, 255, 255, 255);
            if (!level) {
                return false;
            }
        }
        else if (response.type == ClientResponseType::FlagReceived) {
            DEBUG_PRINT("Received flag from server");

            state = GameState::Flag;

            flag = font->render(response.flag, 255, 255, 255);
            if (!flag) {
                return false;
            }
        }
        else {
            DEBUG_PRINT("Received unexpected response from server");
            return false;
        }
    }
    return true;
}

void GameScene::wait_user_input(EventMgr *eventmgr, ClientFacade *client) {
    if (eventmgr->pressed[Key_1]) enter_color(Color_Red, client);
    else if (eventmgr->pressed[Key_2]) enter_color(Color_Green, client);
    else if (eventmgr->pressed[Key_3]) enter_color(Color_Blue, client);
    else if (eventmgr->pressed[Key_4]) enter_color(Color_Yellow, client);
}

void GameScene::enter_color(Color color, ClientFacade *client) {
    for (int i = 0; i < 4; i++) {
        buttons[i].unpress();
    }
    
    int index = color_to_index(color);
    buttons[index].press();

    if (color != colors[input_size]) {
        timer = 30;
        state = GameState::Delay;
        next_state = GameState::Error;
    }
    else {
        input[input_size++] = color;
        if (input_size == count) {
            send_solution(client);
            timer = 30;
            state = GameState::Delay;
            next_state = GameState::Success;
        }
    }
}

void GameScene::send_solution(ClientFacade *client) {
    for (size_t i = 0; i < input_size; i++) {
        input[i] = htonl(input[i]);
    }

    uint8_t hash[64];
    br_sha512_context sha512;
    br_sha512_init(&sha512);
    br_sha512_update(&sha512, input, input_size * 4);
    br_sha512_out(&sha512, hash);

    client->solve_level(hash);
}

void GameScene::play_colors() {
    if (play_frame == 0 && play_index > 0) {
        Color color = colors[play_index - 1];
        int index = color_to_index(color);
        buttons[index].press();
    }

    play_frame++;
    if (play_frame == 30) {
        play_frame = 0;
        play_index++;
        if (play_index > count) {
            play_index = 0;
            input_size = 0;
            state = GameState::WaitUserInput;
        }
    }
}

int GameScene::color_to_index(Color color) {
    if (color == Color_Red) return 0;
    else if (color == Color_Green) return 1;
    else if (color == Color_Blue) return 2;
    return 3;
}
