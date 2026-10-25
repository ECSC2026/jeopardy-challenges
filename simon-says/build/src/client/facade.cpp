
#include "client/facade.h"

#include "debug.h"

#include <cstring>


ClientFacade::ClientFacade() {
    client = nullptr;
}

ClientFacade::~ClientFacade() {
    if (client) {
        delete client;
    }
}

void ClientFacade::initialize(const char *ip, int port) {
    this->ip = ip;
    this->port = port;

    has_response = false;

    thread = std::thread(&ClientFacade::thread_loop, this);

    reconnect();
}

void ClientFacade::reconnect() {
    request.type = ClientRequestType::Reconnect;
    wakeup();
}

void ClientFacade::reset_game() {
    request.type = ClientRequestType::ResetGame;
    wakeup();
}

void ClientFacade::solve_level(const void *hash) {
    request.type = ClientRequestType::SolveLevel;
    memcpy(request.hash, hash, 64);

    wakeup();
}

void ClientFacade::wakeup() {
    active.store(true);
    active.notify_one();
}

bool ClientFacade::check_response(ClientResponse *response) {
    if (!has_response) {
        return false;
    }

    has_response = false;
    *response = this->response;
    return true;
}

void ClientFacade::thread_loop() {
    while (true) {
        process_request();
    }
}

void ClientFacade::process_request() {
    active.wait(false);
    active.store(false);

    if (request.type == ClientRequestType::Reconnect) {
        handle_reconnect();
    }
    else if (request.type == ClientRequestType::ResetGame) {
        handle_reset_game();
    }
    else if (request.type == ClientRequestType::SolveLevel) {
        handle_solve_level();
    }
}

void ClientFacade::handle_reconnect() {
    client = new GameClient();
    DEBUG_PRINT("Connecting to game server...");
    if (client->connect(ip, port)) {
        resolve_success(ClientResponseType::ConnectionSuccess);
    }
    else {
        resolve_connection_failure();
    }
}

void ClientFacade::handle_reset_game() {
    if (client->reset_game(&response.color)) {
        DEBUG_PRINT("Reset game request succeeded");
        resolve_success(ClientResponseType::ColorReceived);
    }
    else {
        DEBUG_PRINT("Reset game request failed");
        resolve_connection_failure();
    }
}

void ClientFacade::handle_solve_level() {
    size_t flag_size = sizeof(response.flag);
    if (client->solve_level(
        request.hash, &response.color, response.flag, &flag_size)
    ) {
        if (flag_size) {
            resolve_success(ClientResponseType::FlagReceived);
        }
        else {
            resolve_success(ClientResponseType::ColorReceived);
        }
    }
    else {
        resolve_connection_failure();
    }
}

void ClientFacade::resolve_success(ClientResponseType type) {
    response.type = type;
    has_response = true;
}

void ClientFacade::resolve_connection_failure() {
    delete client;
    client = nullptr;

    response.type = ClientResponseType::ConnectionFailed;

    has_response = true;
}
