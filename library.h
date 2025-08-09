#ifndef CONSTELLATIONDDS_LIBRARY_H
#define CONSTELLATIONDDS_LIBRARY_H

#define MAX_BUFFER_SIZE 65535

typedef enum {
    SUCCESS = 0,              /**< Operation completed successfully */
    FAILURE = 1,              /**< Generic failure */
    SOCKET_CREATE_ERROR = 2,  /**< Socket creation failed */
    SOCKET_BIND_ERROR = 3,    /**< Socket bind failed */
    WSA_STARTUP_ERROR = 4,    /**< Windows WSAStartup failed */
    INVALID_IP_ERROR = 5,     /**< Invalid IP address string */
    THREAD_CREATE_ERROR = 6,  /**< Thread creation failed */
    MEMORY_ALLOC_ERROR = 7,   /**< Memory allocation failed */
    SENDTO_ERROR = 8,         /**< sendto() function failed */
    INVALID_SOCKET_ERROR = 9,
    UNKNOWN_ERROR = 99        /**< Unknown error */
} Status;

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

#elif __linux__

#include <sys/socket.h>


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
typedef void *(*_RECEIVER_INTERRUPT_FUNCTION)(void *);

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
    int receivedDataLength; /**< Number of bytes received in this message */
    char dataBuffer[MAX_BUFFER_SIZE]; /**< Buffer containing the received data */
} ReceivedDataStructure;


/**
 * @struct TransmitterConfigStructure
 * @brief Configuration and socket info for sending UDP messages.
 */
typedef struct {
    const char *ipAddressPointer; /**< Destination IP address as a string */
    int port; /**< Destination port number */
    socket_t transmitterSocket; /**< UDP socket used for transmission */
    struct sockaddr_in destinationAddress; /**< Cached destination address struct */
} TransmitterConfigStructure;

/**
 * @brief Starts the UDP listener thread.
 *
 * @param ReceiverInterruptFunction Callback function invoked for each received UDP message.
 */
void InitiateConstellation(RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction);

/**
 * @brief Creates and configures a UDP transmitter.
 *
 * @param ipAddressPointer Destination IP address as a null-terminated string.
 * @return TransmitterConfigStructure Configured transmitter structure.
 */
TransmitterConfigStructure CreateTransmitter(const char *ipAddressPointer);

/**
 * @brief Sends raw data to the configured destination IP and port.
 *
 * @param transmitterConfigStructure Pointer to a configured transmitter structure.
 * @param dataBufferPointer Pointer to the raw data buffer to send.
 * @param dataBufferLength Length of the data buffer in bytes.
 * @return Status Returns SUCCESS on success or a specific error code on failure.
 */
Status Transmitter(TransmitterConfigStructure *transmitterConfigStructure, const char *dataBufferPointer, int dataBufferLength);

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
 * @param transmitterConfigStructure Pointer to transmitter structure to destroy.
 */
void DestroyTransmitter(TransmitterConfigStructure *transmitterConfigStructure);


/**
 * @brief Requests termination of the UDP listener thread and cleans up.
 */
void DeInitiateConstellation();

#endif // CONSTELLATIONDDS_LIBRARY_H

