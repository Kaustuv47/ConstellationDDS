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

/**
 * @typedef socket_t
 * @brief Socket type alias for Linux platform.
 */
typedef int socket_t;

typedef struct {
    socket_t udpSocket; /**< Single UDP Socket */

    RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction; /**< Receiver Function */

    pthread_t receiverThread; /**< Receiver Thread Pointer*/
    Status receivingThreadStatus; /**< Receiver Thread Status*/

    struct sockaddr_in receiverSocketAddress; /**< Receiver Socket Structure*/
    struct sockaddr_in transmitterSocketAddress; /**< Transmitter Socket Structure*/
} ConstellationStructure;

static ConstellationStructure constellationStructure = {0};


void ConstellationReceiver(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    if (bind(constellationStructure.udpSocket,
             (struct sockaddr *) &constellationStructure.receiverSocketAddress,
             sizeof(constellationStructure.receiverSocketAddress)) == SOCKET_ERROR) {
        constellationStructure.receivingThreadStatus = FAILURE_SOCKET_BIND;
    } else {
        struct sockaddr_in clientIPAddress; /**< IP address of the client who sent the data */
        int clientIPAddressLength; /**< Length of the client IP address structure */
        ReceivedDataStructure receivedDataStructure; /**< Receiver structure */

        while (true) {
            receivedDataStructure.receivedDataLength =
                    recvfrom(constellationStructure.udpSocket, receivedDataStructure.dataBuffer,
                             sizeof(receivedDataStructure.dataBuffer), 0, (struct sockaddr *) &clientIPAddress,
                             (socklen_t *) &clientIPAddressLength);
            constellationStructure.ReceiverInterruptFunction(&receivedDataStructure);
        }
    }
    pthread_exit(NULL);
}

Status *InitiateConstellation(RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction, int hostPort,
                              const char *targetIPAddressPointer, int targetPort) {
    constellationStructure.receivingThreadStatus = SUCCESS;

    // Create UDP Socket
    constellationStructure.udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (constellationStructure.udpSocket == INVALID_TRANSMITTER_SOCKET) {
        constellationStructure.receivingThreadStatus = FAILURE_SOCKET_INVALID;
    } else {
        constellationStructure.ReceiverInterruptFunction = ReceiverInterruptFunction;
        constellationStructure.receivingThreadStatus = SUCCESS;
        constellationStructure.receiverSocketAddress.sin_family = AF_INET;
        constellationStructure.receiverSocketAddress.sin_port = htons(hostPort);
        constellationStructure.receiverSocketAddress.sin_addr.s_addr = INADDR_ANY;

        // Create Receiver Thread

        int pthreadStatus = pthread_create(&constellationStructure.receiverThread, NULL,
                                           (void *) &ConstellationReceiver,
                                           NULL);
        if (pthreadStatus != 0) {
            constellationStructure.receiverThread = NULL;
            switch (pthreadStatus) {
                case EAGAIN: constellationStructure.receivingThreadStatus = FAILURE_THREAD_CREATE_EAGAIN;
                    break;
                case EINVAL: constellationStructure.receivingThreadStatus = FAILURE_THREAD_CREATE_EINVAL;
                    break;
                case EPERM: constellationStructure.receivingThreadStatus = FAILURE_THREAD_CREATE_EPERM;
                    break;
                default: constellationStructure.receivingThreadStatus = FAILURE_THREAD_CREATE;
            }
        } else {
            memset(&constellationStructure.transmitterSocketAddress, 0,
                   sizeof(constellationStructure.transmitterSocketAddress));

            constellationStructure.transmitterSocketAddress.sin_family = AF_INET;
            constellationStructure.transmitterSocketAddress.sin_port = htons(targetPort);
            constellationStructure.transmitterSocketAddress.sin_addr.s_addr = inet_addr(
                targetIPAddressPointer);
        }

        SleepForMs(500);
    }
    return &constellationStructure.receivingThreadStatus;
}


Status Transmitter(const char *dataBufferPointer, const int dataBufferLength) {
    Status status = SUCCESS;
    if (constellationStructure.udpSocket == INVALID_TRANSMITTER_SOCKET) {
        status = FAILURE_SOCKET_INVALID;
    }

    long result = sendto(
        constellationStructure.udpSocket,
        dataBufferPointer,
        dataBufferLength,
        0,
        (struct sockaddr *) &constellationStructure.transmitterSocketAddress,
        sizeof(constellationStructure.transmitterSocketAddress)
    );

    if (result == SOCKET_ERROR) {
        status = FAILURE_SOCKET_SENDTO;
    }
    return status;
}

void DeInitiateConstellation() {
    // Example: Set a global flag to request thread exit (you need to implement it)
    if (constellationStructure.receiverThread) {
        pthread_cancel(constellationStructure.receiverThread); // Send cancellation request
        pthread_join(constellationStructure.receiverThread, NULL); // Wait for the thread to terminate
    }
}
