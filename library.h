#ifndef CONSTELLATIONDDS_LIBRARY_H
#define CONSTELLATIONDDS_LIBRARY_H

#include <winsock2.h>
#include <Ws2tcpip.h>

typedef struct {
    SOCKET socket;
    struct sockaddr_in address;
} clientInfoStructure;

// Deserializer function pointer
typedef void (*DeserializerFunctionPointer)(const char *data, int length, void *client_context);

// Transmit raw data to a specific IP (reuses connection)
void Transmitter(const char *ip, const char *data, int length);

// Start the UDP listener in a separate thread
void InitiateConstellation(DeserializerFunctionPointer deserializerFunctionPointer);

#endif // CONSTELLATIONDDS_LIBRARY_H
