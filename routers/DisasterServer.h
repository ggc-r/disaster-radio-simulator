#ifndef DISASTERSERVER_H
#define DISASTERSERVER_H

#include <cstddef>
#include <LoRaLayer2.h>

#include "DisasterClient.h"

#define DATAGRAM_HEADER 5
#define DATAGRAM_MESSAGE 239

class DisasterClient;

class DisasterServer
{
public:
    virtual DisasterClient* connect(DisasterClient *client) = 0;
    virtual void disconnect(DisasterClient *client) = 0;

    virtual void setup() = 0;
    virtual void loop() = 0;

    virtual void transmit(DisasterClient *client, struct Datagram datagram, size_t len) = 0;
};

#endif
