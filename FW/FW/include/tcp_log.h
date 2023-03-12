#include "ESP8266WiFi.h"
//#include <stdint.h>

class TcpLogger : public WiFiServer {
  WiFiClient _client;
  public:
    TcpLogger(uint16_t port) : WiFiServer(port){}
    void loop();
    void print(const char *str);
    void println(const char *str);
};