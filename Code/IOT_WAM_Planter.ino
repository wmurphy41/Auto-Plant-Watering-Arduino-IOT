#include "arduino_secrets.h"
// RingBufCPP - Version: 1.1.0
#include <stdint.h>
#include "thingProperties.h"
#include "Planter.hpp"
#include "hardware.h"

/* IoT Cloud Properties
  String planterStatus;
  float moistureSensor;
  bool pumpSwitch;
*/
#define MAXLEN_IOT_TEXT_BUFFER 206


// ********** CUSTOMIZATION VALUES *********
#define INITIAL_MOISTURE_TRIGGER 50       // Set initial trigger v. high
#define POST_TRIGGER_PAUSE_SEC 3000       // Pause 5 min after last pump stop 
#define POST_TRIGGER_PAUSE_SEC 300        // Pause 5 min after last pump stop 
// before auto trigger (let moisture settle)

// Global variables ;
unsigned long timerDelay = SENSOR_CYCLE_TIME_MS ;
unsigned long pumpPauseTimeSec = POST_TRIGGER_PAUSE_SEC ;
unsigned long timerCounter = 0 ;
struct Planter::pinConfig pins = { PUMP_CONTROLLER_PIN, LED_PIN, SENSOR_PIN } ;
Planter planter(pins, MOISTURE_HI_VOLT, MOISTURE_LOW_VOLT) ;

// Local functions
void blink_toggle(int pin = LED_BUILTIN) ;
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
  blink_delay(1) ;

  // Set up the control logics  planter.setup() ;
  pumpSwitch = false ;
  planterStatus = String("EOM") ;
  moistureTrigger = INITIAL_MOISTURE_TRIGGER ;
  planter.setup() ;
  Serial.println("planter configured...") ;
  Serial.println("------- setup complete ------------") ;
  blink_delay(1) ;
}

void loop() {
  ArduinoCloud.update();

  // Run device logic every second
  if (planter.checkStatus()) {
    blink_toggle() ;
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
    planterStatus = "Moisture level is " + String((int)moisturePercent) + String("%\n")  ;

  else if (command == "pump")
    planterStatus = "Pump status is " + String(pumpSwitch) ;

  else if (command == "status") {
    String pumpRunStatus ;
    if (planter.isTimeoutFlagSet())
      pumpRunStatus = "timeout" ;
    else if (planter.getTimeSinceLastStopSec() < pumpPauseTimeSec)
      pumpRunStatus += "paused" ;
    else
      pumpRunStatus += "normal" ;
    planterStatus =
      String("Moisture status: \n") +
      String("- Level:\t") + String((int)moisturePercent) + String("% \n") +
      String("- Limit:\t") + String((int)moistureTrigger) + String("% \n") +
      String("Pump data: \n") +
      String("- # cycles:\t") + String(planter.getRunCount()) + String("\n") +
      String("- Runtime:\t") + String(planter.getRunTimeSec()) + String(" sec\n") +
      String("- Pump run status:\t") + pumpRunStatus + String(" \n") +
      String("- Pulse time:\t") + planter.getPumpPulseTime_MS()/1000 + String(" \n") +
      String("- Reservoir capaciity time:\t") + planter.getResevervoirCapacity_MS()/1000 + String(" \n") +
      String("- Sensor sat volt:\t") + planter.getSensorSatVolt() + String(" \n") +
      String("- Sensor dry volt:\t") + planter.getSensorDryVolt() + String(" \n") ;
  }

  else if (command == "reset") {
    planter.reset() ;
    planterStatus = String("Pump controls been reset.") ;
  }
  
  else if (command == "history") {
    planterStatus = String("Planter recent history:\n") ;
    planterStatus += planter.getDailyHistory() ;
    planterStatus = planterStatus.substring(0, MAXLEN_IOT_TEXT_BUFFER) ;
  }
  
  else if (command.startsWith("set pulse")) {
    unsigned long newPulseTime = command.substring(10).toInt() ;
    if (planter.setPumpPulseTime_MS(newPulseTime*1000))
      planterStatus = String("Pulse time set to ") + String(newPulseTime) ;
    else 
      planterStatus = String("Invalid pulse time") ;
  }
  
  else if (command.startsWith("set sensor dry volt")) {
    int newSDV = command.substring(20).toInt() ;
    if (planter.setSensorDryVolt(newSDV))
      planterStatus = String("Sensor dry volt set to ") + String(newSDV) ;
    else 
      planterStatus = String("Invalid sensor dry volt value") ;
  }
  
  else if (command.startsWith("set sensor sat volt")) {
    int newSSV = command.substring(20).toInt() ;
    if (planter.setSensorSatVolt(newSSV))
      planterStatus = String("Sensor sat volt set to ") + String(newSSV) ;
    else 
      planterStatus = String("Invalid sensor sat volt value") ;
  }
  
  else if (command.startsWith("set reservoir")) {
    unsigned long newReservoirTime = command.substring(14).toInt() ;
    if (planter.setResevervoirCapacity_MS(newReservoirTime*1000))
      planterStatus = String("Reservoir time set to ") + String(newReservoirTime) ;
    else 
      planterStatus = String("Invalid reservoir time") ;
  }

  else if (command.startsWith("set pause")) {
    unsigned long new_pause_sec = command.substring(9).toInt() ;
    pumpPauseTimeSec = new_pause_sec ;
    String("Pump post-run pause time set to ") + String(pumpPauseTimeSec) ;
  }

  else if (command == "help") {
    planterStatus = String("Available commands:\n") +
                    String("moisture \n") +
                    String("pump \n") +
                    String("reset \n") +
                    String("status \n") +
                    String("history \n") +
                    String("set pulse X \n") +
                    String("set reservoir X\n") +
                    String("set sensor sat volt X\n") +
                    String("set sensor dry volt X\n") +
                    String("set pause X\n") ;
  }
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
  moisturePercent = planter.getMoisturePercent() ;
  moistureLevel = planter.getMoistureLevel() ;
}

void checkMoistureTrigger() {

  if (! planter.isTimeoutFlagSet() &&
      planter.getTimeSinceLastStopSec() > pumpPauseTimeSec  &&
      moisturePercent < moistureTrigger) {
    planter.pumpPulse() ;
    planterStatus = String("AUTO: ") + String("Pump triggered by low moisture: ") + String((int)moisturePercent) + String("%")  ;
  }
}

// ---------------------------------------
// Blink routines
// ---------------------------------------
void blink_toggle(int pin /*= LED_BUILTIN*/) {
    
   digitalWrite(pin, !digitalRead(pin));
}

void blink_delay(int reps /* = 1*/, unsigned long int delay_ms /*= 500*/, int pin /*= LED_BUILTIN*/) {
    
    for (int i=1; i<=reps; i++) {
      digitalWrite(pin, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(delay_ms);                       // wait for a second
      digitalWrite(pin, LOW);    // turn the LED off by making the voltage LOW
      delay(delay_ms); 
      
      // wait for a second
    }
}
