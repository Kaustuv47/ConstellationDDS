#include "library.h"
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

void *RECEIVER_INTERRUPT_HANDLER(void *arg) {
    ReceivedDataStructure *receivedDataStructure = (ReceivedDataStructure *)arg;
    return NULL;
}

int main() {
    Status *receivingThreadStatus = InitiateConstellation(RECEIVER_INTERRUPT_HANDLER, 47474, "127.0.0.1", 47474);
    SleepForMs(2000);

    while (1) {
        SleepForMs(1000);
        Transmitter("From Org", sizeof("From Org"));
    }
    return 0;
}
