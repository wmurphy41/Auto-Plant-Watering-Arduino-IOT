#ifndef _PLANTER_HPP_
#define _PLANTER_HPP_

#include <RingBufCPP.h>

// Time constants
// #define ONE_HOUR_MS 3600000               // 1 hour = 3,600,000 miliseconds
#define ONE_HOUR_MS 10000                    // 1 hour = 10 seconds = 10,0000 ms
#define HOUR_LOG_SIZE 24
#define DAY_LOG_SIZE 8

class Planter {
  public: 
  struct pinConfig { int pump; int led; int sensor; } ;

  private:
  // Properties set during construction
  struct pinConfig pins ;
  unsigned long  reservoir_capacity_ms ;
  unsigned long  pulse_time_ms ;
  int sensor_dry_volt ;
  int sensor_sat_volt ;
  
  // State tracking properties
  bool pump_on ;
  bool timeout_triggered ; 

  unsigned long pump_start_ms ;
  unsigned long pump_stop_ms ;
  unsigned long elapsed_start_time_ms ;
  unsigned long run_time_ms ;
  unsigned int run_count ;
 
  // history tracking properties
  struct moisture_reading {
    unsigned long tstamp ;
    int moisturePercent ;
  } ;
  RingBufCPP <struct moisture_reading, HOUR_LOG_SIZE> moistureLog_24hr ;
  RingBufCPP <struct moisture_reading, DAY_LOG_SIZE> moistureLog_8day ;
 
  public:
  Planter(struct pinConfig, unsigned long, unsigned long, int, int) ;

  unsigned long getTimeSinceLastStopSec() ;
  unsigned long getElapsedTimeSec() ;
  unsigned long getRunTimeSec() ;
  unsigned int getRunCount() ;
  String getRecentHistory() ;
  String getDailyHistory() ;
  bool isPumpRunning() ;
  bool isTimeoutFlagSet() ;
 
  void setup() ;                                          // Initialze pump during setup() ;
  void reset() ;                                          // Reset after refilling pump
  void pumpOn() ;                                         // Turn on pump
  void pumpOff() ;                                        // Turn off pump
  void pumpPulse(unsigned long override_pulse_ms = 0) ;   // Pulse pump on/off
  bool pumpCheck() ;                                      // Check that we still have water left
  int readMoisture() ;                                    // Read moisture from sensor
} ;


#endif //_PLANTER_HPP_