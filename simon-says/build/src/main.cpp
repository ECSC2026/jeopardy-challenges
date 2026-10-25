
#include "app.h"


int main(int argc, const char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    App *app = new App();
    if (!app->initialize(argv[1], atoi(argv[2]))) {
        return 1;
    }
    app->run();
    return 0;
}
