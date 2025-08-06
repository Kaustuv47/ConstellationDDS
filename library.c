#include "library.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <windows.h>

#define PORT 47474
#define MAX_BUFFER_SIZE 65535
#define MAX_CLIENTS 100

typedef struct {
    clientInfoStructure *info;
    DeserializerFunctionPointer handler;
} threadedClientStructure;

static struct {
    char ip[INET_ADDRSTRLEN];
    HANDLE thread;
} knownClientStructure[MAX_CLIENTS];

static int clientCount = 0;

unsigned __stdcall ClientReceiverThread(void *arg) {
    threadedClientStructure *args = (threadedClientStructure *)arg;
    clientInfoStructure *info = args->info;
    DeserializerFunctionPointer deserializerFunctionPointer = args->handler;

    char buffer[MAX_BUFFER_SIZE];

    while (1) {
        struct sockaddr_in recv_addr;
        int addr_len = sizeof(recv_addr);

        int bytes_received = recvfrom(
            info->socket,
            buffer,
            sizeof(buffer),
            0,
            (struct sockaddr *)&recv_addr,
            &addr_len
        );

        if (bytes_received <= 0) continue;

        if (memcmp(&recv_addr, &info->address, sizeof(struct sockaddr_in)) == 0) {
            deserializerFunctionPointer(buffer, bytes_received, info);
        }
    }

    free(info);
    free(args);
    return 0;
}

int CheckClientExistence(const char *ip) {
    for (int i = 0; i < clientCount; ++i) {
        if (strcmp(knownClientStructure[i].ip, ip) == 0) return 1;
    }
    return 0;
}

void AddClient(const char *ip, HANDLE thread) {
    if (clientCount >= MAX_CLIENTS) return;
    strcpy(knownClientStructure[clientCount].ip, ip);
    knownClientStructure[clientCount].thread = thread;
    clientCount++;
}

unsigned __stdcall Receiver(void *arg) {
    DeserializerFunctionPointer handler = (DeserializerFunctionPointer)arg;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) return 0;

    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        WSACleanup();
        return 0;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(udpSocket);
        WSACleanup();
        return 0;
    }

    char buffer[MAX_BUFFER_SIZE];

    while (1) {
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);

        int bytes_received = recvfrom(
            udpSocket,
            buffer,
            sizeof(buffer),
            MSG_PEEK,
            (struct sockaddr *)&client_addr,
            &addr_len
        );

        if (bytes_received <= 0) continue;

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip, sizeof(ip));

        if (!CheckClientExistence(ip)) {
            clientInfoStructure *info = malloc(sizeof(clientInfoStructure));
            info->socket = udpSocket;
            info->address = client_addr;

            threadedClientStructure *args = malloc(sizeof(threadedClientStructure));
            args->info = info;
            args->handler = handler;

            HANDLE th = (HANDLE)_beginthreadex(NULL, 0, ClientReceiverThread, args, 0, NULL);
            AddClient(ip, th);
        }

        Sleep(50);
    }

    closesocket(udpSocket);
    WSACleanup();
    return 0;
}




// ===================================================
#define MAX_TRANSMIT_CLIENTS 100

typedef struct {
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in address;
    SOCKET socket;
} TransmitClientInfo;

static TransmitClientInfo transmitClients[MAX_TRANSMIT_CLIENTS];
static int transmitClientCount = 0;

void Transmitter(const char *ip, const char *data, int length) {
    // Initialize WinSock once
    static int initialized = 0;
    if (!initialized) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return;
        initialized = 1;
    }

    // Check if IP already has a socket
    for (int i = 0; i < transmitClientCount; ++i) {
        if (strcmp(transmitClients[i].ip, ip) == 0) {
            sendto(
                transmitClients[i].socket,
                data,
                length,
                0,
                (struct sockaddr *)&transmitClients[i].address,
                sizeof(transmitClients[i].address)
            );
            return;
        }
    }

    // If new IP, create socket and store
    if (transmitClientCount >= MAX_TRANSMIT_CLIENTS) return;

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) return;

    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(PORT);
    inet_pton(AF_INET, ip, &dest.sin_addr);

    TransmitClientInfo *entry = &transmitClients[transmitClientCount++];
    strncpy(entry->ip, ip, INET_ADDRSTRLEN);
    entry->address = dest;
    entry->socket = sock;

    sendto(
        sock,
        data,
        length,
        0,
        (struct sockaddr *)&dest,
        sizeof(dest)
    );
}


void InitiateConstellation(DeserializerFunctionPointer deserializerFunctionPointer) {
    HANDLE threadHandler = (HANDLE)_beginthreadex(NULL, 0, Receiver, (void *)deserializerFunctionPointer, 0, NULL);
    if (threadHandler) CloseHandle(threadHandler);
}

