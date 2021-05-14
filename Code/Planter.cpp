#include "Planter.hpp"
#include "hardware.h"
#include <Arduino.h>
#include <limits.h>

// ---------------------------------------
// Planter Class
// ---------------------------------------
Planter::Planter(struct pinConfig p, int dv, int sv ) {

  // Initialize input parameters
  pins = p ;
  sensor_dry_volt = dv ;
  sensor_sat_volt = sv ;
  
  reservoir_capacity_ms = PUMP_RESERVOIR_CAPACITY_MS ;
  pulse_time_ms = PUMP_PULSE_TIME_MS ;
  sensor_cycle_time_ms = SENSOR_CYCLE_TIME_MS ;


  // Other properties are initialized during pump::Setup
}
void Planter::setup() {


  pinMode(pins.pump, OUTPUT) ;
  pinMode(pins.led, OUTPUT) ;
  digitalWrite(pins.pump,LOW) ;
  digitalWrite(pins.led,LOW) ;

  // analogReference(DEFAULT) ;
  analogReadResolution(ANALOG_RESOLUTION) ;
  pinMode(pins.sensor, INPUT) ;

  unsigned long now = millis() ;
  last_check_time_ms = pump_start_ms = pump_stop_ms = now ;
  total_run_count = 0 ;
  total_run_time_ms = 0 ;
  timeout_triggered = false ;
  
  moistureLog_Day0.tstamp_begin = moistureLog_Day0.tstamp_end = now ;
  moistureLog_Day0.lowMoisture = INT_MAX ;
  getMoisturePercent(true) ;

}

// This is the regular check that should be called every loop cycle
// - Once a second (or as configured) check and log moisture
// - Check that the pump doesn't need to turn off
// - Check if hourly log needs to be updated.
// Returns yes, if there is any status change to note.
bool Planter::checkStatus() {
  bool statusMoisture = false ;
  unsigned long now = millis() ;

  // check and log moisture
  if (now - last_check_time_ms > sensor_cycle_time_ms) {
    last_check_time_ms = now ;
    getMoisturePercent(true) ; 
    statusMoisture = true ;
  }

  bool statusPump = checkPump() ;
  
  // if it's been an hour... update the hourly log.
  if (moistureLog_Day0.tstamp_end - moistureLog_Day0.tstamp_begin > ONE_DAY_MS) {
    moistureLog_Daily.add(moistureLog_Day0, true) ; // add to circular log

    // reset this-hour log
    moistureLog_Day0 = planterStatus() ; 
    moistureLog_Day0.tstamp_begin = moistureLog_Day0.tstamp_end = now ;
    moistureLog_Day0.lowMoisture = INT_MAX ;
    getMoisturePercent(true) ;
  }

  
  return (statusMoisture || statusPump) ;
}

bool Planter::isPumpOn() {
  
  return (digitalRead(pins.pump) == HIGH) ;
  
}
bool Planter::isTimeoutFlagSet() {
   return timeout_triggered ;
}

unsigned long Planter::getRunTimeSec() {
  return (total_run_time_ms / 1000) ;
}

unsigned int Planter::getRunCount() {
   return total_run_count ;
}
unsigned long Planter::getTimeSinceLastStopSec() {
  return (millis() - pump_stop_ms)/1000 ;
}

bool Planter::setResevervoirCapacity_MS(unsigned long rc_ms) {
  bool success = false ;
  if (rc_ms>=MIN_PULSE_TIME_MS && rc_ms<=MAX_RESERVOIR_TIME_MS) {
    reservoir_capacity_ms = rc_ms ;
    success = true ;
  }
  return success ;
}

bool Planter::setPumpPulseTime_MS(unsigned long ppt_ms) {
  bool success = false ;
  if (ppt_ms>=MIN_PULSE_TIME_MS && ppt_ms<=MAX_PULSE_TIME_MS) {
    pulse_time_ms = ppt_ms ;
    success = true ;
  }
  return success ;
}

bool Planter::setSensorDryVolt(int sdv) {
  bool success = false ;
  if (sdv >= 0 && sdv <=ANALOG_MAX_OUT) {
    sensor_dry_volt = sdv ;
    success = true ;
  }
  return success ;
}

bool Planter::setSensorSatVolt(int ssv) {
  bool success = false ;
  if (ssv >= 0 && ssv <=ANALOG_MAX_OUT) {
    sensor_sat_volt = ssv ;
    success = true ;
  }
  return success ;
}

unsigned long Planter::getResevervoirCapacity_MS() { return reservoir_capacity_ms ; }

unsigned long  Planter::getPumpPulseTime_MS() { return pulse_time_ms ; }

int Planter::getSensorDryVolt() { return sensor_dry_volt ; }

int Planter::getSensorSatVolt() { return sensor_sat_volt ; }

// Returns text string of current hour + most recent hours.
String Planter::getDailyHistory() {
  // Header
  String status = 
      String("Day\tlast\tavg\tMin\tMax\tR_#\tR_tm\n") +
      String("-----\t-----\t-----\t-----\t-----\t-----\t----- \n") ;
      
  // header  
  status += 
    String("0 \t") +
    String(moistureLog_Day0.moisturePercent) + "\t" +
    String((int)moistureLog_Day0.avgMoisture) + "\t" +
    String(moistureLog_Day0.lowMoisture) + "\t" +
    String(moistureLog_Day0.highMoisture) + "\t" +
    String(moistureLog_Day0.pumpStarts) + "\t" +
    String((int)moistureLog_Day0.pumpRunTime_ms/1000) + "\n" ;

  // Current Day
  int newest_i = moistureLog_Daily.numElements() - 1 ;      // Index of most recent entry
  int oldest_i = max ( 0, newest_i - (DAY_LOG_SIZE-1) )  ;
  for (int i=0 ; i <= (newest_i-oldest_i) ; i++)   {        
    
    struct planterStatus *pMR ; 
    if (pMR = moistureLog_Daily.peek(newest_i - i))
      status += 
        String(i+1) + "\t" +
        String(pMR->moisturePercent) + "\t" +
        String((int)pMR->avgMoisture) + "\t" +
        String(pMR->lowMoisture) + "\t" +
        String(pMR->highMoisture) + "\t" +
        String(pMR->pumpStarts) + "\t" +
        String((int)pMR->pumpRunTime_ms/1000) + "\n" ;
  }
  
  return status ;
}

// Turn on pump
void Planter::pumpOn() {
  
  if (! isPumpOn() && ! timeout_triggered) {    // No pump, if it's already on or in timeout
    digitalWrite(pins.pump, HIGH) ;
    digitalWrite(pins.led,HIGH) ;
    pump_start_ms = millis() ;             // reset the start counter
  }
}

// Turn off pump (whether it was on or not)
// If it had been on, log pump run time
void Planter::pumpOff() {
  
  bool wasPumpOn = isPumpOn() ;
  digitalWrite(pins.pump,LOW) ;
  digitalWrite(pins.led,LOW) ;

  if (wasPumpOn) {
    pump_stop_ms = millis() ;     // log when pump was stopped
    unsigned long pump_runtime_ms = pump_stop_ms - pump_start_ms ;
    total_run_count++ ;
    moistureLog_Day0.pumpStarts++ ;
    moistureLog_Day0.pumpRunTime_ms += pump_runtime_ms ;
    total_run_time_ms += pump_runtime_ms ;
  }
}

// Pulse pump
void Planter::pumpPulse(unsigned long override_pulse_ms)  {
  unsigned long p ;
  if (override_pulse_ms == 0) p = pulse_time_ms ;
  else p = override_pulse_ms ;
    
  if ((p  + total_run_time_ms) >= reservoir_capacity_ms) 
    timeout_triggered = true ;

  if (! timeout_triggered) {
    pumpOn() ;
    delay(p) ;
    pumpOff() ;
  }
}

// if total time running is longer than the time it takes to empty the reservoir, then 
// shut off the pump and set the timeout flag.
bool Planter::checkPump() {
  
  bool status_changed = false ;

  if ( isPumpOn() )
    if (((millis() - pump_start_ms) + total_run_time_ms) >= reservoir_capacity_ms) {
      pumpOff() ;
      timeout_triggered = true ;
      status_changed = true ;
    }
  return status_changed ;
}

// Reset pump after refill 
void Planter::reset() {        
  // reset statistics, clear timeout flag
  pumpOff() ;
  total_run_count = 0 ;
  total_run_time_ms = 0 ;
  timeout_triggered = false ;
}

// Read moisture from sensor and returns raw value
int Planter::getMoistureLevel() {
  return analogRead(pins.sensor) ;
}

// Read moisture from sensor, converts to percent, and logs
int Planter::getMoisturePercent(bool log_result) {
  
  // Take the reading and convert to moisture percent
  int soil_reading = getMoistureLevel() ;
  int moisture_percent = map(soil_reading, sensor_dry_volt, sensor_sat_volt, 0, 100) ;

  // Record data in the current log
  if (log_result) {
    unsigned long now = millis() ;
  
    // Log time stamps
    moistureLog_Day0.tstamp_end = now ;
    moistureLog_Day0.numReadings++ ;

    // Log moisture reading, calculate average, update min/max
    moistureLog_Day0.moisturePercent = moisture_percent ;
    moistureLog_Day0.avgMoisture = 
            (moistureLog_Day0.avgMoisture*(moistureLog_Day0.numReadings-1) + 
             moistureLog_Day0.moisturePercent) /
            moistureLog_Day0.numReadings ;
    moistureLog_Day0.lowMoisture = min(moistureLog_Day0.lowMoisture, moistureLog_Day0.moisturePercent) ;
    moistureLog_Day0.highMoisture = max(moistureLog_Day0.highMoisture, moistureLog_Day0.moisturePercent) ;
  }

  return moisture_percent ;
}

