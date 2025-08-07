#include "library.h"

#include <stdio.h>
#include <process.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

// #define PORT 47474

unsigned __stdcall Receiver(void *arg) {
    ReceiverConfigStructure *receiverConfigStructure = (ReceiverConfigStructure *) arg;
    _RECEIVER_INTERRUPT_FUNCTION _ReceiverInterruptFunction = (_RECEIVER_INTERRUPT_FUNCTION) receiverConfigStructure->_ReceiverInterruptFunction;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) return 0;

    SOCKET receiverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiverSocket == INVALID_SOCKET) {
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
            MSG_PEEK,
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
    transmitterConfigStructure.transmitterSocket = INVALID_SOCKET;
    memset(&transmitterConfigStructure.destinationAddress, 0, sizeof(transmitterConfigStructure.destinationAddress));

    transmitterConfigStructure.transmitterSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (transmitterConfigStructure.transmitterSocket == INVALID_SOCKET) {
        fprintf(stderr, "[CreateTransmitter] Socket creation failed\n");
        return transmitterConfigStructure;
    }

    transmitterConfigStructure.destinationAddress.sin_family = AF_INET;
    transmitterConfigStructure.destinationAddress.sin_port = htons(transmitterConfigStructure.port);
    if (inet_pton(AF_INET, transmitterConfigStructure.ipAddressPointer,
                  &transmitterConfigStructure.destinationAddress.sin_addr) != 1) {
        fprintf(stderr, "[CreateTransmitter] Invalid IP address: %s\n", transmitterConfigStructure.ipAddressPointer);
        closesocket(transmitterConfigStructure.transmitterSocket);
        transmitterConfigStructure.transmitterSocket = INVALID_SOCKET;
    }
    return transmitterConfigStructure;
}

void Transmitter(TransmitterConfigStructure *transmitterConfigStructure, char *dataBufferPointer, int dataBufferLength) {
    if (transmitterConfigStructure->transmitterSocket == INVALID_SOCKET) {
        *transmitterConfigStructure = CreateTransmitter(transmitterConfigStructure->ipAddressPointer,
                                                        transmitterConfigStructure->port);
        if (transmitterConfigStructure->transmitterSocket == INVALID_SOCKET) {
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
