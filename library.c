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

#define MAX_CLIENT_ALLOWED 100
#define MAX_ACTIVE_RECEIVER_THREAD 100

typedef int socket_t;

typedef struct {
    socket_t udpSocket;

    struct sockaddr_in subscriberSocketAddress;
    struct sockaddr_in publisherSocketAddress;

    Status status;

    struct timespec delay;
} PubSubStructure;

static PubSubStructure pubSubStructure = {0};

Status *Subscribe(void *pubSubInstancePointer, const char *targetIPAddressString, const int pubSubPort,
                  const unsigned int *dataBufferPointer, const unsigned int dataBufferLength, const long updateDelay) {
    pubSubStructure.status = SUCCESS;
    pubSubStructure.delay.tv_nsec = updateDelay;
    pubSubStructure.delay.tv_sec = 0;

    memset(&pubSubStructure.subscriberSocketAddress, 0, sizeof(struct sockaddr_in));

    pubSubStructure.subscriberSocketAddress.sin_family = AF_INET;
    pubSubStructure.subscriberSocketAddress.sin_port = htons(pubSubPort);
    pubSubStructure.subscriberSocketAddress.sin_addr.s_addr = inet_addr(targetIPAddressString);

    pubSubStructure.udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (pubSubStructure.udpSocket == INVALID_TRANSMITTER_SOCKET) {
        pubSubStructure.status = FAILURE_SOCKET_INVALID;
    } else {
        if (bind(pubSubStructure.udpSocket,
                 (struct sockaddr *) &pubSubStructure.subscriberSocketAddress,
                 sizeof(pubSubStructure.subscriberSocketAddress)) == SOCKET_ERROR) {
            pubSubStructure.status = FAILURE_SOCKET_BIND;
        } else {
            unsigned int tempDataBuffer[65535];

            struct sockaddr_in clientIPAddress;
            socklen_t clientIPAddressLength = sizeof(clientIPAddress);

            while (pubSubStructure.status) {
                // Peek to get client address without removing the message
                unsigned long int tempDataBufferLength = recvfrom(
                    pubSubStructure.udpSocket,
                    tempDataBuffer,
                    sizeof(tempDataBuffer),
                    0, // -> Consider Zero if you want to remove packet from Socket
                    (struct sockaddr *) &clientIPAddress,
                    (socklen_t *) &clientIPAddressLength
                );

                if (tempDataBufferLength <= 0) {
                    continue;
                }
            }
        }
    }

    if (pubSubStructure.udpSocket != INVALID_TRANSMITTER_SOCKET) {
        CLOSE_SOCKET(pubSubStructure.udpSocket);
        pubSubStructure.udpSocket = INVALID_TRANSMITTER_SOCKET;
    }
    return &pubSubStructure.status;
}

Status *Publish(void *pubSubInstancePointer, const char *targetIPAddressString, const int pubSubPort,
                const unsigned int *dataBufferPointer, const unsigned int dataBufferLength, const long updateDelay) {
    pubSubStructure.status = SUCCESS;
    pubSubStructure.delay.tv_nsec = updateDelay;
    pubSubStructure.delay.tv_sec = 0;

    memset(&pubSubStructure.publisherSocketAddress, 0, sizeof(struct sockaddr_in));

    pubSubStructure.publisherSocketAddress.sin_family = AF_INET;
    pubSubStructure.publisherSocketAddress.sin_port = htons(pubSubPort);
    pubSubStructure.publisherSocketAddress.sin_addr.s_addr = inet_addr(targetIPAddressString);
    pubSubStructure.udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (pubSubStructure.udpSocket == INVALID_TRANSMITTER_SOCKET) {
        pubSubStructure.status = FAILURE_SOCKET_INVALID;
    } else {
        pubSubInstancePointer = (PubSubStructure *) &pubSubStructure;
        while (SUCCESS == pubSubStructure.status) {
            long result = sendto(
                pubSubStructure.udpSocket,
                dataBufferPointer,
                dataBufferLength,
                0,
                (struct sockaddr *) &pubSubStructure.publisherSocketAddress,
                sizeof(pubSubStructure.publisherSocketAddress)
            );


            if (result == SOCKET_ERROR) {
                pubSubStructure.status = FAILURE_SOCKET_SENDTO;
            }
        }
    }
    if (pubSubStructure.udpSocket != INVALID_TRANSMITTER_SOCKET) {
        CLOSE_SOCKET(pubSubStructure.udpSocket);
        pubSubStructure.udpSocket = INVALID_TRANSMITTER_SOCKET;
    }
    return &pubSubStructure.status;
}

void UnPubSub(void *pubSubInstancePointer) {
    PubSubStructure *pubSubStructurePointer = pubSubInstancePointer;
    pubSubStructurePointer->status = KILL;
}
