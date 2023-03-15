/****************************************************************************************************************************
  ESPAsync_WiFi.ino
  For ESP8266 / ESP32 boards

  ESPAsync_WiFiManager_Lite (https://github.com/khoih-prog/ESPAsync_WiFiManager_Lite) is a library 
  for the ESP32/ESP8266 boards to enable store Credentials in EEPROM/SPIFFS/LittleFS for easy 
  configuration/reconfiguration and autoconnect/autoreconnect of WiFi and other services without Hardcoding.

  Built by Khoi Hoang https://github.com/khoih-prog/ESPAsync_WiFiManager_Lite
  Licensed under MIT license
  *****************************************************************************************************************************/

#define ESP_ARDUINO_VERSION_MAJOR   2
#define USE_WIFI_WPS  true


#define motor1 D1
#define motor2 D2
#define motor_sersor D6
#define sensor_activate D3
#define sensor_on 0
#define sensor_off 1
#define diode D7

#define button D0

#include "defines.h"
#include "Credentials.h"
#include "dynamicParams.h"
#include <ArduinoOTA.h>
#include "tcp_log.h"
#include "LittleFS.h"
#include "FTPServer.h"

#include "AsyncJson.h"
#include "ArduinoJson.h"

TcpLogger logger(8081);

const char* PARAM_FEED = "feed";
AsyncWebServer server(80);

FTPServer ftpSrv(LittleFS);


ESPAsync_WiFiManager_Lite* ESPAsync_WiFiManager;

void heartBeatPrint()
{
  static int num = 1;

  if (WiFi.status() == WL_CONNECTED)
    {
    if (!ESPAsync_WiFiManager->isConfigMode())
      logger.print("H");        // H means connected to WiFi
    }
  else
  {
    if (ESPAsync_WiFiManager->isConfigMode())
      logger.print("C");        // C means in Config Mode
    else
      logger.print("F");        // F means not connected to WiFi  
  }

  if (num == 80)
  {
    logger.println("");
    num = 1;
  }
  else if (num++ % 10 == 0)
  {
    logger.print(" ");
  }
}

void check_wifi_status()
{
  static unsigned long checkstatus_timeout = 0;

  //KH
#define HEARTBEAT_INTERVAL    10000L
  // Print hearbeat every HEARTBEAT_INTERVAL (20) seconds.
  if ((millis() > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    heartBeatPrint();
    checkstatus_timeout = millis() + HEARTBEAT_INTERVAL;
  }
}

void activate_sensor(){
  logger.println("Sensor active");
  digitalWrite(sensor_activate, sensor_on);
}
void deactivate_sensor(){
  logger.println("Sensor inactive");
  digitalWrite(sensor_activate, sensor_off);
}

bool btn;
void check_buttons()
{
  static unsigned long checkstatus_timeout = 0;

#define BUTTONS_REFRESH_INTERVAL    100L
  if ((millis() > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    btn = !digitalRead(button);
    checkstatus_timeout = millis() + BUTTONS_REFRESH_INTERVAL;
  }
}

int do_feed_counter;
void do_feed(int count)
{
  do_feed_counter = count;
  activate_sensor();
  delay(50);
}

void check_motor_sensor()
{
  static int prv_state = -1;
  static unsigned long checkstatus_timeout = 0;
#define SENSOR_REFRESH_INTERVAL    20L
  if (((millis() > checkstatus_timeout) || (checkstatus_timeout == 0)) && do_feed_counter > 0)
  {
    bool value = digitalRead(motor_sersor);
    if (prv_state == -1) prv_state = value;
    if (prv_state != value){
      digitalWrite(diode, value);
      if (value){
        logger.println("diode=1");
      }else{
        logger.println("diode=0");
      }
      if (do_feed_counter > 0)
      {
        do_feed_counter -= 1;
        if (do_feed_counter == 0) {
          deactivate_sensor();
        }
      }
      prv_state = value;
    }
    checkstatus_timeout = millis() + SENSOR_REFRESH_INTERVAL;
  }
}


void motor_run()
{
  #define MOTOR_REFRESH_INTERVAL    100L
  static unsigned long checkstatus_timeout = 0;
  static int motor_status = 0;
  if ((millis() > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    int new_motor_status = 0;
    if (do_feed_counter > 0){
      new_motor_status = 255;
    } else {
      digitalWrite(diode, 0);
    }
    if (motor_status != new_motor_status){
      analogWrite(motor1, new_motor_status);
      motor_status = new_motor_status;
    }
    checkstatus_timeout = millis() + MOTOR_REFRESH_INTERVAL;
  }
}


#if USING_CUSTOMS_STYLE
const char NewCustomsStyle[] /*PROGMEM*/ = "<style>div,input{padding:5px;font-size:1em;}input{width:95%;}body{text-align: center;}\
button{background-color:blue;color:white;line-height:2.4rem;font-size:1.2rem;width:100%;}fieldset{border-radius:0.3rem;margin:0px;}</style>";
#endif


void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}


void setup()
{

  analogWriteResolution(8);

  pinMode(motor1, OUTPUT);
  digitalWrite(motor1, 0);
  pinMode(motor2, OUTPUT);
  digitalWrite(motor2, 0);

  pinMode(sensor_activate, OUTPUT);
  digitalWrite(sensor_activate, sensor_off);

  pinMode(diode, OUTPUT);

  pinMode(button, INPUT);

  // Debug console
  Serial.begin(115200);
  while (!Serial);

  delay(200);

  Serial.print(F("\nStarting ESPAsync_WiFi using ")); Serial.print(FS_Name);
  Serial.print(F(" on ")); Serial.println(ARDUINO_BOARD);
  Serial.println(ESP_ASYNC_WIFI_MANAGER_LITE_VERSION);

#if USING_MRD  
  Serial.println(ESP_MULTI_RESET_DETECTOR_VERSION);
#else
  Serial.println(ESP_DOUBLE_RESET_DETECTOR_VERSION);
#endif

  ESPAsync_WiFiManager = new ESPAsync_WiFiManager_Lite();

  // Optional to change default AP IP(192.168.4.1) and channel(10)
  //ESPAsync_WiFiManager->setConfigPortalIP(IPAddress(192, 168, 120, 1));
  ESPAsync_WiFiManager->setConfigPortalChannel(0);

#if USING_CUSTOMS_STYLE
  ESPAsync_WiFiManager->setCustomsStyle(NewCustomsStyle);
#endif

#if USING_CUSTOMS_HEAD_ELEMENT
  ESPAsync_WiFiManager->setCustomsHeadElement("<style>html{filter: invert(10%);}</style>");
#endif

#if USING_CORS_FEATURE  
  ESPAsync_WiFiManager->setCORSHeader("Your Access-Control-Allow-Origin");
#endif

  // Set customized DHCP HostName
  ESPAsync_WiFiManager->begin(HOST_NAME);
  //Or use default Hostname "NRF52-WIFI-XXXXXX"
  //ESPAsync_WiFiManager->begin();
  ArduinoOTA.begin();

  logger.begin();

  LittleFS.begin();

  server.serveStatic("/", LittleFS, "/static_www/");

  server.on("/api", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String message;
    if (request->hasParam(PARAM_FEED)) {
      //FEED value shuld be int
      int value = atoi(request->getParam(PARAM_FEED)->value().c_str());
      do_feed(value);
      message = "GETFEED: " + String(value);
    } else {
      message = "No function";
    }
    request->send(200, "text/plain", message);
  });

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/post-message", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<200> data;
    if (json.is<JsonArray>())
    {
      data = json.as<JsonArray>();
    }
    else if (json.is<JsonObject>())
    {
      data = json.as<JsonObject>();
    }
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
    Serial.println(response);
  });
  server.addHandler(handler);


  server.onNotFound(notFound);

  server.begin();

  ftpSrv.begin("admin", "admin");

}

#if USE_DYNAMIC_PARAMETERS
void displayCredentials()
{
  Serial.println(F("\nYour stored Credentials :"));

  for (uint16_t i = 0; i < NUM_MENU_ITEMS; i++)
  {
    Serial.print(myMenuItems[i].displayName);
    Serial.print(F(" = "));
    Serial.println(myMenuItems[i].pdata);
  }
}

void displayCredentialsInLoop()
{
  static bool displayedCredentials = false;

  if (!displayedCredentials)
  {
    for (int i = 0; i < NUM_MENU_ITEMS; i++)
    {
      if (!strlen(myMenuItems[i].pdata))
      {
        break;
      }

      if ( i == (NUM_MENU_ITEMS - 1) )
      {
        displayedCredentials = true;
        displayCredentials();
      }
    }
  }
}

#endif

WiFiClient client;

void loop()
{
  ESPAsync_WiFiManager->run();
  check_wifi_status();
  check_buttons();
  check_motor_sensor();
  motor_run();
  logger.loop();

  ArduinoOTA.handle();
 
  ftpSrv.handleFTP();

#if USE_DYNAMIC_PARAMETERS
  displayCredentialsInLoop();
#endif
}
