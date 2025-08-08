#ifndef CONSTELLATIONDDS_LIBRARY_H
#define CONSTELLATIONDDS_LIBRARY_H

#define MAX_BUFFER_SIZE 65535

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET socket_t;
typedef unsigned __stdcall (*_RECEIVER_INTERRUPT_FUNCTION)(void *);
#elif __linux__
#include <sys/socket.h>
typedef int socket_t;
typedef void *(*_RECEIVER_INTERRUPT_FUNCTION)(void *);
#endif

typedef struct {
    struct sockaddr_in clientIPAddress;
    int clientIPAddressLength;
    int receivedDataLength;
    char dataBuffer[MAX_BUFFER_SIZE];
} ReceivedDataStructure;

typedef struct {
    int port;
    _RECEIVER_INTERRUPT_FUNCTION _ReceiverInterruptFunction;
} ReceiverConfigStructure;

typedef struct {
    const char *ipAddressPointer;
    int port;
    socket_t transmitterSocket; // INVALID_TRANSMITTER_SOCKET = not initialized
    struct sockaddr_in destinationAddress; // Stores destination once built
} TransmitterConfigStructure;

// Transmit raw data to a specific IP (reuses connection)
void Transmitter(TransmitterConfigStructure *, char *, int);

// Start the UDP listener in a separate thread
void InitiateConstellation(ReceiverConfigStructure *);

// Configure transmitter
TransmitterConfigStructure CreateTransmitter(const char *ipAddressPointer, int port);

#endif // CONSTELLATIONDDS_LIBRARY_H
