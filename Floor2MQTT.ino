#include "HAFloorMonitor.h"
#include <ESP8266WiFi.h>
#include <HAMqtt.h>
#include <LED.h>
#include <Clock.h>
#include <Timer.h>
#include <ArduinoOTA.h>
#include "secrets.h"
#include <DatedVersion.h>
DATED_VERSION(0, 2)
#define LOG_REMOTE
#define LOG_LEVEL 2
#include <Logging.h>

////////////////////////////////////////////////////////////////////////////////////////////
// Configuration
const char* sta_ssid      = STA_SSID;
const char* sta_pswd      = STA_PASS;

const char* mqtt_server   = "192.168.2.170";   // test.mosquitto.org"; //"broker.hivemq.com"; //6fee8b3a98cd45019cc100ef11bf505d.s2.eu.hivemq.cloud";
int         mqtt_port     = 1883;             // 8883;
const char* mqtt_user     = MQTT_USER;
const char *mqtt_passwd   = MQTT_PASS;

////////////////////////////////////////////////////////////////////////////////////////////
// Global instances
WiFiClient        socket;
HAFloorMonitor    floor_mon(D3);                     // THe Floorheat monitor with all of its sensors
HAMqtt            mqtt(socket, floor_mon, FLOOR_SENSOR_COUNT);  // Home Assistant MTTQ
Clock             rtc;                            // A real (software) time clock
LED               led;                            // 

////////////////////////////////////////////////////////////////////////////////////////////
// Send log remotely to MQTT
void LOG_CALLBACK(char *msg) { 
  LOG_REMOVE_NEWLINE(msg);
  mqtt.publish("FloorHeating/log", msg, true); 
}

////////////////////////////////////////////////////////////////////////////////////////////
// Connect to the STA network
void wifi_connect() 
{ 
  if (((WiFi.getMode() & WIFI_STA) == WIFI_STA) && WiFi.isConnected())
    return;

  DEBUG("Wifi connecting to %s.", sta_ssid);
  WiFi.begin(sta_ssid, sta_pswd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG(".");
  }
  DEBUG("\n");
  INFO("WiFi connected with IP address: %s\n", WiFi.localIP().toString().c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////
// MQTT Connect
void mqtt_connect() {
  INFO("Floorheating Monitor v%s saying hello\n", VERSION);
}

///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
void sync_clock()
{
  rtc.ntp_sync();
  INFO("Clock synchronized to %s\n", rtc.now().timestamp().c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
void setup() 
{
  Serial.begin(115200);
  INFO("\n\nFloor heating monitor Version %s\n", VERSION);
  wifi_connect();

  // start MQTT to enable remote logging asap
  INFO("Connecting to MQTT server %s\n", mqtt_server);
  uint8_t mac[6];
  WiFi.macAddress(mac);
  floor_mon.begin(mac, &mqtt);              // 5) make sure the device gets a unique ID (based on mac address)
  mqtt.onConnected(mqtt_connect);           // register function called when newly connected
  mqtt.begin(mqtt_server, mqtt_port, mqtt_user, mqtt_passwd);  // 

  sync_clock(); 

  if (!floor_mon.logmsg.isEmpty())
    ERROR(floor_mon.logmsg.c_str());

  INFO("Initialize OTA\n");
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("Floorheating-Logger");
  ArduinoOTA.setPassword(OTA_PASS);

  ArduinoOTA.onStart([]() {
    INFO("[%s] - Starting remote software update",
          rtc.now().timestamp(DateTime::TIMESTAMP_TIME).c_str());
  });
  ArduinoOTA.onEnd([]() {
    INFO("[%s] - Remote software update finished",
          rtc.now().timestamp(DateTime::TIMESTAMP_TIME).c_str());
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  });
  ArduinoOTA.onError([](ota_error_t error) {
    ERROR("Error remote software update");
  });
  ArduinoOTA.begin();
  INFO("Setup complete\n\n");
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
Timer update_interval;

void loop() 
{
  // ensure we are still connected (STA-mode)
  wifi_connect();
  // handle any OTA requests
  ArduinoOTA.handle();
  // handle MQTT
  mqtt.loop();
  // whats the time
  DateTime now = rtc.now();

  if (!update_interval.passed()) 
    return;

  led.blink();

  if (!floor_mon.loop())
    ERROR("[%s] - %s\n", 
          rtc.now().timestamp(DateTime::TIMESTAMP_TIME).c_str(), 
          floor_mon.logmsg.c_str());

  // if temp from boiler is ~2 degrees above temp of floor there is heating demand
  float temp_diff = floor_mon.from_boiler.getCurrentValue().toFloat() - floor_mon.from_floor.getCurrentValue().toFloat();
  // this temp offset may not work in summertime, with outside temp above 30
  if (temp_diff > 1.5f)
    update_interval.set(1000);
  else
    update_interval.set(10000);

  INFO("[%s] - Reading temperature sensors (Tba-Tfr:%f)\n", 
            now.timestamp(DateTime::TIMESTAMP_TIME).c_str(), temp_diff);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

