#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>


const char THING_ID[] = "dca8fd51-f122-4bff-b4b3-82351901d8e9";

const char SSID[]     = SECRET_SSID;    // Network SSID (name)
const char PASS[]     = SECRET_PASS;    // Network password (use for WPA, or use as key for WEP)

void onPlanterCommandChange();
void onMoistureTriggerChange();
void onPumpSwitchChange();

String planterCommand;
String planterStatus;
CloudPercentage moistureSensor;
CloudPercentage moistureTrigger;
bool pumpSwitch;

void initProperties(){

  ArduinoCloud.setThingId(THING_ID);
  ArduinoCloud.addProperty(planterCommand, READWRITE, ON_CHANGE, onPlanterCommandChange);
  ArduinoCloud.addProperty(planterStatus, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(moistureSensor, READ, ON_CHANGE, NULL, 1);
  ArduinoCloud.addProperty(moistureTrigger, READWRITE, ON_CHANGE, onMoistureTriggerChange, 1);
  ArduinoCloud.addProperty(pumpSwitch, READWRITE, ON_CHANGE, onPumpSwitchChange);

}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
