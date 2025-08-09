#include "library.h"

#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
#include <process.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winsock2.h>

#define CLOSE_SOCKET closesocket
#define INVALID_TRANSMITTER_SOCKET INVALID_SOCKET

#elif __linux__

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define CLOSE_SOCKET close
#define INVALID_TRANSMITTER_SOCKET -1
#define SOCKET_ERROR -1

typedef pthread_t THREAD;

#endif

#ifdef _WIN32
#else
#endif

#define RECEIVER_PORT 47474
#define TRANSMITTER_PORT 47475
#define MAX_ACTIVE_RECEIVER_THREAD 100

#define MAX_MULTICAST_GROUPS 16
#define MAX_IP_LENGTH 16

typedef struct {
    HANDLE receiverThread;
    int activeReceivingThreads;
    volatile bool receiverThreadStatusActive;
    int multiCastArrayLength;
    char multiCastArray[MAX_MULTICAST_GROUPS][MAX_IP_LENGTH];
} ReceiverConfigStructure;

ReceiverConfigStructure receiverConfigStructure;

unsigned __stdcall Receiver(void *arg) {
    RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction = (RECEIVER_INTERRUPT_FUNCTION) arg;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) return 0;

    SOCKET receiverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiverSocket == INVALID_TRANSMITTER_SOCKET) {
        WSACleanup();
        return 0;
    }

    struct sockaddr_in receiverIPAddress = {0};
    receiverIPAddress.sin_family = AF_INET;
    receiverIPAddress.sin_port = htons(RECEIVER_PORT);
    receiverIPAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(receiverSocket, (struct sockaddr *) &receiverIPAddress, sizeof(receiverIPAddress)) == SOCKET_ERROR) {
        closesocket(receiverSocket);
        WSACleanup();
        return 0;
    }


    for (int i = 0; i < receiverConfigStructure.multiCastArrayLength; i++) {
        const char *multicastGroupIP = receiverConfigStructure.multiCastArray[i];
            struct ip_mreq multicastRequest;
            multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroupIP);
            multicastRequest.imr_interface.s_addr = INADDR_ANY; // Use default interface

            if (setsockopt(receiverSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                           (char *) &multicastRequest, sizeof(multicastRequest)) < 0) {
#ifdef _WIN32
                fprintf(stderr, "[Receiver] Failed to join multicast group %s\n", multicastGroupIP);
#else
                perror("[Receiver] Failed to join multicast group");
#endif
                closesocket(receiverSocket);
                WSACleanup();
                return 0;
            }
        }


    receiverConfigStructure.receiverThreadStatusActive = TRUE;

    while (receiverConfigStructure.receiverThreadStatusActive) {
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

        if (receiverConfigStructure.activeReceivingThreads > 0) {
#ifdef _WIN32
            _beginthreadex(NULL, 0, ReceiverInterruptFunction, receivedDataStructure, 0, nullptr);
#elif __linux__
            pthread_t thread_id;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_create(&thread_id, &attr, ReceiverInterruptFunction, receivedDataStructure);
            pthread_attr_destroy(&attr);
#endif
            receiverConfigStructure.activeReceivingThreads--;
        } else {
            free(receivedDataStructure);
        }

        // Wait a little to avoid re-spawning for same client
        Sleep(100);
    }

    // This line is technically unreachable
    closesocket(receiverSocket);
    WSACleanup();
    return 0;
}

void InitiateConstellation(RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction,
                           const char (*multiCastArray)[MAX_IP_LENGTH]) {
    receiverConfigStructure.activeReceivingThreads = MAX_ACTIVE_RECEIVER_THREAD;
    receiverConfigStructure.multiCastArrayLength = 0;

    if (multiCastArray == NULL) {
        receiverConfigStructure.multiCastArrayLength = 0;
    } else {
        // Auto-detect length based on sentinel empty string
        for (int i = 0; i < MAX_MULTICAST_GROUPS; i++) {
            if (multiCastArray[i][0] == '\0') {
                break;
            }
            strncpy(receiverConfigStructure.multiCastArray[i], multiCastArray[i], MAX_IP_LENGTH - 1);
            receiverConfigStructure.multiCastArray[i][MAX_IP_LENGTH - 1] = '\0';
            receiverConfigStructure.multiCastArrayLength++;
        }
    }

#ifdef _WIN32
    receiverConfigStructure.receiverThread = (HANDLE) _beginthreadex(
        nullptr, 0, Receiver, (void *) ReceiverInterruptFunction, 0, nullptr);
    if (receiverConfigStructure.receiverThread) CloseHandle(receiverConfigStructure.receiverThread);
#elif __linux__
    int pthreadStatus = pthread_create(&receiverConfigStructure.receiverThread, NULL, Receiver,
                                       (void *) ReceiverInterruptFunction);
    if (pthreadStatus != 0) {
        // pthread_create failed; handle error if you want
        receiverConfigStructure.receiverThread = 0; // or some invalid value to indicate failure
    }
#endif
    SleepForMs(500);
}

unsigned EXIT_RECEIVER_INTERRUPT(ReceivedDataStructure *receivedDataStructure) {
    if (receivedDataStructure) {
        free(receivedDataStructure);
    }
    receiverConfigStructure.activeReceivingThreads++;
#ifdef _WIN32
    _endthreadex(0);
#elif __linux__
    pthread_exit(NULL);
#endif
}

TransmitterConfigStructure CreateTransmitter(const char *ipAddressPointer) {
    static int wsaStarted = 0;
    if (!wsaStarted) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            fprintf(stderr, "[CreateTransmitter] WSAStartup failed\n");
        } else {
            wsaStarted = 1;
        }
    }

    TransmitterConfigStructure transmitterConfigStructure = {nullptr};  // Zero initialize all fields

    transmitterConfigStructure.ipAddressPointer = ipAddressPointer;
    transmitterConfigStructure.port = TRANSMITTER_PORT;
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

#ifdef _WIN32
        closesocket(transmitterConfigStructure.transmitterSocket);
#elif __linux__
        close(transmitterConfigStructure.transmitterSocket);
#endif

        transmitterConfigStructure.transmitterSocket = INVALID_TRANSMITTER_SOCKET;
    }

    // Optional: Check if the IP address is multicast (224.0.0.0 to 239.255.255.255)
    if (transmitterConfigStructure.transmitterSocket != INVALID_TRANSMITTER_SOCKET) {
        uint32_t ip_netorder = transmitterConfigStructure.destinationAddress.sin_addr.s_addr;
        uint8_t first_octet = (uint8_t) ((ntohl(ip_netorder) >> 24) & 0xFF);

        if (first_octet >= 224 && first_octet <= 239) {
            int multicastTTL = 1; // Limit multicast packets to local subnet

#ifdef _WIN32
            if (setsockopt(transmitterConfigStructure.transmitterSocket, IPPROTO_IP, IP_MULTICAST_TTL,
                           (const char *) &multicastTTL, sizeof(multicastTTL)) < 0) {
                fprintf(stderr, "[CreateTransmitter] Failed to set multicast TTL\n");
            }
#else
            if (setsockopt(transmitterConfigStructure.transmitterSocket, IPPROTO_IP, IP_MULTICAST_TTL,
                           &multicastTTL, sizeof(multicastTTL)) < 0) {
                perror("[CreateTransmitter] Failed to set multicast TTL");
            }
#endif
        }
        transmitterConfigStructure.multiCastSocket = TRUE;
    }
    else {
        transmitterConfigStructure.multiCastSocket = FALSE;
    }

    return transmitterConfigStructure;
}


Status Transmitter(TransmitterConfigStructure *transmitterConfigStructure, const char *dataBufferPointer,
                   const int dataBufferLength) {
    Status status;
    if (transmitterConfigStructure->transmitterSocket == INVALID_TRANSMITTER_SOCKET) {
        status = INVALID_SOCKET_ERROR;
    }
    else {
        int result = sendto(
            transmitterConfigStructure->transmitterSocket,
            dataBufferPointer,
            dataBufferLength,
            0,
            (struct sockaddr *) &transmitterConfigStructure->destinationAddress,
            sizeof(transmitterConfigStructure->destinationAddress)
        );

        if (result == SOCKET_ERROR) {
            status = SENDTO_ERROR;
        } else {
            status = SUCCESS;
        }
    }

    return status;
}

void DestroyTransmitter(TransmitterConfigStructure *transmitterConfigStructure) {
    if (transmitterConfigStructure->transmitterSocket != INVALID_TRANSMITTER_SOCKET) {
        CLOSE_SOCKET(transmitterConfigStructure->transmitterSocket);
        transmitterConfigStructure->transmitterSocket = INVALID_TRANSMITTER_SOCKET;
    }
}


void DeInitiateConstellation() {
    // Example: Set a global flag to request thread exit (you need to implement it)
    receiverConfigStructure.receiverThreadStatusActive = FALSE;
    if (receiverConfigStructure.receiverThread) {
#ifdef _WIN32
        WaitForSingleObject(receiverConfigStructure.receiverThread, INFINITE);
        CloseHandle(receiverConfigStructure.receiverThread);
#elif __linux__
        pthread_join(receiverConfigStructure.receiverThread, NULL);
#endif
        receiverConfigStructure.receiverThread = NULL;
    }
}




