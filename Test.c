#include "library.h"
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <Ws2tcpip.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#endif



#ifdef _WIN32
unsigned __stdcall RECEIVER_INTERRUPT_HANDLER(void *arg) {
    ReceivedDataStructure *receivedDataStructure = (ReceivedDataStructure *) arg;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(receivedDataStructure->clientIPAddress.sin_addr), ip, sizeof(ip));
    int port = ntohs(receivedDataStructure->clientIPAddress.sin_port);
    printf("[From %s:%d] - %.*s\n", ip, port, receivedDataStructure->receivedDataLength,
           receivedDataStructure->dataBuffer);
    EXIT_RECEIVER_INTERRUPT(receivedDataStructure);
    return 0;
}
#else
void *RECEIVER_INTERRUPT_HANDLER(void *arg) {
    ReceivedDataStructure *receivedDataStructure = (ReceivedDataStructure *)arg;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(receivedDataStructure->clientIPAddress.sin_addr), ip, sizeof(ip));
    int port = ntohs(receivedDataStructure->clientIPAddress.sin_port);
    printf("[From %s:%d] - %.*s\n", ip, port, receivedDataStructure->receivedDataLength,
           receivedDataStructure->dataBuffer);
    free(receivedDataStructure);
    return NULL;
}
#endif




int main() {
    InitiateConstellation(RECEIVER_INTERRUPT_HANDLER, 47474);
    TransmitterID transmitterID = CreateTransmitter("127.0.0.1", 47474);
    printf("%i\n", transmitterID);
    SleepForMs(2000);

    while (1) {
        SleepForMs(1000);
        Transmitter(transmitterID, "From Org", sizeof("From Org"));
    }
    return 0;
}
