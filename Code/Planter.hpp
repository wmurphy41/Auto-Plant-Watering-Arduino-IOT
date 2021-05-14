#ifndef _PLANTER_HPP_
#define _PLANTER_HPP_

#include <RingBufCPP.h>

// Time constants
#define ONE_DAY_MS 86400000               // 1 day = 86,400,000 miliseconds
// #define ONE_DAY_MS 10000               // 1 day = 10 seconds = 10,0000 ms <== for testing
#define DAY_LOG_SIZE 8                    // Track 6 days of data (max buffer will hold)


#define ANALOG_RESOLUTION 12              // Read resolution of analog pin in bits
#define ANALOG_MAX_OUT 4096               // Corresponding max value



class Planter {
  public: 
  struct pinConfig { int pump; int led; int sensor; } ;

  private:
  // Parameters set during construction
  struct pinConfig pins ;
  unsigned long  reservoir_capacity_ms ;
  unsigned long sensor_cycle_time_ms ;
  unsigned long  pulse_time_ms ;
  int sensor_dry_volt ;
  int sensor_sat_volt ;
  
  // State tracking properties
  bool timeout_triggered ; 
  unsigned long last_check_time_ms ;
  unsigned long pump_start_ms ;
  unsigned long pump_stop_ms ;
  unsigned long total_run_time_ms ;
  unsigned int total_run_count ;
 
  // history tracking properties
  struct planterStatus {
    unsigned long tstamp_begin ;  
    unsigned long tstamp_end ;    

    int numReadings ;
    int moisturePercent ;         
    float avgMoisture ;
    int lowMoisture ; 
    int highMoisture ;

    int pumpStarts ;
    unsigned long pumpRunTime_ms ;
  } ;
  struct planterStatus moistureLog_Day0 ;
  RingBufCPP <struct planterStatus, DAY_LOG_SIZE> moistureLog_Daily ;

  public:
  Planter(struct pinConfig, int, int) ;

  unsigned long getTimeSinceLastStopSec() ;
  unsigned long getElapsedTimeSec() ;
  unsigned long getRunTimeSec() ;
  unsigned int getRunCount() ;
  bool isPumpOn() ;
  bool isTimeoutFlagSet() ;
  
  bool setResevervoirCapacity_MS(unsigned long) ;
  bool setPumpPulseTime_MS(unsigned long) ;
  bool setSensorDryVolt(int) ;
  bool setSensorSatVolt(int) ;
  unsigned long getResevervoirCapacity_MS() ;
  unsigned long  getPumpPulseTime_MS() ;
  int getSensorDryVolt() ;
  int getSensorSatVolt() ;
 
  String getDailyHistory() ;

  void setup() ;                                          // Initialze pump during setup() ;
  bool checkStatus() ;                                    // Must be run each loop.  Checks environment
  void reset() ;                                          // Reset after refilling pump
  void pumpOn() ;                                         // Turn on pump
  void pumpOff() ;                                        // Turn off pump
  bool checkPump() ;                                      // Should be run in each loop. Checks for timeout
  void pumpPulse(unsigned long override_pulse_ms = 0) ;   // Pulse pump on/off
  int getMoistureLevel() ;                                // Reads raw soil moisture level
  int getMoisturePercent(bool log_result=false) ;        // Read moisture from sensor. Return percent
} ;


#endif //_PLANTER_HPP_