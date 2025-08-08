#include "library.h"

#include <stdio.h>

#ifdef _WIN32
#include <process.h>
#include <ws2tcpip.h>
#include <windows.h>

#define INVALID_TRANSMITTER_SOCKET INVALID_SOCKET

#pragma comment(lib, "ws2_32.lib")
#elif __linux__
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define INVALID_TRANSMITTER_SOCKET -1
#endif

// #define PORT 47474

#ifdef _WIN32
unsigned __stdcall Receiver(void *arg) {
    ReceiverConfigStructure *receiverConfigStructure = (ReceiverConfigStructure *) arg;
    _RECEIVER_INTERRUPT_FUNCTION _ReceiverInterruptFunction = (_RECEIVER_INTERRUPT_FUNCTION) receiverConfigStructure->
            _ReceiverInterruptFunction;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) return 0;

    SOCKET receiverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiverSocket == INVALID_TRANSMITTER_SOCKET) {
        WSACleanup();
        return 0;
    }

    struct sockaddr_in receiverIPAddress = {0};
    receiverIPAddress.sin_family = AF_INET;
    receiverIPAddress.sin_port = htons(receiverConfigStructure->port);
    receiverIPAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(receiverSocket, (struct sockaddr *) &receiverIPAddress, sizeof(receiverIPAddress)) == SOCKET_ERROR) {
        closesocket(receiverSocket);
        WSACleanup();
        return 0;
    }

    while (1) {
        ReceivedDataStructure *receivedDataStructure = malloc(sizeof(ReceivedDataStructure));
        if (!receivedDataStructure) continue;

        receivedDataStructure->clientIPAddressLength = sizeof(receivedDataStructure->clientIPAddress);

        // Peek to get client address without removing the message
        receivedDataStructure->receivedDataLength = recvfrom(
            receiverSocket,
            receivedDataStructure->dataBuffer,
            sizeof(receivedDataStructure->dataBuffer),
            0, // -> Consider Zero if you want to remove packet from Socket
            (struct sockaddr *) &receivedDataStructure->clientIPAddress,
            &receivedDataStructure->clientIPAddressLength
        );

        if (receivedDataStructure->receivedDataLength <= 0) {
            free(receivedDataStructure);
            continue;
        }

        _beginthreadex(NULL, 0, _ReceiverInterruptFunction, receivedDataStructure, 0, NULL);

        // Wait a little to avoid re-spawning for same client
        Sleep(100);
    }

    // This line is technically unreachable
    closesocket(receiverSocket);
    WSACleanup();
    return 0;
}

void InitiateConstellation(ReceiverConfigStructure *receiverConfigStructure) {
    HANDLE receiverThread = (HANDLE) _beginthreadex(NULL, 0, Receiver, (void *) receiverConfigStructure, 0, NULL);
    if (receiverThread) CloseHandle(receiverThread);
}

TransmitterConfigStructure CreateTransmitter(const char *ipAddressPointer, int port) {
    static int wsaStarted = 0;
    if (!wsaStarted) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            fprintf(stderr, "[CreateTransmitter] WSAStartup failed\n");
        } else {
            wsaStarted = 1;
        }
    }

    TransmitterConfigStructure transmitterConfigStructure;
    transmitterConfigStructure.ipAddressPointer = ipAddressPointer;
    transmitterConfigStructure.port = port;
    transmitterConfigStructure.transmitterSocket = INVALID_TRANSMITTER_SOCKET;
    memset(&transmitterConfigStructure.destinationAddress, 0, sizeof(transmitterConfigStructure.destinationAddress));

    transmitterConfigStructure.transmitterSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (transmitterConfigStructure.transmitterSocket == INVALID_TRANSMITTER_SOCKET) {
        fprintf(stderr, "[CreateTransmitter] Socket creation failed\n");
        return transmitterConfigStructure;
    }

    transmitterConfigStructure.destinationAddress.sin_family = AF_INET;
    transmitterConfigStructure.destinationAddress.sin_port = htons(transmitterConfigStructure.port);
    if (inet_pton(AF_INET, transmitterConfigStructure.ipAddressPointer,
                  &transmitterConfigStructure.destinationAddress.sin_addr) != 1) {
        fprintf(stderr, "[CreateTransmitter] Invalid IP address: %s\n", transmitterConfigStructure.ipAddressPointer);
        closesocket(transmitterConfigStructure.transmitterSocket);
        transmitterConfigStructure.transmitterSocket = INVALID_TRANSMITTER_SOCKET;
    }
    return transmitterConfigStructure;
}

void Transmitter(TransmitterConfigStructure *transmitterConfigStructure, char *dataBufferPointer,
                 int dataBufferLength) {
    if (transmitterConfigStructure->transmitterSocket == INVALID_TRANSMITTER_SOCKET) {
        *transmitterConfigStructure = CreateTransmitter(transmitterConfigStructure->ipAddressPointer,
                                                        transmitterConfigStructure->port);
        if (transmitterConfigStructure->transmitterSocket == INVALID_TRANSMITTER_SOCKET) {
            fprintf(stderr, "[Transmitter] Failed to (re)create transmitter socket\n");
            return;
        }
    }

    int result = sendto(
        transmitterConfigStructure->transmitterSocket,
        dataBufferPointer,
        dataBufferLength,
        0,
        (struct sockaddr *) &transmitterConfigStructure->destinationAddress,
        sizeof(transmitterConfigStructure->destinationAddress)
    );

    if (result == SOCKET_ERROR) {
        fprintf(stderr, "[Transmitter] sendto() failed. Reinitializing socket...\n");
        *transmitterConfigStructure = CreateTransmitter(transmitterConfigStructure->ipAddressPointer,
                                                        transmitterConfigStructure->port);
    }
}

#elif __linux__
unsigned int Receiver(void *arg) {
    ReceiverConfigStructure *receiverConfigStructure = (ReceiverConfigStructure *) arg;
    _RECEIVER_INTERRUPT_FUNCTION _ReceiverInterruptFunction = (_RECEIVER_INTERRUPT_FUNCTION) receiverConfigStructure->
            _ReceiverInterruptFunction;

    int receiverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiverSocket < 0) return 0;

    struct sockaddr_in receiverIPAddress = {0};
    receiverIPAddress.sin_family = AF_INET;
    receiverIPAddress.sin_port = htons(receiverConfigStructure->port);
    receiverIPAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(receiverSocket, (struct sockaddr *) &receiverIPAddress, sizeof(receiverIPAddress)) < 0) {
        close(receiverSocket);
        return 0;
    }

    while (1) {
        ReceivedDataStructure *receivedDataStructure = malloc(sizeof(ReceivedDataStructure));
        if (!receivedDataStructure) continue;

        receivedDataStructure->clientIPAddressLength = sizeof(receivedDataStructure->clientIPAddress);

        // MSG_PEEK to simulate the same client-based spawning
        receivedDataStructure->receivedDataLength = recvfrom(
            receiverSocket,
            receivedDataStructure->dataBuffer,
            sizeof(receivedDataStructure->dataBuffer),
            MSG_PEEK,
            (struct sockaddr *) &receivedDataStructure->clientIPAddress,
            &receivedDataStructure->clientIPAddressLength
        );

        if (receivedDataStructure->receivedDataLength <= 0) {
            free(receivedDataStructure);
            continue;
        }

        pthread_t thread;
        pthread_create(&thread, NULL, _ReceiverInterruptFunction, receivedDataStructure);
        pthread_detach(thread);

        usleep(100000); // 100 ms
    }

    close(receiverSocket);
    return 0;
}

void InitiateConstellation(ReceiverConfigStructure *receiverConfigStructure) {
    pthread_t receiverThread;
    pthread_create(&receiverThread, NULL, (void *(*)(void *)) Receiver, receiverConfigStructure);
    pthread_detach(receiverThread);
}

TransmitterConfigStructure CreateTransmitter(const char *ipAddressPointer, int port) {
    TransmitterConfigStructure transmitterConfigStructure;
    transmitterConfigStructure.ipAddressPointer = ipAddressPointer;
    transmitterConfigStructure.port = port;
    transmitterConfigStructure.transmitterSocket = -1;
    memset(&transmitterConfigStructure.destinationAddress, 0, sizeof(transmitterConfigStructure.destinationAddress));

    transmitterConfigStructure.transmitterSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (transmitterConfigStructure.transmitterSocket < 0) {
        fprintf(stderr, "[CreateTransmitter] Socket creation failed\n");
        return transmitterConfigStructure;
    }

    transmitterConfigStructure.destinationAddress.sin_family = AF_INET;
    transmitterConfigStructure.destinationAddress.sin_port = htons(transmitterConfigStructure.port);
    if (inet_pton(AF_INET, transmitterConfigStructure.ipAddressPointer,
                  &transmitterConfigStructure.destinationAddress.sin_addr) != 1) {
        fprintf(stderr, "[CreateTransmitter] Invalid IP address: %s\n", transmitterConfigStructure.ipAddressPointer);
        close(transmitterConfigStructure.transmitterSocket);
        transmitterConfigStructure.transmitterSocket = -1;
    }

    return transmitterConfigStructure;
}

void Transmitter(TransmitterConfigStructure *transmitterConfigStructure, char *dataBufferPointer,
                 int dataBufferLength) {
    if (transmitterConfigStructure->transmitterSocket < 0) {
        *transmitterConfigStructure = CreateTransmitter(transmitterConfigStructure->ipAddressPointer,
                                                        transmitterConfigStructure->port);
        if (transmitterConfigStructure->transmitterSocket < 0) {
            fprintf(stderr, "[Transmitter] Failed to (re)create transmitter socket\n");
            return;
        }
    }

    int result = sendto(
        transmitterConfigStructure->transmitterSocket,
        dataBufferPointer,
        dataBufferLength,
        0,
        (struct sockaddr *) &transmitterConfigStructure->destinationAddress,
        sizeof(transmitterConfigStructure->destinationAddress)
    );

    if (result < 0) {
        fprintf(stderr, "[Transmitter] sendto() failed. Reinitializing socket...\n");
        *transmitterConfigStructure = CreateTransmitter(transmitterConfigStructure->ipAddressPointer,
                                                        transmitterConfigStructure->port);
    }
}
#endif
