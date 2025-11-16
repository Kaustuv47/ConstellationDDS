#include "library.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>


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

// #define RECEIVER_PORT 47474
// #define TRANSMITTER_PORT 47474
#define MAX_CLIENT_ALLOWED 100
#define MAX_ACTIVE_RECEIVER_THREAD 100

typedef struct {
    socket_t udpSocket;

    struct sockaddr_in receiverSocketAddress;
    struct sockaddr_in transmitterSocketAddress[MAX_CLIENT_ALLOWED];

    RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction;

    struct Receiver {
        struct {
            THREAD thread;
            int totalActiveThreads;
            volatile bool listeningStatusActive;
        } threading;
    } receiver;
} ConstellationStructure;

static ConstellationStructure constellationStructure = {0};

void *Receiver(void *arg) {
    receivingThreadStatus = SUCCESS;

    if (bind(constellationStructure.udpSocket,
             (struct sockaddr *) &constellationStructure.receiverSocketAddress,
             sizeof(constellationStructure.receiverSocketAddress)) == SOCKET_ERROR) {
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

                pthread_t thread_id;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                pthread_create(&thread_id, &attr, constellationStructure.ReceiverInterruptFunction,
                               receivedDataStructure);
                pthread_attr_destroy(&attr);
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
    return NULL;
}

Status InitiateConstellation(RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction, int receiverPort) {
    Status status = SUCCESS;

    for (int transmitterIdentifier=0; transmitterIdentifier<MAX_CLIENT_ALLOWED; transmitterIdentifier++) {
        memset(&constellationStructure.transmitterSocketAddress[transmitterIdentifier], 0, sizeof(struct sockaddr_in));
    }

    constellationStructure.receiverSocketAddress.sin_family = AF_INET;
    constellationStructure.receiverSocketAddress.sin_port = htons(
        receiverPort);
    constellationStructure.receiverSocketAddress.sin_addr.s_addr = INADDR_ANY;

    // Create UDP Socket
    constellationStructure.udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (constellationStructure.udpSocket == INVALID_TRANSMITTER_SOCKET) {
        status = FAILURE_SOCKET_INVALID;
    } else {
        constellationStructure.receiver.threading.totalActiveThreads = MAX_ACTIVE_RECEIVER_THREAD;
        constellationStructure.ReceiverInterruptFunction = ReceiverInterruptFunction;

        // Create Receiver Thread
        int pthreadStatus = pthread_create(&constellationStructure.receiver.threading.thread, NULL, Receiver, NULL);
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
        SleepForMs(500);
    }
    return status;
}

unsigned EXIT_RECEIVER_INTERRUPT(ReceivedDataStructure *receivedDataStructure) {
    if (receivedDataStructure) {
        free(receivedDataStructure);
    }
    constellationStructure.receiver.threading.totalActiveThreads++;
    pthread_exit(NULL);
}

TransmitterID CreateTransmitter(const char *ipAddressPointer, int port)
{
    TransmitterID transmitterID = -1;
    uint32_t newIP;
    inet_pton(AF_INET, ipAddressPointer, &newIP);

    // Step 1: Check for duplicates
    for (int transmitterIdentifier = 0; transmitterIdentifier < MAX_CLIENT_ALLOWED; transmitterIdentifier++) {
        if (constellationStructure.transmitterSocketAddress[transmitterIdentifier].sin_family == AF_INET) {
            if (constellationStructure.transmitterSocketAddress[transmitterIdentifier].sin_addr.s_addr == newIP) {
                // Already exists
                transmitterID = transmitterIdentifier;
            }
        }
    }

    // Step 2: Find an empty slot
    for (int transmitterIdentifier = 0; transmitterIdentifier < MAX_CLIENT_ALLOWED; transmitterIdentifier++) {
        if (constellationStructure.transmitterSocketAddress[transmitterIdentifier].sin_family == 0) {
            // Use this slot
            memset(&constellationStructure.transmitterSocketAddress[transmitterIdentifier], 0,
                   sizeof(struct sockaddr_in));

            constellationStructure.transmitterSocketAddress[transmitterIdentifier].sin_family = AF_INET;
            constellationStructure.transmitterSocketAddress[transmitterIdentifier].sin_port = htons(port);
            constellationStructure.transmitterSocketAddress[transmitterIdentifier].sin_addr.s_addr = newIP;

            transmitterID = transmitterIdentifier;  // SUCCESS
        }
    }
    return transmitterID;
}



Status Transmitter(int transmitterID, const char *dataBufferPointer,
                   const int dataBufferLength) {
    Status status = SUCCESS;
    if (constellationStructure.udpSocket == INVALID_TRANSMITTER_SOCKET) {
        status = FAILURE_SOCKET_INVALID;
    }

    long result = sendto(
        constellationStructure.udpSocket,
        dataBufferPointer,
        dataBufferLength,
        0,
        (struct sockaddr *) &constellationStructure.transmitterSocketAddress[transmitterID],
        sizeof(constellationStructure.transmitterSocketAddress[transmitterID])
    );

    if (result == SOCKET_ERROR) {
        status = FAILURE_SOCKET_SENDTO;
    }
    return status;
}

void DestroyTransmitter(int transmitterID) {
    if (transmitterID >= 0 && transmitterID < MAX_CLIENT_ALLOWED) {
        memset(&constellationStructure.transmitterSocketAddress[transmitterID], 0, sizeof(struct sockaddr_in));
    }
}


void DeInitiateConstellation() {
    // Example: Set a global flag to request thread exit (you need to implement it)
    constellationStructure.receiver.threading.listeningStatusActive = False;
    if (constellationStructure.receiver.threading.thread) {
        pthread_join(constellationStructure.receiver.threading.thread, NULL);
        constellationStructure.receiver.threading.thread = NULL;
    }
}
