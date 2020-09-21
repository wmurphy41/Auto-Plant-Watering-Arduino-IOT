#ifndef _PUMP_HPP_
#define _PUMP_HPP_

class Pump {
  private:
 
  // Properties set during construction
  int pump_pin;
  int led_pin ;
  unsigned long  reservoir_capacity_ms ;
  unsigned long  pulse_time_ms ;
  
  // State tracking properties
  bool pump_on ;
  bool timeout_triggered ; 

  unsigned long pump_start_ms ;
  unsigned long pump_stop_ms ;
  unsigned long elapsed_start_time_ms ;
  unsigned long run_time_ms ;
  unsigned int run_count ;
 
  public:
  Pump(int, int, unsigned long, unsigned long) ;

  unsigned long getTimeSinceLastStopSec() ;
  unsigned long getElapsedTimeSec() ;
  unsigned long getRunTimeSec() ;
  unsigned int getRunCount() ;
  bool isPumpRunning() ;
  bool isTimeoutFlagSet() ;
 
  void setup() ;
  void reset() ;
  void pumpOn() ;
  void pumpOff() ;
  void pumpPulse(unsigned long override_pulse_ms = 0) ;  // Pulse pump on/off
  bool pumpCheck() ;                            // Check that we still have water left
} ;


#endif //_PUMP_HPP_