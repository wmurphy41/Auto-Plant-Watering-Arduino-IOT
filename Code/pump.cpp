#include "pump.hpp"
#include <Arduino.h>

// ---------------------------------------
// Pump Class
// ---------------------------------------
Pump::Pump(int p_pin, int l_pin, unsigned long r_c_ms, unsigned long p_time_ms ) {

  // Initialize input parameters
  pump_pin = p_pin ;
  led_pin = l_pin ;
  reservoir_capacity_ms = r_c_ms ;
  pulse_time_ms = p_time_ms ;

  // Other properties are set during pump::Setup
}
void Pump::setup() {

  pinMode(pump_pin, OUTPUT) ;
  pinMode(led_pin, OUTPUT) ;
  digitalWrite(pump_pin,0) ;
  digitalWrite(led_pin,0) ;

  pump_start_ms = pump_stop_ms = elapsed_start_time_ms = millis() ;
  run_count = 0 ;
  run_time_ms = 0 ;
  pump_on = timeout_triggered = false ;
  
}

bool Pump::isTimeoutFlagSet() {
   return timeout_triggered ;
}

unsigned long Pump::getElapsedTimeSec() {
  return ((millis() - elapsed_start_time_ms)/1000) ;
}

unsigned long Pump::getRunTimeSec() {
  return (run_time_ms / 1000) ;
}

unsigned int Pump::getRunCount() {
   return run_count ;
}
bool Pump::isPumpRunning() {  // Returns true if pump is currently running
    return pump_on ;
}
unsigned long Pump::getTimeSinceLastStopSec() {
  return (millis() - pump_stop_ms)/1000 ;
}
void Pump::pumpOn() {
  if (! pump_on && ! timeout_triggered) {    // No pump, if it's already on or in timeout
    digitalWrite(pump_pin, 1) ;
    digitalWrite(led_pin,1) ;
    pump_on = true ;
    run_count++ ;
    pump_start_ms = millis() ;             // reset the start counter
  }
}

void Pump::pumpOff() {
  // Whether pump running flag is on or not, force pump stop
  digitalWrite(pump_pin,0) ;
  digitalWrite(led_pin,0) ;
  if (pump_on) {
    pump_stop_ms = millis() ;     // log when pump was stopped
    run_time_ms += (pump_stop_ms - pump_start_ms) ;
    timeout_triggered = (run_time_ms >= reservoir_capacity_ms) ;
    pump_on = false ;
  }
}

void Pump::pumpPulse(unsigned long override_pulse_ms)  {
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
bool Pump::pumpCheck() {

  if (pump_on && ! timeout_triggered )
    if ((millis() - pump_start_ms + run_time_ms) >= reservoir_capacity_ms)
      pumpOff() ;
          
  return timeout_triggered ;
}

void Pump::reset() {        
  // reset statistics and clear timeout flag
  pumpOff() ;
  pump_start_ms = pump_stop_ms = elapsed_start_time_ms = millis() ;
  run_count = 0 ;
  run_time_ms = 0 ;
  pump_on = timeout_triggered = false ;
}