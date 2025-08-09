#include "library.h"
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <Ws2tcpip.h>
#elif __linux__
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
#elif __linux__
void *ReceiverInterrupt(void *arg) {
    ReceivedDataStructure *receivedDataStructure = (ReceivedDataStructure *)arg;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(receivedDataStructure->clientIPAddress.sin_addr), ip, sizeof(ip));
    int port = ntohs(receivedDataStructure->clientIPAddress.sin_port);
    printf("[From %s:%d] - %.*s\n", ip, port, receivedDataStructure->receivedDataLength,
           receivedDataStructure->dataBuffer);
    free(receivedDataStructure);
    fflush(stdout);
    return NULL;
}
#endif




int main() {
    TransmitterConfigStructure transmitterConfigStructure = CreateTransmitter("127.0.0.1");
    InitiateConstellation(RECEIVER_INTERRUPT_HANDLER);
    SleepForMs(2000);

    while (1) {
        SleepForMs(1000);
        Transmitter(&transmitterConfigStructure, "From Org", sizeof("From Org"));
    }

    return 0;
}
