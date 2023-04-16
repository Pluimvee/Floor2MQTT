/*

*/

#include "HAFloorMonitor.h"
#include <HAMqtt.h>
#include <String.h>
#include <DatedVersion.h>
DATED_VERSION(0, 9)
#define DEVICE_NAME  "Floorheating"
#define DEVICE_MODEL "Floorheating Logger esp8266"

////////////////////////////////////////////////////////////////////////////////////////////
#define LOG(s)    Serial.print(s)
//#define LOG(s)    

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool HATempSensor::begin(DallasTemperature *interface, int idx) {
  return interface->getAddress(address, idx);
}

bool HATempSensor::loop(DallasTemperature *interface) {
  float temp = interface->getTempC(address);
//  if (temp < -5 || temp > 100)
//    return false;
 
  return setValue(temp);
}

////////////////////////////////////////////////////////////////////////////////////////////
#define CONSTRUCT_P0(var)       var(#var, HABaseDeviceType::PrecisionP0)
#define CONSTRUCT_P2(var)       var(#var, HABaseDeviceType::PrecisionP2)

#define CONFIGURE_BASE(var, name, class, icon)  var.setName(name); var.setDeviceClass(class); var.setIcon("mdi:" icon)
#define CONFIGURE(var, name, class, icon, unit) CONFIGURE_BASE(var, name, class, icon); var.setUnitOfMeasurement(unit)
#define CONFIGURE_TEMP(var, name, icon)         CONFIGURE(var, name, "temperature", icon, "°C")
#define CONFIGURE_DS(var, icon)                 CONFIGURE_TEMP(var, #var, icon)

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
HAFloorMonitor::HAFloorMonitor(int wire_pin)
: CONSTRUCT_P2(to_floor), CONSTRUCT_P2(from_floor), CONSTRUCT_P2(to_boiler), CONSTRUCT_P2(from_boiler), 
  _wire(wire_pin), _Tsensors(&_wire)
{
  CONFIGURE_DS(to_floor,    "thermometer");
  CONFIGURE_DS(from_floor,  "thermometer");
  CONFIGURE_DS(to_boiler,   "thermometer");
  CONFIGURE_DS(from_boiler, "thermometer");
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool HAFloorMonitor::begin(const byte mac[6], HAMqtt *mqtt) 
{
  logmsg.clear();
  setUniqueId(mac, 6);
  setManufacturer("InnoVeer");
  setName(DEVICE_NAME);
  setSoftwareVersion(VERSION);
  setModel(DEVICE_MODEL);

  // boiler
  mqtt->addDeviceType(&to_floor);  
  mqtt->addDeviceType(&from_floor);  
  mqtt->addDeviceType(&to_boiler);  
  mqtt->addDeviceType(&from_boiler);  

  _Tsensors.begin();
  if (_Tsensors.getDeviceCount() < 4)
    logmsg = "ERROR: We have not found the 4 DS sensors";

  bool result = true;
  result &= to_floor.begin(&_Tsensors, 0);
  result &= from_floor.begin(&_Tsensors, 1);
  result &= to_boiler.begin(&_Tsensors, 2);
  result &= from_boiler.begin(&_Tsensors, 3);

  if (!result)
    logmsg = "ERROR: One of the sensors did not give its address";

  return result;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool HAFloorMonitor::loop()
{
  logmsg.clear();
  _Tsensors.requestTemperatures();   // send command to sensors to measure

  bool result = true;
  result &= to_floor.loop(&_Tsensors);       // retrieve temp
  result &= from_floor.loop(&_Tsensors);  
  result &= to_boiler.loop(&_Tsensors);  
  result &= from_boiler.loop(&_Tsensors);  

  if (!result)
    logmsg = "ERROR: getting/setting one of the temeratures";
  return result;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

