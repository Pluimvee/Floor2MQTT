#ifndef HOME_ASSIST_FLOORHEATING
#define HOME_ASSIST_FLOORHEATING

#include <HADevice.h>
#include <device-types\HASensorNumber.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define FLOOR_SENSOR_COUNT 5    // its actually 4 but well..... give it some slack

////////////////////////////////////////////////////////////////////////////////////////////
//
class HATempSensor : public HASensorNumber
{
private:
  byte address[8];
public:
  HATempSensor(const char*id, const NumberPrecision p) : HASensorNumber(id, p) {};
  bool begin(DallasTemperature *interface, int idx);
  bool loop(DallasTemperature *interface);
};

////////////////////////////////////////////////////////////////////////////////////////////
class HAFloorMonitor : public HADevice 
{
private:
private:
  OneWire            _wire;
  DallasTemperature  _Tsensors;
public:
  HAFloorMonitor(int wire_pin);

  // boiler
  HATempSensor  to_floor;     // in
  HATempSensor  from_floor;   // return
  HATempSensor  from_boiler;  // received
  HATempSensor  to_boiler;    // return
  
  bool begin(const byte mac[6], HAMqtt *mqqt);
  bool loop();                                                    // read DS1820 sensors

  String logmsg; 
};

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

#endif