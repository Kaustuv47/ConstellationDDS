#include "library.h"
#include <stdio.h>
#include <string.h>



#ifdef _WIN32
#include <windows.h>
#include <Ws2tcpip.h>

#define SleepForMs(x) Sleep(x)
#elif __linux__
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define SleepForMs(x) usleep((x)*1000)
#endif



#ifdef _WIN32
unsigned __stdcall ReceiverInterrupt(void *arg) {
    ReceivedDataStructure *receivedDataStructure = (ReceivedDataStructure *) arg;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(receivedDataStructure->clientIPAddress.sin_addr), ip, sizeof(ip));
    int port = ntohs(receivedDataStructure->clientIPAddress.sin_port);
    printf("[From %s:%d] - %.*s\n", ip, port, receivedDataStructure->receivedDataLength,
           receivedDataStructure->dataBuffer);
    free(receivedDataStructure);
    fflush(stdout);
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
    ReceiverConfigStructure *receiverConfigStructure = malloc(sizeof(ReceiverConfigStructure));
    receiverConfigStructure->port = 47474;
    receiverConfigStructure->_ReceiverInterruptFunction = ReceiverInterrupt;

    int MaximumTransmitters = 1;
    TransmitterConfigStructure transmitterConfigStructure[MaximumTransmitters];
    transmitterConfigStructure[0] = CreateTransmitter("127.0.0.1", 47474);

    SleepForMs(2000);
    InitiateConstellation(receiverConfigStructure);

    while (1) {
        SleepForMs(7000);
        Transmitter(&transmitterConfigStructure[0], "Hello", sizeof("Hello"));
    }

    return 0;
}
