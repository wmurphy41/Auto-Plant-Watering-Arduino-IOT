// Code generated by Arduino IoT Cloud, DO NOT EDIT.

#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>


const char THING_ID[] = "dca8fd51-f122-4bff-b4b3-82351901d8e9";

const char SSID[]     = SECRET_SSID;    // Network SSID (name)
const char PASS[]     = SECRET_PASS;    // Network password (use for WPA, or use as key for WEP)

void onPlanterCommandChange();
void onPumpSwitchChange();
void onMoistureTriggerChange();

String planterCommand;
String planterStatus;
CloudSwitch pumpSwitch;
CloudPercentage moistureSensor;
CloudPercentage moistureTrigger;

void initProperties(){

  ArduinoCloud.setThingId(THING_ID);
  ArduinoCloud.addProperty(planterCommand, READWRITE, ON_CHANGE, onPlanterCommandChange);
  ArduinoCloud.addProperty(planterStatus, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(pumpSwitch, READWRITE, ON_CHANGE, onPumpSwitchChange);
  ArduinoCloud.addProperty(moistureSensor, READ, ON_CHANGE, NULL, 2);
  ArduinoCloud.addProperty(moistureTrigger, READWRITE, ON_CHANGE, onMoistureTriggerChange, 1);

}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
