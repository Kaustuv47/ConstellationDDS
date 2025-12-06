#ifndef CONSTELLATIONDDS_LIBRARY_H
#define CONSTELLATIONDDS_LIBRARY_H

#define MAX_BUFFER_SIZE 65535
#define MAX_PUBSUB_INSTANCES 500

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
 * @brief Sends raw data to a target via UDP at a specified delay.
 *
 * This function handles the continuous transmission of data using a pre-configured UDP socket.
 * It packages the provided data and sends it to the specified IP address and port,
 * pausing between transmissions according to the update delay.
 *
 * @param pubSubInstancePointer A pointer to the PubSub instance's internal structure.
 * @param targetIPAddressString The destination IP address (publisher target) as a null-terminated string.
 * @param pubSubPort The destination UDP port number.
 * @param dataBufferPointer A pointer to the raw data buffer (data payload) to be transmitted.
 * @param dataBufferLength The length of the data buffer in bytes.
 * @param updateDelay The delay between successive transmissions (e.g., in nanoseconds or milliseconds).
 * @return Status* Returns a pointer to the internal status variable (SUCCESS on success or a specific error code on failure).
 */
Status *Publish(void *pubSubInstancePointer, const char *targetIPAddressString, int pubSubPort,
                const unsigned int *dataBufferPointer, unsigned int dataBufferLength, long updateDelay);

/**
 * @brief Subscribes to a publisher's data stream via UDP.
 *
 * This function sets up a UDP socket to bind to the specified port and
 * IP address, then enters a loop to continuously listen for incoming
 * data packets from a publisher.
 *
 * @param pubSubInstancePointer A pointer to the PubSub structure instance (e.g., for state management).
 * @param targetIPAddressString The IP address string (e.g., "192.168.1.10") to bind the subscriber socket to.
 * @param pubSubPort The UDP port number to listen on.
 * @param dataBufferPointer A pointer to the buffer where received data could potentially be copied (Note: this parameter seems unused in typical subscriber loops).
 * @param dataBufferLength The expected maximum length of the data buffer.
 * @param updateDelay The delay (in a context-specific unit, often ms or ns) used for socket configuration or loop control.
 * @return Status* Returns a pointer to the internal status variable (SUCCESS or a specific error code like FAILURE_SOCKET_BIND).
 */
Status *Subscribe(void *pubSubInstancePointer, const char *targetIPAddressString, int pubSubPort,
                  const unsigned int *dataBufferPointer, unsigned int dataBufferLength, long updateDelay);

/**
 * @brief Cleans up and releases resources associated with the PubSub instance.
 *
 * This function should be called when the publisher or subscriber is no longer needed.
 * It is responsible for closing the underlying UDP socket, freeing any allocated
 * memory for internal structures, and ensuring the PubSub instance is properly
 * shut down.
 *
 * @param pubSubInstancePointer A pointer to the PubSub structure instance to be cleaned up.
 */
void UnPubSub(void *pubSubInstancePointer);
#endif // CONSTELLATIONDDS_LIBRARY_H
