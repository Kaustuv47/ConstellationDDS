#ifndef CONSTELLATIONDDS_LIBRARY_H
#define CONSTELLATIONDDS_LIBRARY_H

#define MAX_BUFFER_SIZE 65535

typedef int TransmitterID;

typedef enum {
    ACTIVE,
    PASSIVE,
    INACTIVE,
    DESTROYED
}State;

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

Status receivingThreadStatus;

#ifdef _WIN32
#include <winsock2.h>

/**
 * @typedef socket_t
 * @brief Socket type alias for Windows platform.
 */
typedef SOCKET socket_t;

/**
 * @typedef _RECEIVER_INTERRUPT_FUNCTION
 * @brief Function pointer type for receiver interrupt callback (Windows).
 *
 * The callback function takes a pointer to received data and returns a thread exit code.
 * @return unsigned Thread exit code.
 */
typedef unsigned __stdcall (*RECEIVER_INTERRUPT_FUNCTION)(void *);


/**
 * @def SleepForMs
 * @brief Sleep for specified milliseconds (Windows).
 */
#define SleepForMs(x) Sleep(x)

#else

#include <netinet/in.h>

/**
 * @typedef socket_t
 * @brief Socket type alias for Linux platform.
 */
typedef int socket_t;

/**
 * @typedef _RECEIVER_INTERRUPT_FUNCTION
 * @brief Function pointer type for receiver interrupt callback (Linux).
 *
 * The callback function takes a pointer to received data and returns a thread exit code.
 * @return void * Thread exit pointer.
 */
typedef void *(*RECEIVER_INTERRUPT_FUNCTION)(void *);

/**
 * @def SleepForMs
 * @brief Sleep for specified milliseconds (Linux).
 */
#define SleepForMs(x) usleep((x)*1000)

#endif

/**
 * @struct ReceivedDataStructure
 * @brief Holds information about a single UDP message received.
 */
typedef struct {
    struct sockaddr_in clientIPAddress; /**< IP address of the client who sent the data */
    int clientIPAddressLength; /**< Length of the client IP address structure */
    ssize_t receivedDataLength; /**< Number of bytes received in this message */
    char dataBuffer[MAX_BUFFER_SIZE]; /**< Buffer containing the received data */
} ReceivedDataStructure;

/**
 * @brief Starts the UDP listener thread.
 *
 * @param ReceiverInterruptFunction Callback function invoked for each received UDP message.
 * @param port Destination port as an integer.
 */
Status InitiateConstellation(RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction, int port);

/**
 * @brief Creates and configures a UDP transmitter.
 *
 * @param ipAddressPointer Destination IP address as a null-terminated string.
 * @param port Destination port as an integer.
 * @return TransmitterConfigStructure Configured transmitter structure.
 */
TransmitterID CreateTransmitter(const char *ipAddressPointer, int port);

/**
 * @brief Sends raw data to the configured destination IP and port.
 *
 * @param transmitterID Transmitter Identifier.
 * @param dataBufferPointer Pointer to the raw data buffer to send.
 * @param dataBufferLength Length of the data buffer in bytes.
 * @return Status Returns SUCCESS on success or a specific error code on failure.
 */
Status Transmitter(int transmitterID, const char *dataBufferPointer, int dataBufferLength);

/**
 * @brief Cleans up resources used by a receiver thread after processing.
 *
 * @param receivedDataStructure Pointer to the received data structure to free.
 * @return unsigned Thread exit code (Windows) or ignored (Linux).
 */
unsigned EXIT_RECEIVER_INTERRUPT(ReceivedDataStructure *receivedDataStructure);

/**
 * @brief Destroys and cleans up the transmitter socket.
 *
 * @param transmitterID Transmitter Identifier.
 */
void DestroyTransmitter(int transmitterID);


/**
 * @brief Requests termination of the UDP listener thread and cleans up.
 */
void DeInitiateConstellation();

#endif // CONSTELLATIONDDS_LIBRARY_H

