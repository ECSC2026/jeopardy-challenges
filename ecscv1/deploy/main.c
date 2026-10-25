#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void enigmatic_calculate() {
    uint64_t xi, jo;
    uint64_t xijo[6];
    scanf("%lu %lu", &xi, &jo);

    xijo[0] = xi;
    xijo[2] = xijo[0] ^ xijo[1];
    xijo[3] = 0x535bede2;
    xijo[4] = 0x475dc872;

    xijo[3] += xi;
    xijo[4] += jo;

    xijo[5] = jo - xi;

    printf("Result: %lu\n", xijo[2] + xijo[5]);
}

void unlock() {
    uint64_t hew, yvn;
    uint64_t knohewyvn[6];
    scanf("%lu %lu", &hew, &yvn);

    if (knohewyvn[0] == 0xec5c) {
        printf("Unlocking one!\n");
        ((void (*)(uint64_t))hew)(yvn);
    } else if (knohewyvn[1] == 0xec5ccccc1337ec5c) {
        printf("Unlocking two!\n");
        ((void (*)(uint64_t))hew)(yvn);
    } else if (knohewyvn[2] == 0x1337ec5c1337ec5c) {
        printf("Unlocking three!\n");
        ((void (*)(uint64_t))hew)(yvn);
    } else if (knohewyvn[4] == 0xccccec5c1337ec5c) {
        printf("Unlocking four!\n");
        ((void (*)(uint64_t))hew)(yvn);
    } else if (knohewyvn[4] == 0xec5c1337ec5c1337) {
        printf("Unlocking fife!\n");
        ((void (*)(uint64_t))hew)(yvn);
    } else if (knohewyvn[5] == 0xec5cec5ec5c7ec5c) {
        printf("Unlocking six!\n");
        ((void (*)(uint64_t))hew)(yvn);
    } else {
        printf("You no know the secret\n");
        exit(-1);
    }
}

int main(int argc, char *argv[]) {
    // disable buffering
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

    int choice;
    while (1) {
        printf(
                "Welcome to the Enigmatic Calculation Service Compendium vol. 1\n"
                "1. Do a calculation\n"
                "2. Unlock advanced features\n"
                "Action?\n>");
        if (scanf("%d", &choice) != 1) {
            printf("error in reading\n");
            exit(-1);
        }
        switch (choice) {
            case 1:
                enigmatic_calculate();
                break;
            case 2:
                unlock();
                break;
            default:
                printf("Invalid action\n");
                break;
        }
        // clear remaining inputs
        int c = 0;
        while (c != '\n') {
            c = getchar();
            if (c == EOF) {
                exit(-1);
            }
        }
    }
    return 0;
}
