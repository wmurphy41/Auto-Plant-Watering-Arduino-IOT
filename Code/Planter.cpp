#include "Planter.hpp"
#include <Arduino.h>

// ---------------------------------------
// Planter Class
// ---------------------------------------
Planter::Planter(struct pinConfig p, unsigned long r_c_ms, unsigned long p_time_ms, int dv, int sv ) {

  // Initialize input parameters
  pins = p ;
  reservoir_capacity_ms = r_c_ms ;
  pulse_time_ms = p_time_ms ;
  sensor_dry_volt = dv ;
  sensor_sat_volt = sv ;

  // Other properties are set during pump::Setup
}
void Planter::setup() {

  pinMode(pins.pump, OUTPUT) ;
  pinMode(pins.led, OUTPUT) ;
  digitalWrite(pins.pump,0) ;
  digitalWrite(pins.led,0) ;
  analogReference(AR_EXTERNAL) ;
  pinMode(pins.sensor, INPUT) ;

  pump_start_ms = pump_stop_ms = elapsed_start_time_ms = millis() ;
  run_count = 0 ;
  run_time_ms = 0 ;
  pump_on = timeout_triggered = false ;
  
}

bool Planter::isTimeoutFlagSet() {
   return timeout_triggered ;
}

unsigned long Planter::getElapsedTimeSec() {
  return ((millis() - elapsed_start_time_ms)/1000) ;
}

unsigned long Planter::getRunTimeSec() {
  return (run_time_ms / 1000) ;
}

unsigned int Planter::getRunCount() {
   return run_count ;
}
bool Planter::isPumpRunning() {  // Returns true if pump is currently running
    return pump_on ;
}
unsigned long Planter::getTimeSinceLastStopSec() {
  return (millis() - pump_stop_ms)/1000 ;
}

String Planter::getRecentHistory() {
  String status = String("Recent (hours ago): \n") ;
  
  int n = moistureLog_24hr.numElements() - 1 ;
  int d = constrain (n - 1, 0, n - 8) ;
  for (int i = n ; i >= d; i--)   {                   // display only last 8 hours ;
    struct moisture_reading *pMR ; 
    if (pMR = moistureLog_24hr.peek(i))
      status += String(n - i) + String(": ") + String(pMR->moisturePercent) + "% \n" ;
  }
  
  return status ;
}

String Planter::getDailyHistory() {
  String status = String("\nPrevious (days ago): \n") ;
  
  int n = moistureLog_8day.numElements() - 1 ;
  for (int i = n ; i >= 0; i--) {
    struct moisture_reading *pMR ; 
    if (pMR = moistureLog_8day.peek(i))
      status += String(n - i + 1) + String(": ") + String(pMR->moisturePercent) + "% \n" ;
  }
  
  return status ;
}


// Turn on pump
void Planter::pumpOn() {
  if (! pump_on && ! timeout_triggered) {    // No pump, if it's already on or in timeout
    digitalWrite(pins.pump, 1) ;
    digitalWrite(pins.led,1) ;
    pump_on = true ;
    run_count++ ;
    pump_start_ms = millis() ;             // reset the start counter
  }
}

// Turn off pump
void Planter::pumpOff() {
  // Whether pump running flag is on or not, force pump stop
  digitalWrite(pins.pump,0) ;
  digitalWrite(pins.led,0) ;
  if (pump_on) {
    pump_stop_ms = millis() ;     // log when pump was stopped
    run_time_ms += (pump_stop_ms - pump_start_ms) ;
    timeout_triggered = (run_time_ms >= reservoir_capacity_ms) ;
    pump_on = false ;
  }
}

// Pulse pump
void Planter::pumpPulse(unsigned long override_pulse_ms)  {
  unsigned long p ;
  if (override_pulse_ms == 0) p = pulse_time_ms ;
  else p = override_pulse_ms ;

  if (! timeout_triggered) {
    pumpOn() ;
    delay(p) ;
    pumpOff() ;
  }
}

// if total time running is longer than the time it takes to empty the resevoir, then 
// shut off the pump and set the timeout flag.
bool Planter::pumpCheck() {

  if (pump_on && ! timeout_triggered )
    if ((millis() - pump_start_ms + run_time_ms) >= reservoir_capacity_ms)
      pumpOff() ;
          
  return timeout_triggered ;
}

// Reset pump after refill 
void Planter::reset() {        
  // reset statistics and clear timeout flag
  pumpOff() ;
  pump_start_ms = pump_stop_ms = elapsed_start_time_ms = millis() ;
  run_count = 0 ;
  run_time_ms = 0 ;
  pump_on = timeout_triggered = false ;
}

// Read moisture from sensor
int Planter::readMoisture() {
  
  static unsigned long lastHourlyReading = 0 ;
  static int numHourlyReadings = 0 ;
  struct moisture_reading reading ;

  // Take the reading
  
  reading.tstamp = millis() ;
  int soil_reading = analogRead(pins.sensor) ;
  reading.moisturePercent = map(soil_reading, sensor_dry_volt, sensor_sat_volt, 0, 100) ;

  // if it's been an hour since last reading, add to logs
  if (reading.tstamp - lastHourlyReading > ONE_HOUR_MS) {
    moistureLog_24hr.add(reading, true) ; // add to circular log
    lastHourlyReading = reading.tstamp ;

    if (++numHourlyReadings >= 24) {
      moistureLog_8day.add(reading, true) ;
      numHourlyReadings = 0 ;
    }
  }
  
  return reading.moisturePercent ;
}