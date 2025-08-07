#include "library.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <Ws2tcpip.h>

unsigned __stdcall ReceiverInterrupt(void *arg) {
    printf("Received");
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

int main() {
    ReceiverConfigStructure *receiverConfigStructure = malloc(sizeof(ReceiverConfigStructure));
    receiverConfigStructure->port = 47474;
    receiverConfigStructure->_ReceiverInterruptFunction = ReceiverInterrupt;

    int MaximumTransmitters = 1;
    TransmitterConfigStructure transmitterConfigStructure[MaximumTransmitters];

    transmitterConfigStructure[0] = CreateTransmitter("127.0.0.1", 47474);

    Sleep(2000);
    InitiateConstellation(receiverConfigStructure);



    while (1) {
        Sleep(1000);
        Transmitter(&transmitterConfigStructure[0], "Hello", sizeof("Hello"));
    }

    return 0;
}
