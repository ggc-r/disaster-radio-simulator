#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>
#include "base.h"

#define HEADER_LENGTH 16
#define SHA1_LENGTH 40
#define ADDR_LENGTH 6 

uint8_t mac[ADDR_LENGTH];
char macaddr[ADDR_LENGTH*2];

// global sequence number
uint8_t messageCount = 0;

// mode switches
int retransmitEnabled = 0;
int hashingEnabled = 1;

// timeout intervals
int _helloInterval = 10;
int _routeInterval = 10;
int _messageInterval = 5;
int _discoveryTimeout = 30;
int _learningTimeout = 400;
int _maxRandomDelay = 20;

int simulationTime(int realTime) {
    return realTime * timeDistortion;
}
int helloInterval() {
    return simulationTime(_helloInterval);
}
int routeInterval() {
    return simulationTime(_routeInterval);
}
int messageInterval() {
    return simulationTime(_messageInterval);
}
int discoveryTimeout() {
    return simulationTime(_discoveryTimeout);
}
int learningTimeout() {
    return simulationTime(_learningTimeout);
}
int maxRandomDelay() {
    return simulationTime(_maxRandomDelay);
}

// metric variables
float packetSuccessWeight = .4;
float RSSIWeight = .3;
float SNRWeight = .3;

// packet structures
struct Metadata {
    uint8_t rssi;
    uint8_t snr;
};
struct Packet {
    uint8_t ttl;
    uint8_t totalLength;
    uint8_t source[ADDR_LENGTH];
    uint8_t destination[ADDR_LENGTH];
    uint8_t sequence;
    uint8_t type;
    uint8_t data[240];
};

struct Packet buffer[8];
int bufferEntry = 0;

// table structures
uint8_t hashTable[256][SHA1_LENGTH];
uint8_t hashEntry = 0;

struct NeighborTableEntry{
    uint8_t address[ADDR_LENGTH];
    uint8_t lastReceived;
    uint8_t packet_success;
    uint8_t metric;
};
struct NeighborTableEntry neighborTable[255];
int neighborEntry = 0;

struct RoutingTableEntry{
    uint8_t destination[ADDR_LENGTH];
    uint8_t nextHop[ADDR_LENGTH];
    uint8_t distance;
    uint8_t lastReceived;
    uint8_t metric;
};
struct RoutingTableEntry routeTable[255];
int routeEntry = 0;

int isHashNew(uint8_t hash[SHA1_LENGTH]){

    int hashNew = 1;
    Serial.printf("hash is %x\n", hash);
    for( int i = 0 ; i <= hashEntry ; i++){
        if(strcmp(hash, hashTable[i]) == 0){
            hashNew = 0; 
            Serial.printf("Not new!\n");
        }
    }
    if(hashNew){
        // add to hash table
        Serial.printf("New message received");
        Serial.printf("\r\n");
        for( int i = 0 ; i < SHA1_LENGTH ; i++){
            hashTable[hashEntry][i] = hash[i];
        }
        hashEntry++;
    }
    return hashNew;
}

int sendPacket(struct Packet packet) {

    //if(!loraInitialized){
    //    return;
    //}
    uint8_t* sending = malloc(sizeof(packet));
    memcpy(sending, &packet, sizeof(packet));
    if(hashingEnabled){
        // do not send message if already transmitted once
        uint8_t hash[SHA1_LENGTH];
        SHA1(sending, packet.totalLength, hash);
        if(isHashNew(hash)){
            send_packet(sending, packet.totalLength);
            messageCount++;
        }
    }
    return messageCount;
    /*
    LoRa.beginPacket();
    for( int i = 0 ; i < outgoingLength ; i++){
        LoRa.write(outgoing[i]);
        Serial.printf("%c", outgoing[i]);
    }
    Serial.printf("\r\n");
    LoRa.endPacket();
    LoRa.receive();
    */
}

void pushToBuffer(struct Packet packet){

    if(bufferEntry > 7){
        bufferEntry = 0;
    }

    memset(&buffer[bufferEntry], 0, sizeof(buffer[bufferEntry]));
    memcpy(&buffer[bufferEntry], &packet, sizeof(buffer[bufferEntry]));
    bufferEntry++;
}

struct Packet popFromBuffer(){

    bufferEntry--;
    struct Packet pop;
    memcpy(&pop, &buffer[bufferEntry], sizeof(pop));
    return pop; 
}

void checkBuffer(){

    if (bufferEntry > 0){
        struct Packet packet = popFromBuffer();
        sendPacket(packet);
    }else{
        debug_printf("Buffer is empty\n");
    }
}

void printPacketInfo(struct Packet packet, struct Metadata metadata){

    Serial.printf("\n");
    Serial.printf("Packet Received: \n");
    Serial.printf("RSSI: %x\n", metadata.rssi);
    Serial.printf("SNR: %x\n", metadata.snr);
    Serial.printf("ttl: %d\n", packet.ttl);
    Serial.printf("length: %d\n", packet.totalLength);
    Serial.printf("source: ");
    for(int i = 0 ; i < ADDR_LENGTH ; i++){
        Serial.printf("%x", packet.source[i]);
    }
    Serial.printf("\n");
    Serial.printf("destination: ");
    for(int i = 0 ; i < ADDR_LENGTH ; i++){
        Serial.printf("%x", packet.destination[i]);
    }
    Serial.printf("\n");
    Serial.printf("sequence: %02x\n", packet.sequence);
    Serial.printf("type: %c\n", packet.type);
    Serial.printf("data: ");
    for(int i = 0 ; i < packet.totalLength-HEADER_LENGTH ; i++){
        Serial.printf("%02x", packet.data[i]);
    }
    Serial.printf("\n");
}

void printNeighborTable(){

    Serial.printf("\n");
    Serial.printf("Neighbor Table:\n");
    for( int i = 0 ; i < neighborEntry ; i++){
        for(int j = 0 ; j < ADDR_LENGTH ; j++){
            Serial.printf("%02x", neighborTable[i].address[j]);
        }
        Serial.printf(" %3d ", neighborTable[i].metric);
        Serial.printf("\n");
    }
    Serial.printf("\n");
}

void printRoutingTable(){

    Serial.printf("\n");
    Serial.printf("Routing Table: total routes %d\n", routeEntry);
    for( int i = 0 ; i < routeEntry ; i++){
        Serial.printf("%d hops from ", routeTable[i].distance);
        for(int j = 0 ; j < ADDR_LENGTH ; j++){
            Serial.printf("%02x", routeTable[i].destination[j]);
        }
        Serial.printf(" via ");
        for(int j = 0 ; j < ADDR_LENGTH ; j++){
            Serial.printf("%02x", routeTable[i].nextHop[j]);
        }
        Serial.printf(" metric %3d ", routeTable[i].metric);
        Serial.printf("\n");
    }
    Serial.printf("\n\n");
}

void printAddress(uint8_t address[ADDR_LENGTH]){
    for( int i = 0 ; i < ADDR_LENGTH; i++){
        debug_printf("%02x", address[i]);
    }
}

uint8_t calculatePacketLoss(int entry, uint8_t sequence){

    uint8_t packet_loss;
    uint8_t sequence_diff = sequence - neighborTable[entry].lastReceived;
    if(sequence_diff == 0){
        // this is first packet received from neighbor
        // assume perfect packet success
        neighborTable[entry].packet_success = 0xFF; 
        packet_loss = 0x00; 
    }else if(sequence_diff == 1){
        // do not decrease packet success rate
        packet_loss = 0x00; 
    }else if(sequence_diff > 1 && sequence_diff < 16){
        // decrease packet success rate by difference
        packet_loss = 0x10 * sequence_diff; 
    }else if(sequence_diff > 16){
        // no packet received recently
        // assume complete pakcet loss
        packet_loss = 0xFF; 
    }
    return packet_loss;
}

uint8_t calculateMetric(int entry, uint8_t sequence, struct Metadata metadata){

    float weightedPacketSuccess =  ((float) neighborTable[entry].packet_success)*packetSuccessWeight;
    float weightedRSSI =  ((float) metadata.rssi)*RSSIWeight;
    float weightedSNR =  ((float) metadata.snr)*SNRWeight;
    uint8_t metric = weightedPacketSuccess+weightedRSSI+weightedSNR;
    debug_printf("weighted packet success: %3f\n", weightedPacketSuccess);
    debug_printf("weighted RSSI: %3f\n", weightedRSSI);
    debug_printf("weighted SNR: %3f\n", weightedSNR);
    debug_printf("metric calculated: %3d\n", metric);
    return metric;
}

int checkNeighborTable(struct Packet packet){

    int neighborNew = 1;
    for( int i = 0 ; i < neighborEntry ; i++){
        //had to use memcmp instead of strcmp?
        if(memcmp(packet.source, neighborTable[i].address, sizeof(packet.source)) == 0){
            neighborNew = 0; 
        }
    }
    return neighborNew;
}

int checkRoutingTable(struct RoutingTableEntry route){

    int entry = routeEntry; // assume this is a new route
    for( int i = 0 ; i < routeEntry ; i++){
        if(memcmp(route.destination, routeTable[i].destination, sizeof(route.destination)) == 0){
            if(memcmp(route.nextHop, routeTable[i].nextHop, sizeof(route.nextHop)) == 0){
                // already have this exact route, update metric
                //debug_printf("existing route\n");
                entry = i; 
                return entry;
            }else{
                // already have this destination, but via a different neighbor
                // not sure how to handle this, maybe keep better metric?
                //debug_printf("existing route from another neighbor\n");
                if(route.distance < routeTable[i].distance){
                    // replace route if distance is better 
                    entry = i;
                    return entry;
                }
                entry = -1;
                return entry;
            }
        }else 
        if(memcmp(route.destination, mac, sizeof(route.destination)) == 0){
            //this is me don't add to routing table 
            //debug_printf("this route is my local address\n");
            entry = -1;
            return entry;
        } 
    }
    return entry;
}

int addNeighbor(struct Packet packet, struct Metadata metadata){

    debug_printf("New neighbor found: ");
    for( int i = 0 ; i < ADDR_LENGTH; i++){
        neighborTable[neighborEntry].address[i] = packet.source[i];
        debug_printf("%02x", neighborTable[neighborEntry].address[i]);
    }
    debug_printf("\n");
    neighborTable[neighborEntry].lastReceived = packet.sequence;
    uint8_t packet_loss = calculatePacketLoss(neighborEntry, packet.sequence);
    neighborTable[neighborEntry].packet_success = neighborTable[neighborEntry].packet_success - packet_loss;
    uint8_t metric = calculateMetric(neighborEntry, packet.sequence, metadata); 
    neighborTable[neighborEntry].metric = metric;
    neighborEntry++;
    return neighborEntry-1;
}

int updateRouteTable(struct RoutingTableEntry route, int entry){

    memset(&routeTable[entry], 0, sizeof(routeTable[entry]));
    memcpy(&routeTable[entry], &route, sizeof(routeTable[entry]));
    if(entry == routeEntry){
        routeEntry++;
        debug_printf("new route found! ");
    }else{
        debug_printf("route updated! ");
    }
    printAddress(routeTable[entry].destination);
    debug_printf("\n");
    return entry;
}

void retransmitRoutedPacket(struct Packet packet, struct RoutingTableEntry route){

    // replace with new nextHop
    memcpy(packet.data, route.nextHop, sizeof(packet.data));
    // transmit packet
    pushToBuffer(packet);
    //sendPacket(packet);
}

void parseHelloPacket(struct Packet packet, struct Metadata metadata){

    if(checkNeighborTable(packet)){
        int neighbor = addNeighbor(packet, metadata);  
        struct RoutingTableEntry route;
        memcpy(route.destination, packet.source, ADDR_LENGTH);
        memcpy(route.nextHop, packet.source, ADDR_LENGTH);
        route.distance = 1;
        route.metric = neighborTable[neighbor].metric;
        int entry = checkRoutingTable(route);
        if(entry == -1){
            debug_printf("do nothing, already have route to ");
            printAddress(route.destination);
            debug_printf("\n");
        }else{
            updateRouteTable(route, entry);
        }
    }else{
        // update metric
    }
}

void parseRoutingPacket(struct Packet packet, struct Metadata metadata){
    int numberOfRoutes = (packet.totalLength - HEADER_LENGTH) / (ADDR_LENGTH+2);
    debug_printf("routes in packet: %d\n", numberOfRoutes);

    for( int i = 0 ; i < numberOfRoutes ; i++){
        struct RoutingTableEntry route; 
        memcpy(route.destination, packet.data + (ADDR_LENGTH+2)*i, ADDR_LENGTH);
        memcpy(route.nextHop, packet.source, ADDR_LENGTH);
        route.distance = packet.data[(ADDR_LENGTH+2)*i + ADDR_LENGTH]; 
        route.distance++; // add a hop to distance
        route.metric = packet.data[(ADDR_LENGTH+2)*i + ADDR_LENGTH+1];
        int entry = checkRoutingTable(route);
        if(entry == -1){
            debug_printf("do nothing, already have route to ");
            printAddress(route.destination);
            debug_printf("\n");
        }else{
            updateRouteTable(route, entry);
        }
    }
}

void parseChatPacket(struct Packet packet){
    
    
    if(memcmp(packet.destination, mac, sizeof(packet.destination)) == 0){
        Serial.printf("this message is for me %s\n", macaddr);
        return;
    }

    int hasRoute = 0;
    for( int i = 0 ; i < routeEntry ; i++){
        if(memcmp(packet.destination, routeTable[i].destination, sizeof(packet.destination)) == 0){
            hasRoute = i;
        }
    }
    uint8_t nextHop[ADDR_LENGTH];
    memcpy(nextHop, packet.data, sizeof(nextHop));
    if(memcmp(nextHop, mac, sizeof(nextHop)) == 0){
        Serial.printf("I am the next hop\n");
        if(hasRoute){
            Serial.printf("and I have a route RETRANSMIT\n");
            retransmitRoutedPacket(packet, routeTable[hasRoute]);
            //sendPacket(packet);
        }else{
            Serial.printf("but I don't have a route\n");
        }
    }else{
        Serial.printf("I am not the next hop\n");
        if(hasRoute){
            Serial.printf("but I have a route\n");
        }else{
            Serial.printf("and I don't have a route\n");
        }
    }
}
    
int packet_received(char* data, size_t len) {

    data[len] = '\0';

    // convert ASCII data to pure bytes
    uint8_t* byteData = ( uint8_t* ) data;
    
    // randomly generate RSSI and SNR values 
    // see https://github.com/sudomesh/disaster-radio-simulator/issues/3
    uint8_t packet_rssi = rand() % (256 - 128) + 128;
    uint8_t packet_snr = rand() % (256 - 128) + 128;

    struct Metadata metadata = {
        packet_rssi,
        packet_snr
    };
    struct Packet packet = {
        byteData[0],
        byteData[1], 
        byteData[2], byteData[3], byteData[4], byteData[5], byteData[6], byteData[7],
        byteData[8], byteData[9], byteData[10], byteData[11], byteData[12], byteData[13],
        byteData[14],
        byteData[15],
    };
    memcpy(packet.data, byteData + HEADER_LENGTH, packet.totalLength-HEADER_LENGTH);

    //printPacketInfo(packet, metadata);
    
    switch(packet.type){
        case 'h' :
            // hello packet;
            parseHelloPacket(packet, metadata);
            //printNeighborTable();
            break;
        case 'r':
            // routing packet;
            parseHelloPacket(packet, metadata);
            parseRoutingPacket(packet, metadata);
            //printRoutingTable();
            break;
        case 'c' :
            // chat packet
            parseChatPacket(packet);
            //Serial.printf("this is a chat message\n");
            break;
        case 'm' :
            Serial.printf("this is a map message\n");
            break;
        default :
            printPacketInfo(packet, metadata);
            Serial.printf("message type not found\n");
    }
    return 0;
}


struct Packet buildPacket( uint8_t ttl, uint8_t dest[6], uint8_t type, uint8_t data[240], uint8_t dataLength){

    uint8_t packetLength = HEADER_LENGTH + dataLength;
    uint8_t* buffer = malloc(dataLength);
    buffer = (uint8_t*) data;
    struct Packet packet = {
        ttl,
        packetLength,
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        dest[0], dest[1], dest[2], dest[3], dest[4], dest[5],
        messageCount,
        type 
    };
    memcpy(&packet.data, buffer, packet.totalLength);
    //free(buffer);
    return packet;
}

long lastHelloTime = 0;
void transmitHello(){

    if (time(NULL) - lastHelloTime > helloInterval()) {
        char data[240];
        char message[10] = "Hola from\0";
        sprintf(data, "%s %s", message, macaddr);
        int dataLength = 22;
        //TODO: add randomness to message to avoid hashisng issues
        uint8_t destination[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
        struct Packet helloMessage = buildPacket(1, destination, 'h', data, dataLength); 
        uint8_t* sending = malloc(sizeof(helloMessage));
        memcpy(sending, &helloMessage, sizeof(helloMessage));
        send_packet(sending, helloMessage.totalLength);
        messageCount++;
        lastHelloTime = time(NULL);
        debug_printf("Sending beacon: ");
        for(int i = 0 ; i < helloMessage.totalLength ; i++){
            debug_printf("%02x", sending[i]);
        }
        debug_printf("\r\n");
    }
}

long lastRouteTime = 0;
void transmitRoutes(){

    if (time(NULL) - lastRouteTime > routeInterval()) {
        uint8_t data[240];
        int dataLength = 0;
        debug_printf("number of routes before transmit: %d\n", routeEntry);
        if (routeEntry == 0){
            lastRouteTime = time(NULL);
            return;
        }
        for( int i = 0 ; i < routeEntry ; i++){
            for( int j = 0 ; j < ADDR_LENGTH ; j++){
                data[dataLength] = routeTable[i].destination[j];
                dataLength++;
            }
            data[dataLength] = routeTable[i].distance; //distance
            dataLength++;
            data[dataLength] = routeTable[i].metric;
            dataLength++;
        }
        debug_printf("Sending data: ");
        for(int i = 0 ; i < dataLength ; i++){
            debug_printf("%02x ", data[i]);
        }
        debug_printf("\n");
        uint8_t destination[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
        struct Packet routeMessage = buildPacket(1, destination, 'r', data, dataLength); 
        uint8_t* sending = malloc(sizeof(routeMessage));
        memcpy(sending, &routeMessage, sizeof(routeMessage));
        send_packet(sending, routeMessage.totalLength);
        messageCount++;
        lastRouteTime = time(NULL);
        debug_printf("Sending routes: ");
        for(int i = 0 ; i < routeMessage.totalLength ; i++){
            debug_printf("%02x", sending[i]);
        }
        debug_printf("\n");
    }
}

long lastMessageTime = 0;
void transmitToRandomRoute(){

    if (time(NULL) - lastMessageTime > messageInterval()) {

        if (routeEntry == 0){
            Serial.printf("trying to send but I have no routes ");
            lastMessageTime = time(NULL);
            return;
        }
        int choose = rand()%routeEntry;
        uint8_t destination[ADDR_LENGTH];
        memcpy(destination, &routeTable[choose].destination, sizeof(destination));

        Serial.printf("trying to send a random message from %s to ", macaddr);
        for( int j = 0 ; j < ADDR_LENGTH ; j++){
            Serial.printf("%02x", routeTable[choose].destination[j]);
        }
        Serial.printf(" via ");
        for( int j = 0 ; j < ADDR_LENGTH ; j++){
            Serial.printf("%02x", routeTable[choose].nextHop[j]);
        }
        Serial.printf("\n");

        uint8_t data[240];
        int dataLength = 0;
        for( int i = 0 ; i < ADDR_LENGTH ; i++){
            data[dataLength] = routeTable[choose].nextHop[i];
            dataLength++;
        }
        struct Packet randomMessage = buildPacket(32, destination, 'c', data, dataLength); 
        uint8_t* sending = malloc(sizeof(randomMessage));
        memcpy(sending, &randomMessage, sizeof(randomMessage));
        send_packet(sending, randomMessage.totalLength);
        messageCount++;
        lastMessageTime = time(NULL);
        Serial.printf("Sending a message: ");
        for(int i = 0 ; i < randomMessage.totalLength ; i++){
            Serial.printf("%02x", sending[i]);
        }
        Serial.printf("\r\n");
    }
}

void wifiSetup(){

    //WiFi.macAddress(mac);
    // generate random mac address
    srand(time(NULL) + getpid());
    for (int i = 0; i < ADDR_LENGTH ; i++){
        mac[i] = rand()%256;
    }
    // MAC address comes in backwards on ESP8266
    // reverse mac array
    uint8_t tmp;
    int start = 0;
    int end = ADDR_LENGTH-1;
    while (start < end) 
    { 
        tmp = mac[start];    
        mac[start] = mac[end]; 
        mac[end] = tmp; 
        start++; 
        end--; 
    }    
    sprintf(macaddr, "%02x%02x%02x%02x%02x%02x\0", mac[0], mac[1], mac[2], mac[3], mac[4], mac [5]);
    debug_printf("%s\n", macaddr);
    //strcat(ssid, macaddr);
    //WiFi.hostname(hostName);
    //WiFi.mode(WIFI_AP);
    //WiFi.softAPConfig(local_IP, gateway, netmask);
    //WiFi.softAP(ssid);
}

long startTime;
int chance;
int setup() {

    Serial.printf("initialized\n");
    debug_printf("debuggin enabled\n");

    wifiSetup();

    // random wait at boot
    int wait = rand()%maxRandomDelay();
    Serial.printf("waiting %d s\n", wait);
    nsleep(wait, 0);

    startTime = time(NULL);
    lastHelloTime = time(NULL);
    lastRouteTime = time(NULL);
    _learningTimeout += wait;
        
    chance=rand()%5;
    if(chance == 3){
        Serial.printf("I shall send a random message\n");
    }

    return 0;
}

int state = 0;
int loop() {

    if(!begin_packet()){
        debug_printf("transmit in progress please wait");
    }else{
        if(state == 0){
            Serial.printf("learning... %d\r", time(NULL) - startTime);
            if(DEBUG){
                printNeighborTable();
                printRoutingTable();
            }
            transmitHello();
            if(time(NULL) - startTime > discoveryTimeout()) {
                state++;
            }
        }else if(state == 1){
            Serial.printf("learning... %d\r", time(NULL) - startTime);
            if(DEBUG){
                printNeighborTable();
                printRoutingTable();
            }
            transmitRoutes();
            if(time(NULL) - startTime > learningTimeout()) {
                state++;
                printNeighborTable();
                printRoutingTable();
            }
        }else if(state == 2){
            checkBuffer(); 
            if(chance == 3){
                transmitToRandomRoute();        
            }
        }
    }
    nsleep(0, 1000000*simulationTime(1));
}
