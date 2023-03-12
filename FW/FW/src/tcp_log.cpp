#include "ESP8266WiFi.h"
#include "tcp_log.h"

void TcpLogger::loop()
{
    WiFiClient new_client = available();
    
    if (new_client) {
        Serial.println("TCP console connected");
        if (_client) {
            _client.stop();
        }
        _client = new_client;
    }
}

void TcpLogger::print(const char *str){
    Serial.print(str);
    if(_client.connected()){
        _client.print(str);
    }
}

void TcpLogger::println(const char *str){
    Serial.println(str);
    if(_client.connected()){
        _client.println(str);
    }
}
