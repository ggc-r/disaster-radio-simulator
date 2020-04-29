#include "Console.h"

/** Comment out to stop debug output */
// #define DEBUG_OUT

#include <Layer1.h>
#include <LoRaLayer2.h>
//#include "settings/settings.h"
//#include "utils/utils.h"

#include <vector>

Console::Console(){};

void Console::print(const char* message)
{
  struct Datagram response;
  memcpy(response.destination, LL2.loopbackAddr(), ADDR_LENGTH);
  response.type = 'i';
  int msgLen = sprintf((char *)response.message, message);
  client->receive(response, msgLen + DATAGRAM_HEADER);
}

void Console::setup()
{
  /*
  DisasterMiddleware::setup();
  if (history)
  {
    history->replay(this);
  }
  */
  sessionConnected = 0;
}

void Console::processLine(char *message, size_t len)
{
  struct Datagram response;
  memset(response.message, 0, DATAGRAM_MESSAGE);
  int msgLen;

  #ifdef DEBUG_OUT
  Serial.printf("Console::processLine help result %s\r\n", message);
  #endif
  // message might not be NULL ended
  char msgBuff[len + 2] = {0};
  memcpy(msgBuff, message, len);

  if (msgBuff[0] == '/')
  {
    std::vector<char *> args;

    char *p;

    p = strtok(msgBuff, " ");
    while (p)
    {
      args.push_back(p);
      p = strtok(NULL, " ");
    }

    p = args[0];

    if (strncmp(&args[0][1], "help", 4) == 0)
    {
      print("Commands: /help /join /nick /raw /lora /set /restart\r\n");
      #ifdef DEBUG_OUT
      Serial.printf("Console::processLine help result %s\r\n", (char *)response.message);
      #endif
    }
    else if (strncmp(&args[0][1], "raw", 3) == 0)
    {
      #ifdef DEBUG_OUT
      Serial.printf("Console::processLine switching to RAW\r\n");
      #endif
      disconnect(client);
      server->disconnect(this);
      server->connect(client);
    }
    /*
    else if ((strncmp(&args[0][1], "set", 3) == 0) && (args.size() > 1))
    {
      #ifdef DEBUG_OUT
      Serial.printf("Switching UI to %s\n", useBLE ? "WiFi" : "BLE");
      #endif
      if (strncmp(&args[1][0], "ui", 2) == 0){
        saveUI(!useBLE);
        delay(500);
        ESP.restart();
      }
    }
    else if ((strncmp(&args[0][1], "set", 3) == 0) && (args.size() == 1)){
      print("No setting provided, type '/set SETTING'\r\n");
      print("SETTINGs include,\r\n");
      print("'ui' - toggles between WiFi and BLE user interface\r\n");
    }

    else if (((strncmp(&args[0][1], "join", 4) == 0) || (strncmp(&args[0][1], "nick", 4) == 0)) && (args.size() > 1))
    {
      strtok(args[1], "\r"); // remove CR-LF from username

      if (username.length() > 0)
      {
        msgLen = sprintf((char *)response.message, "00c|~ %s is now known as %s\r\n", username.c_str(), args[1]);
      }
      else
      {
        msgLen = sprintf((char *)response.message, "00c|~ %s joined the channel\r\n", args[1]);
      }

      memcpy(response.destination, LL2.broadcastAddr(), ADDR_LENGTH);
      response.type = 'c';
      server->transmit(this, response, msgLen + DATAGRAM_HEADER);

      memcpy(response.message, &response.message[4], msgLen - 4);
      response.message[msgLen - 4] = '\n';
      client->receive(response, msgLen - 4 + DATAGRAM_HEADER);

      username = String(args[1]);
      saveUsername(username);

      #ifdef DEBUG_OUT
      Serial.printf("Console::processLine join/nick result %s\r\n", (char *)response.message);
      Serial.printf("Console::processLine new username is %s\r\n", username.c_str());
      #endif
    }
    else if (((strncmp(&args[0][1], "join", 4) == 0) || (strncmp(&args[0][1], "nick", 4) == 0)) && (args.size() == 1)){
      print("No NICKNAME provided, type '/join NICKNAME' to join the chat\r\n");
    }
    else if ((strncmp(&args[0][1], "restart", 7) == 0))
    {
      #ifdef DEBUG_OUT
      Serial.printf("Console::processLine restarting\r\n");
      delay(500);
      #endif
      ESP.restart();
    }
    */
    else if ((strncmp(&args[0][1], "lora", 4) == 0))
    {
      print("Local address: ");
      char str[ADDR_LENGTH*2 + 1] = {'\0'};
      //hexcpy(str, LL2.localAddress(), ADDR_LENGTH);
      //print(str);
      print("\r\n");
      char str2[256] = {'\0'}; //TODO: need to check size of routing table to allocate correct amount of memory
      LL2.getRoutingTable(str2);
      print(str2);
    }
    else
    {
      msgLen = sprintf((char *)response.message, "Unknown command '%s'\r\n", msgBuff);
      client->receive(response, msgLen + DATAGRAM_HEADER);
    }
  }
  /*
  else if (username.length() > 0)
  {
    msgLen = sprintf((char *)response.message, "00c|<%s>%s", username.c_str(), msgBuff);
    memcpy(response.destination, LL2.broadcastAddr(), ADDR_LENGTH);
    response.type = 'c';
    server->transmit(this, response, msgLen + DATAGRAM_HEADER);
    memcpy(response.message, &response.message[4], msgLen - 4);
    response.message[msgLen - 4] = '\n';
    #ifdef DEBUG_OUT
    Serial.printf("Console message =>%s<\r\n", &response.message[4]);
    #endif
  }
  */
  else
  {
    msgLen = sprintf((char *)response.message, "00c|%s", msgBuff);
    memcpy(response.destination, LL2.broadcastAddr(), ADDR_LENGTH);
    response.type = 'c';
    server->transmit(this, response, msgLen + DATAGRAM_HEADER);
    memcpy(response.message, &response.message[4], msgLen - 4);
    response.message[msgLen - 4] = '\n';
    #ifdef DEBUG_OUT
    Serial.printf("Console message =>%s<\r\n", &response.message[4]);
    #endif
  }
}

void Console::printBanner()
{
  print("     ___              __                            ___    \r\n");
  print(" ___/ (_)__ ___ ____ / /____ ____      _______ ____/ (_)__ \r\n");
  print("/ _  / (_-</ _ `(_-</ __/ -_) __/ _   / __/ _ `/ _  / / _ \\\r\n");
  print("\\_,_/_/___/\\_,_/___/\\__/\\__/_/   (_) /_/  \\_,_/\\_,_/_/\\___/\r\n");
  print("v1.0.0-rc.1\r\n");

  /*
  if(Layer1.loraInitialized()){
    print("LoRa transceiver connected\r\n");
  }else{
    print("WARNING: LoRa transceiver not found!\r\n");
  }
  */
  print("Local address of your node is ");
  char str[ADDR_LENGTH*2 + 1] = {'\0'};
  //hexcpy(str, LL2.localAddress(), ADDR_LENGTH);
  //print(str);
  print("\r\n");
  print("Type '/join NICKNAME' to join the chat, or '/help' for more commands.\r\n");
}

void Console::printPrompt()
{
  /*
  if (username.length() > 0)
  {
    print("<");
    print(username.c_str());
    print("> ");
  }
  else
  {
  */
    print("< > ");
  //}
}

void Console::transmit(DisasterClient *client, struct Datagram datagram, size_t len)
{
  #ifdef DEBUG_OUT
  Serial.printf("CONSOLE::transmit raw data with len %d type %c\n", len, datagram.type);
  for (int idx = 0; idx < len - DATAGRAM_HEADER; idx++)
  {
    Serial.printf("%02X ", datagram.message[idx]);
  }
  Serial.println("\n" + String((char *)datagram.message));
  #endif

  // TODO: set sessionConnected back to zero on disconnection?
  if(sessionConnected == 0){
    printBanner();
    printPrompt();
    sessionConnected = 1;
  }
  else if(sessionConnected == 1){
    // Console receives one line at a time
    processLine((char *) datagram.message, len - DATAGRAM_HEADER);
    printPrompt();
  }
}

void Console::receive(struct Datagram datagram, size_t len)
{
  int msgSize = len - DATAGRAM_HEADER;

  if ((datagram.message[2] == 'c') && (datagram.message[3] == '|'))
  {
    #ifdef DEBUG_OUT
    Serial.printf("CONSOLE::receive raw data with len %d type %c\n", msgSize, datagram.type);
    for (int idx = 0; idx < msgSize; idx++)
    {
      Serial.printf("%02X ", datagram.message[idx]);
    }
    Serial.println("");
    #endif

    memcpy(datagram.message, &datagram.message[4], msgSize - 4);
    msgSize -= 4;
    datagram.message[msgSize] = '\n';
    msgSize += 1;
    client->receive(datagram, msgSize + DATAGRAM_HEADER);
  }
}
