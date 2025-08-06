#include "library.h"
#include <stdio.h>
#include <windows.h>

void Deserializer(const char *data, int len, void *client_context) {
    clientInfoStructure *info = (clientInfoStructure *)client_context;

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(info->address.sin_addr), ip, sizeof(ip));
    int port = ntohs(info->address.sin_port);

    printf("[From %s:%d] - %.*s\n", ip, port, len, data);
}

int main() {
    InitiateConstellation(Deserializer);

    while (1) {
        Sleep(1000);
        Transmitter("127.0.0.1", "Hello World!", strlen("Hello World!"));
    }

    return 0;
}

