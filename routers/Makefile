
all: firmware

DRLIB=./libs/disaster-radio/firmware/src
LL2LIB=./libs/LoRaLayer2/src
WSLIB=./libs/Simple-WebSocket-Server
HTTPLIB=./libs/Simple-Web-Server
DEBUGLIB = ./libs/backward-cpp
LOGLIB= ./libs/log.c/src

FLAGS=-lssl -lcrypto -lbfd -ldl -pthread -lboost_filesystem -lboost_system -DBACKWARD_HAS_BFD=1 -DSIM

CPP_FILES=$(LL2LIB)/Layer1_Sim.cpp \
  $(LL2LIB)/LoRaLayer2.cpp \
  $(LL2LIB)/packetBuffer.cpp \
  $(DRLIB)/DisasterMiddleware.cpp \
  $(DRLIB)/server/DisasterRadio.cpp \
  $(DRLIB)/client/WebSocketppClient.cpp \
  $(DRLIB)/client/LoRaClient.cpp \
  $(DRLIB)/client/SocatClient.cpp \
  $(DRLIB)/middleware/Console.cpp \
  $(DRLIB)/utils/utils.cpp \
  $(LOGLIB)/log.c \
  simulator.c \
  main.cpp

firmware: main.cpp
	g++ -g -I$(LL2LIB) -I$(DEBUGLIB) -I$(DRLIB) -I$(WSLIB) -I$(HTTPLIB) -I$(LOGLIB) -o firmware $(CPP_FILES) $(FLAGS)

socatSockets: socatSockets.cpp
	g++ -g -I$(WSLIB) -I$(DEBUGLIB) -o socatSockets socatSockets.cpp $(FLAGS)

clean:
	rm -f firmware 
