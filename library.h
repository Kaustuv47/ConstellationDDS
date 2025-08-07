#ifndef CONSTELLATIONDDS_LIBRARY_H
#define CONSTELLATIONDDS_LIBRARY_H

#include <winsock2.h>

#define MAX_BUFFER_SIZE 65535

typedef struct {
    struct sockaddr_in clientIPAddress;
    int clientIPAddressLength;
    int receivedDataLength;
    char dataBuffer[MAX_BUFFER_SIZE];
} ReceivedDataStructure;

// ReceiverInterrupt function pointer
typedef unsigned __stdcall (*_RECEIVER_INTERRUPT_FUNCTION)(void *);

typedef struct {
    int port;
    _RECEIVER_INTERRUPT_FUNCTION _ReceiverInterruptFunction;
} ReceiverConfigStructure;

typedef struct {
    const char *ipAddressPointer;
    int port;
    SOCKET transmitterSocket; // INVALID_SOCKET = not initialized
    struct sockaddr_in destinationAddress; // Stores destination once built
} TransmitterConfigStructure;

// Transmit raw data to a specific IP (reuses connection)
void Transmitter(TransmitterConfigStructure *, char *, int);

// Start the UDP listener in a separate thread
void InitiateConstellation(ReceiverConfigStructure *);

// Configure transmitter
TransmitterConfigStructure CreateTransmitter(const char *ipAddressPointer, int port);

#endif // CONSTELLATIONDDS_LIBRARY_H
