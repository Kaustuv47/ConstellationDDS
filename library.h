#ifndef CONSTELLATIONDDS_LIBRARY_H
#define CONSTELLATIONDDS_LIBRARY_H

#define MAX_BUFFER_SIZE 65535
#define MAX_ALLOWED_PUBLISHER 100
#define MAX_ALLOWED_SUBSCRIBER 100

typedef int PubSubId;
typedef enum {
    ACTIVE,
    PASSIVE,
    INACTIVE,
    DESTROYED
} State;

typedef enum {
    SUCCESS = 0, /**< Operation completed successfully */

    /* Generic errors */
    FAILURE, /**< Unspecified failure */
    KILL,
    FAILURE_UNKNOWN, /**< Unknown error */

    /* Socket-related errors */
    FAILURE_SOCKET_CREATE, /**< socket() failed */
    FAILURE_SOCKET_BIND, /**< bind() failed */
    FAILURE_SOCKET_CLOSE, /**< close()/closesocket() failed */
    FAILURE_SOCKET_INVALID, /**< Invalid socket descriptor */
    FAILURE_SOCKET_OPTION, /**< setsockopt/getsockopt failed */
    FAILURE_SOCKET_CONNECT, /**< connect() failed */
    FAILURE_SOCKET_LISTEN, /**< listen() failed */
    FAILURE_SOCKET_ACCEPT, /**< accept() failed */
    FAILURE_SOCKET_SEND, /**< send() failed */
    FAILURE_SOCKET_SENDTO, /**< sendto() failed */
    FAILURE_SOCKET_RECV, /**< recv() failed */
    FAILURE_SOCKET_RECVFROM, /**< recvfrom() failed */

    /* Address / IP errors */
    FAILURE_INVALID_IP, /**< Invalid IP address string */
    FAILURE_INVALID_PORT, /**< Invalid port number */
    FAILURE_ADDR_RESOLVE, /**< getaddrinfo/inet_pton failed */

    /* Platform-specific errors */
    FAILURE_WSA_STARTUP, /**< Windows WSAStartup failed */
    FAILURE_WSA_CLEANUP, /**< Windows WSACleanup failed */

    /* Threading / concurrency errors */
    FAILURE_THREAD_CREATE, /**< pthread_create() failed (generic) */
    FAILURE_THREAD_CREATE_EAGAIN, /**< Insufficient resources / process limit reached */
    FAILURE_THREAD_CREATE_EINVAL, /**< Invalid settings in attr */
    FAILURE_THREAD_CREATE_EPERM, /**< No permission to set scheduling parameters */
    FAILURE_THREAD_JOIN, /**< pthread_join() failed */
    FAILURE_THREAD_DETACH, /**< pthread_detach() failed */
    FAILURE_THREAD_ATTR_INIT, /**< pthread_attr_init failed */
    FAILURE_THREAD_ATTR_SET, /**< pthread_attr_set... failed */
    FAILURE_MUTEX_INIT, /**< pthread_mutex_init failed */
    FAILURE_MUTEX_LOCK, /**< pthread_mutex_lock failed */
    FAILURE_MUTEX_UNLOCK, /**< pthread_mutex_unlock failed */
    FAILURE_CONDITION_INIT, /**< pthread_cond_init failed */
    FAILURE_CONDITION_WAIT, /**< pthread_cond_wait failed */
    FAILURE_CONDITION_SIGNAL, /**< pthread_cond_signal failed */

    /* Memory errors */
    FAILURE_MEMORY_ALLOC, /**< malloc/calloc failed */
    FAILURE_MEMORY_FREE, /**< free() failed (invalid pointer) */
    FAILURE_BUFFER_OVERFLOW, /**< Buffer overflow / too small */

    /* Configuration / state errors */
    FAILURE_INVALID_STATE, /**< Invalid state (e.g., transmitter inactive) */
    FAILURE_NOT_INITIALIZED, /**< Module not initialized */
    FAILURE_ALREADY_INITIALIZED, /**< Module already initialized */
    FAILURE_NULL_POINTER, /**< Null pointer passed */
    FAILURE_INDEX_OUT_OF_RANGE /**< Array index out of range */
} Status;

/**
 * @brief Initializes and starts a continuous UDP data publisher.
 *
 * This function sets up a UDP socket and begins a recurring task (e.g., a thread)
 * to continuously transmit the provided raw data buffer to the specified target
 * IP and port. The data to be sent must remain valid and unchanged for the entire
 * duration the publisher is active.
 *
 * @param[out] status Pointer to an output variable that receives the operation's status code (SUCCESS or error).
 * @param[in] targetIPAddressString The destination IP address (e.g., "192.168.1.100") as a null-terminated string.
 * @param[in] pubSubPort The destination UDP port number (e.g., 5000).
 * @param[in] dataBufferPointer A pointer to the raw data buffer (payload) to be transmitted.
 * @param[in] dataBufferLength The length of the data buffer in bytes.
 * @param[in] updateDelay The pause time between successive transmissions (e.g., in milliseconds or microseconds).
 * @return PubSubId A unique identifier (handle) for the running publisher instance. A negative or specific value indicates initialization failure.
 */
PubSubId InitPublisher(Status *status, const char *targetIPAddressString, int pubSubPort,
                       const unsigned char *dataBufferPointer, const unsigned int *dataBufferLength,
                       long updateDelay);

/**
 * @brief Initializes and starts a continuous UDP data stream subscriber.
 *
 * This function sets up a UDP socket to bind to the specified local IP and port,
 * and enters a continuous listening mode (e.g., a thread) to receive incoming
 * data packets from a publisher. Received data is typically copied to an internal
 * or user-provided buffer.
 *
 * @param[out] status Pointer to an output variable that receives the operation's status code (SUCCESS or error).
 * @param[in] targetIPAddressString The local IP address (e.g., "0.0.0.0" for all interfaces) to bind the socket to.
 * @param[in] pubSubPort The UDP port number to listen on.
 * @param[out] dataBufferPointer A pointer to the buffer where received data will be copied for the user.
 * @param[in] dataBufferLength The maximum capacity of the data buffer in bytes.
 * @param[in] updateDelay This parameter's usage is context-specific; it may control the listening loop frequency or be unused.
 * @return PubSubId A unique identifier (handle) for the running subscriber instance. A negative or specific value indicates initialization failure.
 */
PubSubId InitSubscriber(Status *status, const char *targetIPAddressString, int pubSubPort,
                        const unsigned char *dataBufferPointer, const unsigned int *dataBufferLength,
                        long updateDelay);

/**
 * @brief Stops the publisher and cleans up associated resources.
 *
 * This function halts the data transmission thread, closes the underlying UDP socket,
 * and frees any internal memory allocated for the publisher instance identified by its ID.
 *
 * @param[in] publisherId The ID returned by InitPublisher.
 */
void UnPublish(PubSubId publisherId);

/**
 * @brief Stops the subscriber and cleans up associated resources.
 *
 * This function halts the continuous listening thread, closes the underlying UDP socket,
 * and frees any internal memory allocated for the subscriber instance identified by its ID.
 *
 * @param[in] subscriberId The ID returned by InitSubscriber.
 */
void UnSubscribe(PubSubId subscriberId);
#endif // CONSTELLATIONDDS_LIBRARY_H
