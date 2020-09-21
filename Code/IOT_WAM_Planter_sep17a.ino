#include "arduino_secrets.h"
#include <arduino-timer.h>
#include "thingProperties.h"
#include "pump.hpp"

/* IoT Cloud Properties
  String planterCommand;
  String planterStatus;
  float moistureSensor;
  bool pumpSwitch;
*/

// Device constants
#define PUMP_CONTROLLER_PIN 1
#define LED_PIN 2
#define SENSOR_PIN A1
#define MOISTURE_LOW_VOLT 520 // Voltage when sensor is dry
#define MOISTURE_HI_VOLT 1023 // ... when sensor is saturated

// ********** CUSTOMIZATION VALUES *********
#define INITIAL_MOISTURE_TRIGGER 50       // Set initial trigger v. high
#define PUMP_RESERVOIR_CAPACITY_MS 90000  // Reservoir runs dry after 90 seconds
#define PUMP_PULSE_TIME_MS 1500           // Pulse for 1.5 sec
#define SENSOR_CYCLE_TIME_MS 1000         // Check sensors every second
#define POST_TRIGGER_PAUSE_SEC 3000       // Pause 5 min after last pump stop 
                                          // before auto trigger (let moisture settle)

// Global variables ;
auto timer = timer_create_default() ; 
unsigned long timerDelay = SENSOR_CYCLE_TIME_MS ; 
unsigned long timerCounter = 0 ; 
Pump pump(PUMP_CONTROLLER_PIN, LED_PIN, PUMP_RESERVOIR_CAPACITY_MS, PUMP_PULSE_TIME_MS) ;

void setup() {
  Serial.begin(9600);
  delay(500); 

  // Initialize cloud variables and properties
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
  
  // Set up the control logics
  analogReference(AR_EXTERNAL) ;
  pinMode(SENSOR_PIN, INPUT) ;
  timerCounter = millis() ; 
  pump.setup() ;
  pumpSwitch = false ;
  planterStatus = String("EOM") ;
  moistureTrigger = INITIAL_MOISTURE_TRIGGER ;
}

void loop() {
  ArduinoCloud.update();
  
  // Run device logic every second
  if (timerCounter - millis() > timerDelay) {
    timerCounter = millis() ;
     
    checkMoistureTrigger() ;
    updateMoistureReading() ;
    checkPump() ;

  }
}

// ---------------------------------------
// IoT Variable Triggered Routines
// ---------------------------------------
void onPlanterCommandChange() {
  if (planterCommand == "check moisture")
    planterStatus = "Moisture level is " + String((int)moistureSensor) + String("%")  ;
  else if (planterCommand == "check pump") 
    planterStatus = "Pump status is " + String(pumpSwitch) ;
  else if (planterCommand = "get status") {
    
    String pumpRunStatus ;
    if (pump.isTimeoutFlagSet())
      pumpRunStatus = "timeout" ;
    else if (pump.getTimeSinceLastStopSec() < POST_TRIGGER_PAUSE_SEC)
      pumpRunStatus = "paused" ;
    else
      pumpRunStatus = "normal" ;
      
    planterStatus = 
      String("Moisture status: \n") +
      String("- Level:\t") + String((int)moistureSensor) + String("% \n") +
      String("- Limit:\t") + String((int)moistureTrigger) + String("% \n") +
      String("Pump data: \n") +
      String("- # cycles:\t") + String(pump.getRunCount()) + String("\n") +
      String("- Runtime:\t") + String(pump.getRunTimeSec()) + String(" sec\n") +
      String("- Pump run status:\t") + pumpRunStatus + String(" \n") ;
  }
  else 
      planterStatus = "Unknown Command" ;
}

void onPumpSwitchChange() {
  
  // Toggle on
  if (pumpSwitch) {
    if (pump.isTimeoutFlagSet()) {
      planterStatus = String("CMD: ") + String("Pump On... but pump timeout.") ;
      pumpSwitch = false ;
    }
    else {
      pump.pumpOn() ;
      planterStatus = String("CMD: ") + String("Pump On") ;
    }
  }
  
  // Toggle off 
  else {
    pump.pumpOff() ;
    planterStatus = String("CMD: ") + String("Pump Off") ;
  }
}

void onMoistureTriggerChange() {
    planterStatus = String("CMD: ") + String("Moisture Trigger updated to ") + String((int)moistureTrigger) + String("%") ;
}


// ---------------------------------------
// Timer-triggered loop routines
// ---------------------------------------
// Once a Second
void updateMoistureReading() {
  int soil_reading = analogRead(SENSOR_PIN) ;
  moistureSensor = map(soil_reading, MOISTURE_LOW_VOLT, MOISTURE_HI_VOLT, 100, 0) ;
}

// Once every 2 Seconds
void checkMoistureTrigger() {
  
  if (! pump.isTimeoutFlagSet() &&
      pump.getTimeSinceLastStopSec() > POST_TRIGGER_PAUSE_SEC  &&
      moistureSensor < moistureTrigger) {
    pump.pumpPulse() ;
    planterStatus = String("Msg: ") + String("Pump triggered by low moisture: ") + String((int)moistureSensor) + String("%")  ;
  }
}

// Once every 1/2 second
void checkPump() {
  pump.pumpCheck() ;
}
