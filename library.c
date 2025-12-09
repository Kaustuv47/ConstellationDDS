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
#include <time.h>

#define CLOSE_SOCKET close
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

#define True true
#define False false

typedef int socket_t;

typedef struct {
    socket_t udpSocket;
    struct sockaddr_in socketAddress;
    pthread_t threadId;
    Status status;
    struct timespec delay;

    struct Buffer {
        unsigned char *pointer;
        unsigned int *length;
    } buffer;
} PubSubStructure;

static PubSubStructure publisherStructure[MAX_ALLOWED_PUBLISHER] = {0};
static PubSubStructure subscriberStructure[MAX_ALLOWED_SUBSCRIBER] = {0};

void *Subscribe(void *arg) {
    PubSubId subscriberId = (PubSubId) (intptr_t) arg;
    if (subscriberId < 0 || subscriberId >= MAX_ALLOWED_SUBSCRIBER) {
        return NULL;
    }
    if (bind(subscriberStructure[subscriberId].udpSocket,
             (struct sockaddr *) &subscriberStructure[subscriberId].socketAddress,
             sizeof(subscriberStructure[subscriberId].socketAddress)) == SOCKET_ERROR) {
        subscriberStructure[subscriberId].status = FAILURE_SOCKET_BIND;
    } else {
        unsigned char tempDataBuffer[65535];

        struct sockaddr_in clientIPAddress;
        socklen_t clientIPAddressLength = sizeof(clientIPAddress);

        while (subscriberStructure[subscriberId].status == SUCCESS) {
            ssize_t tempDataBufferLength = recvfrom(
                subscriberStructure[subscriberId].udpSocket,
                tempDataBuffer,
                sizeof(tempDataBuffer),
                0,
                (struct sockaddr *) &clientIPAddress,
                &clientIPAddressLength
            );

            if (tempDataBufferLength < 0) {
                // Error or socket closed, exit loop
                break;
            }

            if (tempDataBufferLength > 0) {
                if (subscriberStructure[subscriberId].buffer.pointer != NULL &&
                    subscriberStructure[subscriberId].buffer.length != NULL) {
                    unsigned int copyLength = tempDataBufferLength < *subscriberStructure[subscriberId].buffer.length
                                                  ? tempDataBufferLength
                                                  : *subscriberStructure[subscriberId].buffer.length;
                    memcpy(subscriberStructure[subscriberId].buffer.pointer, tempDataBuffer, copyLength);
                    *subscriberStructure[subscriberId].buffer.length = copyLength;
                }
            }
        }
    }

    if (subscriberStructure[subscriberId].udpSocket != INVALID_SOCKET) {
        CLOSE_SOCKET(subscriberStructure[subscriberId].udpSocket);
        subscriberStructure[subscriberId].udpSocket = INVALID_SOCKET;
    }
    return &subscriberStructure[subscriberId].status;
}

PubSubId InitSubscriber(Status *status, const char *targetIPAddressString, const int pubSubPort,
                        const unsigned char *dataBufferPointer, const unsigned int *dataBufferLength,
                        const long updateDelay) {
    PubSubId subscriberId = 0;
    for (int i = 1; i < MAX_ALLOWED_SUBSCRIBER; i++) {
        if (subscriberStructure[i].udpSocket == INVALID_SOCKET) {
            subscriberId = i;
            break;
        }
    }

    if (0 != subscriberId) {
        status = &subscriberStructure[subscriberId].status;
        subscriberStructure[subscriberId].status = SUCCESS;
        subscriberStructure[subscriberId].buffer.pointer = (unsigned char *) dataBufferPointer;
        subscriberStructure[subscriberId].buffer.length = (unsigned int *) dataBufferLength;
        subscriberStructure[subscriberId].delay.tv_nsec = updateDelay;
        subscriberStructure[subscriberId].delay.tv_sec = 0;

        memset(&subscriberStructure[subscriberId].socketAddress, 0, sizeof(struct sockaddr_in));

        subscriberStructure[subscriberId].socketAddress.sin_family = AF_INET;
        subscriberStructure[subscriberId].socketAddress.sin_port = htons(pubSubPort);
        subscriberStructure[subscriberId].socketAddress.sin_addr.s_addr = inet_addr(targetIPAddressString);

        subscriberStructure[subscriberId].udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (subscriberStructure[subscriberId].udpSocket == INVALID_SOCKET) {
            *status = FAILURE_SOCKET_INVALID;
        } else {
            const int pthreadStatus =
                    pthread_create(&subscriberStructure[subscriberId].threadId, NULL, Subscribe,
                                   (void *) (intptr_t) subscriberId);
            if (pthreadStatus != 0) {
                *status = FAILURE_THREAD_CREATE;
                CLOSE_SOCKET(subscriberStructure[subscriberId].udpSocket);
                subscriberStructure[subscriberId].udpSocket = INVALID_SOCKET;
            } else { *status = SUCCESS; }
        }
    } else {
        *status = FAILURE;
    }
    return subscriberId;
}

void *Publish(void *arg) {
    PubSubId publisherId = (PubSubId) (intptr_t) arg;
    if (publisherId < 0 || publisherId >= MAX_ALLOWED_PUBLISHER) {
        return NULL;
    }
    publisherStructure[publisherId].status = SUCCESS;
    while (SUCCESS == publisherStructure[publisherId].status) {
        if (publisherStructure[publisherId].buffer.pointer != NULL &&
            publisherStructure[publisherId].buffer.length != NULL &&
            *publisherStructure[publisherId].buffer.length > 0) {
            long result = sendto(
                publisherStructure[publisherId].udpSocket,
                publisherStructure[publisherId].buffer.pointer,
                *publisherStructure[publisherId].buffer.length,
                0,
                (struct sockaddr *) &publisherStructure[publisherId].socketAddress,
                sizeof(publisherStructure[publisherId].socketAddress)
            );

            if (result == SOCKET_ERROR) {
                publisherStructure[publisherId].status = FAILURE_SOCKET_SENDTO;
                break;
            }
        }
        nanosleep(&publisherStructure[publisherId].delay, NULL);
    }
    if (publisherStructure[publisherId].udpSocket != INVALID_SOCKET) {
        CLOSE_SOCKET(publisherStructure[publisherId].udpSocket);
        publisherStructure[publisherId].udpSocket = INVALID_SOCKET;
    }
    return &publisherStructure[publisherId].status;
}

PubSubId InitPublisher(Status *status, const char *targetIPAddressString, const int pubSubPort,
                       const unsigned char *dataBufferPointer, const unsigned int *dataBufferLength,
                       const long updateDelay) {
    PubSubId publisherId = 0;
    for (int i = 1; i < MAX_ALLOWED_SUBSCRIBER; i++) {
        if (subscriberStructure[i].udpSocket == INVALID_SOCKET) {
            publisherId = i;
            break;
        }
    }
    if (0 != publisherId) {
        status = &publisherStructure[publisherId].status;
        *status = SUCCESS;
        publisherStructure[publisherId].delay.tv_nsec = updateDelay;
        publisherStructure[publisherId].delay.tv_sec = 0;
        publisherStructure[publisherId].buffer.pointer = (unsigned char *) dataBufferPointer;
        publisherStructure[publisherId].buffer.length = (unsigned int *) dataBufferLength;
        memset(&publisherStructure[publisherId].socketAddress, 0, sizeof(struct sockaddr_in));

        publisherStructure[publisherId].socketAddress.sin_family = AF_INET;
        publisherStructure[publisherId].socketAddress.sin_port = htons(pubSubPort);
        publisherStructure[publisherId].socketAddress.sin_addr.s_addr = inet_addr(targetIPAddressString);
        publisherStructure[publisherId].udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (publisherStructure[publisherId].udpSocket == INVALID_SOCKET) {
            publisherStructure[publisherId].status = FAILURE_SOCKET_INVALID;
        } else {
            const int pthreadStatus =
                    pthread_create(&publisherStructure[publisherId].threadId, NULL, Publish,
                                   (void *) (intptr_t) publisherId);
            if (pthreadStatus != 0) {
                *status = FAILURE_THREAD_CREATE;
                CLOSE_SOCKET(publisherStructure[publisherId].udpSocket);
                publisherStructure[publisherId].udpSocket = INVALID_SOCKET;
            } else { *status = SUCCESS; }
        }
    } else {
        *status = FAILURE;
    }
    return publisherId;
}

void UnPublish(PubSubId publisherId) {
    if (publisherId > 0 && publisherId < MAX_ALLOWED_PUBLISHER) {
        publisherStructure[publisherId].status = KILL;
        pthread_join(publisherStructure[publisherId].threadId, NULL);
    }
    publisherStructure[publisherId].udpSocket = INVALID_SOCKET;
}

void UnSubscribe(PubSubId subscriberId) {
    if (subscriberId > 0 && subscriberId < MAX_ALLOWED_SUBSCRIBER) {
        subscriberStructure[subscriberId].status = KILL;
        if (subscriberStructure[subscriberId].udpSocket != INVALID_SOCKET) {
            shutdown(subscriberStructure[subscriberId].udpSocket, SHUT_RD);
        }
        subscriberStructure[subscriberId].udpSocket = INVALID_SOCKET;
        pthread_join(subscriberStructure[subscriberId].threadId, NULL);
    }
}
