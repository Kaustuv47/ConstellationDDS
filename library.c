#include "library.h"

#include <stdio.h>
#include <stdbool.h>


#ifdef _WIN32
#include <process.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winsock2.h>

#define CLOSE_SOCKET closesocket
#define INVALID_TRANSMITTER_SOCKET INVALID_SOCKET

#define True TRUE
#define False FALSE

#else

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

#define True true
#define False false

typedef pthread_t THREAD;

#endif

#ifdef _WIN32
#else
#endif

// #define RECEIVER_PORT 47474
#define TRANSMITTER_PORT 47474
#define MAX_ACTIVE_RECEIVER_THREAD 100

typedef struct {
    THREAD receiverThread;
    int activeReceivingThreads;
    volatile bool receiverThreadStatusActive;
    RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction;
    int port;
} ReceiverConfigStructure;

ReceiverConfigStructure receiverConfigStructure;


#ifdef _WIN32
unsigned __stdcall Receiver(void *arg) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) return 0;
#else
void *Receiver(void *arg) {
#endif
    Status status;

    socket_t receiverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiverSocket == INVALID_TRANSMITTER_SOCKET) {
        status = FAILURE;
    } else {
        struct sockaddr_in receiverIPAddress = {0};
        receiverIPAddress.sin_family = AF_INET;
        receiverIPAddress.sin_port = htons(receiverConfigStructure.port);
        receiverIPAddress.sin_addr.s_addr = INADDR_ANY;

        if (bind(receiverSocket, (struct sockaddr *) &receiverIPAddress, sizeof(receiverIPAddress)) == SOCKET_ERROR) {
            status = FAILURE;
        } else {
            receiverConfigStructure.receiverThreadStatusActive = true;

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
#else
                    pthread_t thread_id;
                    pthread_attr_t attr;
                    pthread_attr_init(&attr);
                    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                    pthread_create(&thread_id, &attr, receiverConfigStructure.ReceiverInterruptFunction, receivedDataStructure);
                    pthread_attr_destroy(&attr);
#endif
                    receiverConfigStructure.activeReceivingThreads--;
                } else {
                    free(receivedDataStructure);
                }

                // Wait a little to avoid re-spawning for same client
                SleepForMs(100);
            }
        }
    }

    if (receiverSocket != INVALID_TRANSMITTER_SOCKET) {
        CLOSE_SOCKET(receiverSocket);
        receiverSocket = INVALID_TRANSMITTER_SOCKET;
    }
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

void InitiateConstellation(RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction, int port) {
    receiverConfigStructure.activeReceivingThreads = MAX_ACTIVE_RECEIVER_THREAD;
    receiverConfigStructure.ReceiverInterruptFunction = ReceiverInterruptFunction;
    receiverConfigStructure.port = port;
#ifdef _WIN32
    receiverConfigStructure.receiverThread = (HANDLE) _beginthreadex(
        nullptr, 0, Receiver, (void *) ReceiverInterruptFunction, 0, nullptr);
    if (receiverConfigStructure.receiverThread) CloseHandle(receiverConfigStructure.receiverThread);
#else
    int pthreadStatus = pthread_create(&receiverConfigStructure.receiverThread, NULL, Receiver,
                                       NULL);
    if (pthreadStatus != 0) {
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
#else
    pthread_exit(NULL);
#endif
}

TransmitterConfigStructure CreateTransmitter(const char *ipAddressPointer, int port) {
    static int wsaStarted = 0;
#ifdef _WIN32
    if (!wsaStarted) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            fprintf(stderr, "[CreateTransmitter] WSAStartup failed\n");
        } else {
            wsaStarted = 1;
        }
    }
#else

#endif
    TransmitterConfigStructure transmitterConfigStructure;
    transmitterConfigStructure.port = port;
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
        CLOSE_SOCKET(transmitterConfigStructure.transmitterSocket);
        transmitterConfigStructure.transmitterSocket = INVALID_TRANSMITTER_SOCKET;
    }
    return transmitterConfigStructure;
}


Status Transmitter(TransmitterConfigStructure *transmitterConfigStructure, const char *dataBufferPointer,
                   const int dataBufferLength) {
    Status status;
    if (transmitterConfigStructure->transmitterSocket == INVALID_TRANSMITTER_SOCKET) {
        status = INVALID_SOCKET_ERROR;
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
        status = SENDTO_ERROR;
    } else {
        status = SUCCESS;
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
    receiverConfigStructure.receiverThreadStatusActive = False;
    if (receiverConfigStructure.receiverThread) {
#ifdef _WIN32
        WaitForSingleObject(receiverConfigStructure.receiverThread, INFINITE);
        CloseHandle(receiverConfigStructure.receiverThread);
#else
        pthread_join(receiverConfigStructure.receiverThread, NULL);
#endif
        receiverConfigStructure.receiverThread = NULL;
    }
}
