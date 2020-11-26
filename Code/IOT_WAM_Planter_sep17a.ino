#include "arduino_secrets.h"
// RingBufCPP - Version: 1.1.0
#include <stdint.h>
#include "thingProperties.h"
#include "Planter.hpp"

/* IoT Cloud Properties
  String planterStatus;
  float moistureSensor;
  bool pumpSwitch;
*/

// Hardware constants
#define PUMP_CONTROLLER_PIN 1
#define LED_PIN 2
#define SENSOR_PIN A1
#define MOISTURE_LOW_VOLT 480 // Voltage when sensor is saturated
#define MOISTURE_HI_VOLT 1023 // ... when sensor is dry

// ********** CUSTOMIZATION VALUES *********
#define INITIAL_MOISTURE_TRIGGER 50       // Set initial trigger v. high
#define PUMP_RESERVOIR_CAPACITY_MS 90000  // Reservoir runs dry after 90 seconds
#define PUMP_PULSE_TIME_MS 1500           // Pulse for 1.5 sec
#define SENSOR_CYCLE_TIME_MS 1000         // Check sensors every second
#define POST_TRIGGER_PAUSE_SEC 3000       // Pause 5 min after last pump stop 
// before auto trigger (let moisture settle)

// Global variables ;
unsigned long timerDelay = SENSOR_CYCLE_TIME_MS ;
unsigned long timerCounter = 0 ;
struct Planter::pinConfig pins = {
  PUMP_CONTROLLER_PIN ,
  LED_PIN ,
  SENSOR_PIN
} ;
Planter planter(pins, PUMP_RESERVOIR_CAPACITY_MS, PUMP_PULSE_TIME_MS, MOISTURE_HI_VOLT, MOISTURE_LOW_VOLT) ;


void setup() {
  Serial.begin(9600);
  delay(500);

  // Initialize cloud variables and properties
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  // Set up the control logics
  timerCounter = millis() ;
  planter.setup() ;
  pumpSwitch = false ;
  planterStatus = String("EOM") ;
  moistureTrigger = INITIAL_MOISTURE_TRIGGER ;
}

void loop() {
  ArduinoCloud.update();

  // Run device logic every second
  if (timerCounter - millis() > timerDelay) {
    timerCounter = millis() ;

    checkPump() ;               // Check pump for timeout
    readMoisture() ;            // Read moisture/update cloud
    checkMoistureTrigger() ;    // Trigger pump if dry
  }
}

// ---------------------------------------
// IoT Variable Triggered Routines
// ---------------------------------------
void onPlanterCommandChange() {
  
  String command = planterCommand ;
  command.toLowerCase() ; 
  
  if (command == "moisture")
    planterStatus = "Moisture level is " + String((int)moistureSensor) + String("%\n")  ;

  else if (command == "pump")
    planterStatus = "Pump status is " + String(pumpSwitch) ;

  else if (command == "status") {
    String pumpRunStatus ;
    if (planter.isTimeoutFlagSet())
      pumpRunStatus = "timeout" ;
    else if (planter.getTimeSinceLastStopSec() < POST_TRIGGER_PAUSE_SEC)
      pumpRunStatus += "paused" ;
    else
      pumpRunStatus += "normal" ;
    planterStatus =
      String("Moisture status: \n") +
      String("- Level:\t") + String((int)moistureSensor) + String("% \n") +
      String("- Limit:\t") + String((int)moistureTrigger) + String("% \n") +
      String("Pump data: \n") +
      String("- # cycles:\t") + String(planter.getRunCount()) + String("\n") +
      String("- Runtime:\t") + String(planter.getRunTimeSec()) + String(" sec\n") +
      String("- Pump run status:\t") + pumpRunStatus + String(" \n") ;
  }

  else if (command == "history") {
    planterStatus = String("Moisture history------- \n") ;
    planterStatus += planter.getRecentHistory() ;
    planterStatus += planter.getDailyHistory() ;
  }

  else if (planterStatus.startsWith("Unknown Command"))
    planterStatus += " ";
  else planterStatus = "Unknown Command\n" ;
}


void onPumpSwitchChange() {

  // Toggle on
  if (pumpSwitch) {
    if (planter.isTimeoutFlagSet()) {
      planterStatus = String("CMD: ") + String("Pump On... but pump timeout.") ;
      pumpSwitch = false ;
    }
    else {
      planter.pumpOn() ;
      planterStatus = String("CMD: ") + String("Pump On") ;
    }
  }

  // Toggle off
  else {
    planter.pumpOff() ;
    planterStatus = String("CMD: ") + String("Pump Off") ;
  }
}

void onMoistureTriggerChange() {
  planterStatus = String("CMD: ") + String("Moisture Trigger updated to ") + String((int)moistureTrigger) + String("%") ;
}


// ---------------------------------------
// Timer-triggered loop routines
// ---------------------------------------
void readMoisture() {
  moistureSensor = planter.readMoisture() ;
}

void checkMoistureTrigger() {

  if (! planter.isTimeoutFlagSet() &&
      planter.getTimeSinceLastStopSec() > POST_TRIGGER_PAUSE_SEC  &&
      moistureSensor < moistureTrigger) {
    planter.pumpPulse() ;
    planterStatus = String("Msg: ") + String("Pump triggered by low moisture: ") + String((int)moistureSensor) + String("%")  ;
  }
}

void checkPump() {
  planter.pumpCheck() ;
}