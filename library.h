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
void InitiateConstellation(RECEIVER_INTERRUPT_FUNCTION ReceiverInterruptFunction, int port);

/**
 * @brief Creates and configures a UDP transmitter.
 *
 * @param ipAddressPointer Destination IP address as a null-terminated string.
 * @param port Destination port as an integer.
 * @return TransmitterConfigStructure Configured transmitter structure.
 */
int CreateTransmitter(const char *ipAddressPointer, int port);

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

