
#pragma once

#include "client/game.h"

#include <atomic>
#include <thread>


enum class ClientRequestType {
    Reconnect,
    ResetGame,
    SolveLevel
};


enum class ClientResponseType {
    ConnectionSuccess,
    ConnectionFailed,
    ColorReceived,
    FlagReceived
};


struct ClientRequest {
    ClientRequestType type;
    uint8_t hash[64];
};


struct ClientResponse {
    ClientResponseType type;
    Color color;
    char flag[64];
};


class ClientFacade {
public:
    ClientFacade();
    ~ClientFacade();

    void initialize(const char *ip, int port);

    void reconnect();

    void reset_game();
    void solve_level(const void *hash);

    bool check_response(ClientResponse *response);

private:
    void wakeup();
    void thread_loop();
    void process_request();
    void handle_reconnect();
    void handle_reset_game();
    void handle_solve_level();
    void resolve_success(ClientResponseType type);
    void resolve_connection_failure();

    const char *ip;
    int port;

    ClientRequest request;
    ClientResponse response;

    bool has_response;

    std::atomic<bool> active;
    std::thread thread;

    GameClient *client;
};
