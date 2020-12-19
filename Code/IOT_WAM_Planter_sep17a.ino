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
#define MAXLEN_IOT_TEXT_BUFFER 206


// Hardware constants
#define PUMP_CONTROLLER_PIN 1
#define LED_PIN 2
#define SENSOR_PIN A1
#define MOISTURE_LOW_VOLT 480 // Voltage when sensor is saturated
#define MOISTURE_HI_VOLT 1023 // ... when sensor is dry

// ********** CUSTOMIZATION VALUES *********
#define INITIAL_MOISTURE_TRIGGER 50       // Set initial trigger v. high
#define POST_TRIGGER_PAUSE_SEC 3000       // Pause 5 min after last pump stop 
// before auto trigger (let moisture settle)

// Global variables ;
unsigned long timerDelay = SENSOR_CYCLE_TIME_MS ;
unsigned long timerCounter = 0 ;
struct Planter::pinConfig pins = { PUMP_CONTROLLER_PIN, LED_PIN, SENSOR_PIN } ;
Planter planter(pins, MOISTURE_HI_VOLT, MOISTURE_LOW_VOLT) ;

// Local functions
void blink_delay(int reps=1, unsigned long int delay_ms = 500, int pin = LED_BUILTIN) ;


void setup() {


  // Initialize cloud variables and properties
  Serial.begin(9600);
  Serial.println("------- setup starting ------------") ;
  blink_delay(1) ;
  
  // Initialize cloud variables and properties
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
  Serial.println("cloud configured...") ;
  blink_delay(2) ;

  // Set up the control logics  planter.setup() ;
  pumpSwitch = false ;
  planterStatus = String("EOM") ;
  moistureTrigger = INITIAL_MOISTURE_TRIGGER ;
  planter.setup() ;
  Serial.println("planter configured...") ;
  Serial.println("------- setup complete ------------") ;
  blink_delay(3) ;
}

void loop() {
  ArduinoCloud.update();

  // Run device logic every second
  if (planter.checkStatus()) {
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
    planterCommand = "Moisture level is " + String((int)moistureSensor) + String("%\n")  ;

  else if (command == "pump")
    planterCommand = "Pump status is " + String(pumpSwitch) ;

  else if (command == "status") {
    String pumpRunStatus ;
    if (planter.isTimeoutFlagSet())
      pumpRunStatus = "timeout" ;
    else if (planter.getTimeSinceLastStopSec() < POST_TRIGGER_PAUSE_SEC)
      pumpRunStatus += "paused" ;
    else
      pumpRunStatus += "normal" ;
    planterCommand =
      String("Moisture status: \n") +
      String("- Level:\t") + String((int)moistureSensor) + String("% \n") +
      String("- Limit:\t") + String((int)moistureTrigger) + String("% \n") +
      String("Pump data: \n") +
      String("- # cycles:\t") + String(planter.getRunCount()) + String("\n") +
      String("- Runtime:\t") + String(planter.getRunTimeSec()) + String(" sec\n") +
      String("- Pump run status:\t") + pumpRunStatus + String(" \n") ;
  }

  else if (command == "reset") {
    planter.reset() ;
    planterCommand = String("Pump controls been reset.") ;
  }
  
  else if (command == "history") {
    planterCommand = String("Planter recent history:\n") ;
    planterCommand += planter.getDailyHistory() ;
    planterCommand = planterCommand.substring(0, MAXLEN_IOT_TEXT_BUFFER) ;
  }
  
  else if (command.startsWith("set pulse")) {
    unsigned long newPulseTime = command.substring(10).toInt() ;
    if (planter.setPumpPulseTime_MS(newPulseTime*1000))
      planterCommand = String("Pulse time set to ") + String(newPulseTime) ;
    else 
      planterCommand = String("Invalid pulse time") ;
  }
  
  else if (command.startsWith("set reservoir")) {
    unsigned long newReservoirTime = command.substring(14).toInt() ;
    if (planter.setResevervoirCapacity_MS(newReservoirTime*1000))
      planterCommand = String("Reservoir time set to ") + String(newReservoirTime) ;
    else 
      planterCommand = String("Invalid reservoir time") ;
  }

  else if (command == "get params") {
    planterCommand = String("Parameters:--------\n") ;
    planterCommand += planter.getParameters() ;
  }

  else if (command == "help") {
    planterCommand = String("Available commands:\n") +
                    String("moisture, \n") +
                    String("pump \n") +
                    String("reset \n") +
                    String("status \n") +
                    String("history \n") +
                    String("set pulse X \n") +
                    String("set reservoir X\n") ;
  }
  else planterCommand = "Unknown Command\n" ;
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
  moistureSensor = planter.getMoisturePercent() ;
}

void checkMoistureTrigger() {

  if (! planter.isTimeoutFlagSet() &&
      planter.getTimeSinceLastStopSec() > POST_TRIGGER_PAUSE_SEC  &&
      moistureSensor < moistureTrigger) {
    planter.pumpPulse() ;
    planterStatus = String("AUTO: ") + String("Pump triggered by low moisture: ") + String((int)moistureSensor) + String("%")  ;
  }
}

// ---------------------------------------
// Blink routine
// ---------------------------------------
void blink_delay(int reps /* = 1*/, unsigned long int delay_ms /*= 500*/, int pin /*= LED_BUILTIN*/) {
    
    for (int i=1; i<=reps; i++) {
      digitalWrite(pin, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(delay_ms);                       // wait for a second
      digitalWrite(pin, LOW);    // turn the LED off by making the voltage LOW
      delay(delay_ms);                       // wait for a second
    }
}
