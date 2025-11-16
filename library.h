#pragma once
#define CONSTELLATIONDDS_LIBRARY_H
#define MAX_BUFFER_SIZE 65535

#include <netinet/in.h>

typedef enum {
    SUCCESS = 0,                  /**< Operation completed successfully */

    /* Generic errors */
    FAILURE,                      /**< Unspecified failure */
    FAILURE_UNKNOWN,              /**< Unknown error */

    /* Socket-related errors */
    FAILURE_SOCKET_CREATE,        /**< socket() failed */
    FAILURE_SOCKET_BIND,          /**< bind() failed */
    FAILURE_SOCKET_CLOSE,         /**< close()/closesocket() failed */
    FAILURE_SOCKET_INVALID,       /**< Invalid socket descriptor */
    FAILURE_SOCKET_OPTION,        /**< setsockopt/getsockopt failed */
    FAILURE_SOCKET_CONNECT,       /**< connect() failed */
    FAILURE_SOCKET_LISTEN,        /**< listen() failed */
    FAILURE_SOCKET_ACCEPT,        /**< accept() failed */
    FAILURE_SOCKET_SEND,          /**< send() failed */
    FAILURE_SOCKET_SENDTO,        /**< sendto() failed */
    FAILURE_SOCKET_RECV,          /**< recv() failed */
    FAILURE_SOCKET_RECVFROM,      /**< recvfrom() failed */

    /* Address / IP errors */
    FAILURE_INVALID_IP,           /**< Invalid IP address string */
    FAILURE_INVALID_PORT,         /**< Invalid port number */
    FAILURE_ADDR_RESOLVE,         /**< getaddrinfo/inet_pton failed */

    /* Platform-specific errors */
    FAILURE_WSA_STARTUP,          /**< Windows WSAStartup failed */
    FAILURE_WSA_CLEANUP,          /**< Windows WSACleanup failed */

    /* Threading / concurrency errors */
    FAILURE_THREAD_CREATE,            /**< pthread_create() failed (generic) */
    FAILURE_THREAD_CREATE_EAGAIN,     /**< Insufficient resources / process limit reached */
    FAILURE_THREAD_CREATE_EINVAL,     /**< Invalid settings in attr */
    FAILURE_THREAD_CREATE_EPERM,      /**< No permission to set scheduling parameters */
    FAILURE_THREAD_JOIN,              /**< pthread_join() failed */
    FAILURE_THREAD_DETACH,            /**< pthread_detach() failed */
    FAILURE_THREAD_ATTR_INIT,         /**< pthread_attr_init failed */
    FAILURE_THREAD_ATTR_SET,          /**< pthread_attr_set... failed */
    FAILURE_MUTEX_INIT,               /**< pthread_mutex_init failed */
    FAILURE_MUTEX_LOCK,               /**< pthread_mutex_lock failed */
    FAILURE_MUTEX_UNLOCK,             /**< pthread_mutex_unlock failed */
    FAILURE_CONDITION_INIT,           /**< pthread_cond_init failed */
    FAILURE_CONDITION_WAIT,           /**< pthread_cond_wait failed */
    FAILURE_CONDITION_SIGNAL,         /**< pthread_cond_signal failed */

    /* Memory errors */
    FAILURE_MEMORY_ALLOC,             /**< malloc/calloc failed */
    FAILURE_MEMORY_FREE,              /**< free() failed (invalid pointer) */
    FAILURE_BUFFER_OVERFLOW,          /**< Buffer overflow / too small */

    /* Configuration / state errors */
    FAILURE_INVALID_STATE,            /**< Invalid state (e.g., transmitter inactive) */
    FAILURE_NOT_INITIALIZED,          /**< Module not initialized */
    FAILURE_ALREADY_INITIALIZED,      /**< Module already initialized */
    FAILURE_NULL_POINTER,             /**< Null pointer passed */
    FAILURE_INDEX_OUT_OF_RANGE        /**< Array index out of range */
} Status;

/**
 * @typedef _RECEIVER_INTERRUPT_FUNCTION
 * @brief Function pointer type for receiver interrupt callback (Linux).
 *
 * The callback function takes a pointer to receive data and returns a thread exit code.
 * @return void * Thread exit pointer.
 */
typedef void *(*RECEIVER_INTERRUPT_FUNCTION)(void *);

/**
 * @def SleepForMs
 * @brief Sleep for specified milliseconds (Linux).
 */
#define SleepForMs(x) usleep((x)*1000)

/**
 * @struct ReceivedDataStructure
 * @brief Holds information about a single UDP message received.
 */
typedef struct {
    ssize_t receivedDataLength; /**< Number of bytes received in this message */
    char dataBuffer[MAX_BUFFER_SIZE]; /**< Buffer containing the received data */
} ReceivedDataStructure;

/**
 * @brief Starts the UDP listener thread.
 *
 * @param ReceiverInterruptFunction Callback function invoked for each received UDP message.
 * @param hostPort
 * @param targetIPAddressPointer
 * @param targetPort
 */
Status* InitiateConstellation(RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction, int hostPort, const char *targetIPAddressPointer, int targetPort);

/**
 * @brief Sends raw data to the configured destination IP and port.
 *
 * @param dataBufferPointer Pointer to the raw data buffer to send.
 * @param dataBufferLength Length of the data buffer in bytes.
 * @return Status Returns SUCCESS on success or a specific error code on failure.
 */
Status Transmitter(const char *dataBufferPointer, int dataBufferLength);

/**
 * @brief Requests termination of the UDP listener thread and cleans up.
 */
void DeInitiateConstellation();
// CONSTELLATIONDDS_LIBRARY_H

