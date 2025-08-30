#include "library.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

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
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define CLOSE_SOCKET close
#define INVALID_TRANSMITTER_SOCKET (-1)
#define SOCKET_ERROR (-1)

#define True true
#define False false

typedef pthread_t THREAD;

#endif

#ifdef _WIN32
#else
#endif

// #define RECEIVER_PORT 47474
// #define TRANSMITTER_PORT 47474
#define MAX_CLIENT_ALLOWED 100
#define MAX_ACTIVE_RECEIVER_THREAD 100

typedef struct {
    socket_t udpSocket;

    struct Receiver {
        struct {
            struct sockaddr_in socketIPAddress;
            int port;
        } config;

        RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction;

        struct {
            THREAD thread;
            int totalActiveThreads;
            volatile bool listeningStatusActive;
        } threading;
    } receiver;

    struct Transmitter {
        struct {
            const char *ipAddressPointer; /**< Destination IP address as a string */
            int port; /**< Destination port number */
            struct sockaddr_in socketIPAddress; /**< Cached destination address struct */
        } config;

        State state;
    } transmitter[MAX_CLIENT_ALLOWED];
} ConstellationStructure;

ConstellationStructure constellationStructure = {0};

#ifdef _WIN32
unsigned __stdcall Receiver(void *arg) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) return 0;
#else
void *Receiver(void *arg) {
#endif
    receivingThreadStatus = SUCCESS;
    constellationStructure.receiver.config.socketIPAddress.sin_family = AF_INET;
    constellationStructure.receiver.config.socketIPAddress.sin_port = htons(
        constellationStructure.receiver.config.port);
    constellationStructure.receiver.config.socketIPAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(constellationStructure.udpSocket,
             (struct sockaddr *) &constellationStructure.receiver.config.socketIPAddress,
             sizeof(constellationStructure.receiver.config.socketIPAddress)) == SOCKET_ERROR) {
        receivingThreadStatus = FAILURE_SOCKET_BIND;
    } else {
        constellationStructure.receiver.threading.listeningStatusActive = true;

        while (constellationStructure.receiver.threading.listeningStatusActive) {
            ReceivedDataStructure *receivedDataStructure = malloc(sizeof(ReceivedDataStructure));
            if (!receivedDataStructure) continue;

            receivedDataStructure->clientIPAddressLength = sizeof(receivedDataStructure->clientIPAddress);

            // Peek to get client address without removing the message
            receivedDataStructure->receivedDataLength = recvfrom(
                constellationStructure.udpSocket,
                receivedDataStructure->dataBuffer,
                sizeof(receivedDataStructure->dataBuffer),
                0, // -> Consider Zero if you want to remove packet from Socket
                (struct sockaddr *) &receivedDataStructure->clientIPAddress,
                (socklen_t *) &receivedDataStructure->clientIPAddressLength
            );

            if (receivedDataStructure->receivedDataLength <= 0) {
                free(receivedDataStructure);
                continue;
            }

            if (constellationStructure.receiver.threading.totalActiveThreads > 0) {
#ifdef _WIN32
                _beginthreadex(NULL, 0, ReceiverInterruptFunction, receivedDataStructure, 0, NULL);
#else
                pthread_t thread_id;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                pthread_create(&thread_id, &attr, constellationStructure.receiver.ReceiverInterruptFunction,
                               receivedDataStructure);
                pthread_attr_destroy(&attr);
#endif
                constellationStructure.receiver.threading.totalActiveThreads--;
            } else {
                free(receivedDataStructure);
            }

            // Wait a little to avoid re-spawning for same client
            SleepForMs(100);
        }
    }

    if (constellationStructure.udpSocket != INVALID_TRANSMITTER_SOCKET) {
        CLOSE_SOCKET(constellationStructure.udpSocket);
        constellationStructure.udpSocket = INVALID_TRANSMITTER_SOCKET;
    }
#ifdef _WIN32
    WSACleanup();
#endif
    return NULL;
}

Status InitiateConstellation(RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction, int port) {
    Status status = SUCCESS;
    // Set all Transmitters INACTIVE
    for (int index = 0; index < MAX_CLIENT_ALLOWED; index++) {
        constellationStructure.transmitter[index].state = INACTIVE;
    }
    // Create UDP Socket
    constellationStructure.udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (constellationStructure.udpSocket == INVALID_TRANSMITTER_SOCKET) {
        status = FAILURE_SOCKET_INVALID;
    } else {
        constellationStructure.receiver.threading.totalActiveThreads = MAX_ACTIVE_RECEIVER_THREAD;
        constellationStructure.receiver.ReceiverInterruptFunction = ReceiverInterruptFunction;
        constellationStructure.receiver.config.port = port;

        // Create Receiver Thread
#ifdef _WIN32
        constellationStructure.receiver.config.receiverThread = (HANDLE) _beginthreadex(
            NULL, 0, Receiver, (void *) ReceiverInterruptFunction, 0, NULL);
        if (constellationStructure.receiver.config.receiverThread)
            CloseHandle(
                constellationStructure.receiver.config.receiverThread);
#else
        int pthreadStatus = pthread_create(&constellationStructure.receiver.threading.thread, NULL, Receiver,
                                           NULL);
        if (pthreadStatus != 0) {
            constellationStructure.receiver.threading.thread = NULL;
            switch (pthreadStatus) {
                case EAGAIN: status = FAILURE_THREAD_CREATE_EAGAIN;
                    break;
                case EINVAL: status = FAILURE_THREAD_CREATE_EINVAL;
                    break;
                case EPERM: status = FAILURE_THREAD_CREATE_EPERM;
                    break;
                default: status = FAILURE_THREAD_CREATE;
            }
        }
#endif
        SleepForMs(500);
    }
    return status;
}

unsigned EXIT_RECEIVER_INTERRUPT(ReceivedDataStructure *receivedDataStructure) {
    if (receivedDataStructure) {
        free(receivedDataStructure);
    }
    constellationStructure.receiver.threading.totalActiveThreads++;
#ifdef _WIN32
    _endthreadex(0);
#else
    pthread_exit(NULL);
#endif
}

TransmitterID CreateTransmitter(const char *ipAddressPointer, int port) {
    static int wsaStarted = 0;
    int transmitterID = -1;

#ifdef _WIN32
    if (!wsaStarted) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            fprintf(stderr, "[CreateTransmitter] WSAStartup failed\n");
        } else {
            wsaStarted = 1;
        }
    }
#endif

    for (int index = 0; index < MAX_CLIENT_ALLOWED; index++) {
        if (constellationStructure.transmitter[index].state == INACTIVE) {
            transmitterID = index;
            break;
        } else if (0 == strcmp(ipAddressPointer, (const char *) &constellationStructure.transmitter[index].config.ipAddressPointer) &&
                   port ==
                   constellationStructure.transmitter[index].config.port) {
            // already present error
            break;
        }
    }
    if (transmitterID == -1) {
        return (-1);
    }

    constellationStructure.transmitter[transmitterID].config.port = port;
    constellationStructure.transmitter[transmitterID].config.ipAddressPointer = ipAddressPointer;
    memset(&constellationStructure.transmitter[transmitterID].config.socketIPAddress, 0,
           sizeof(constellationStructure.transmitter[transmitterID].config.socketIPAddress));

    constellationStructure.transmitter[transmitterID].config.socketIPAddress.sin_family = AF_INET;
    constellationStructure.transmitter[transmitterID].config.socketIPAddress.sin_port = htons(
        constellationStructure.transmitter[transmitterID].config.port);
    return transmitterID;
}


Status Transmitter(int transmitterID, const char *dataBufferPointer,
                   const int dataBufferLength) {
    Status status = SUCCESS;
    if (constellationStructure.udpSocket == INVALID_TRANSMITTER_SOCKET) {
        status = FAILURE_SOCKET_INVALID;
    }

    int result = sendto(
        constellationStructure.udpSocket,
        dataBufferPointer,
        dataBufferLength,
        0,
        (struct sockaddr *) &constellationStructure.transmitter[transmitterID].config.socketIPAddress,
        sizeof(constellationStructure.transmitter[transmitterID].config.socketIPAddress)
    );

    if (result == SOCKET_ERROR) {
        status = FAILURE_SOCKET_SENDTO;
    }
    return status;
}

void DestroyTransmitter(int transmitterID) {
    // if (constellationStructure.transmitter[transmitterID].config.transmitterSocket != INVALID_TRANSMITTER_SOCKET) {
    //     CLOSE_SOCKET(constellationStructure.transmitter[transmitterID].config.transmitterSocket);
    //     constellationStructure.transmitter[transmitterID].config.transmitterSocket = INVALID_TRANSMITTER_SOCKET;
    // }
}


void DeInitiateConstellation() {
    // Example: Set a global flag to request thread exit (you need to implement it)
    constellationStructure.receiver.threading.listeningStatusActive = False;
    if (constellationStructure.receiver.threading.thread) {
#ifdef _WIN32
        WaitForSingleObject(constellationStructure.receiver.config.receiverThread, INFINITE);
        CloseHandle(constellationStructure.receiver.config.receiverThread);
#else
        pthread_join(constellationStructure.receiver.threading.thread, NULL);
#endif
        constellationStructure.receiver.threading.thread = NULL;
    }
}
