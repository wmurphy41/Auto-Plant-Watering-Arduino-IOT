#ifndef _PLANTER_HPP_
#define _PLANTER_HPP_

#include <RingBufCPP.h>

// Time constants
#define ONE_DAY_MS 86400000               // 1 day = 86,400,000 miliseconds
// #define ONE_DAY_MS 10000                  // 1 day = 10 seconds = 10,0000 ms <== for testing
#define DAY_LOG_SIZE 8                    // Track 6 days of data (max buffer will hold)

// Parameter defaults
#define PUMP_RESERVOIR_CAPACITY_MS 90000  // Reservoir runs dry after 90 seconds (initial setting)
#define PUMP_PULSE_TIME_MS 1500           // Pulse for 1.5 sec
#define SENSOR_CYCLE_TIME_MS 1000         // Check sensors every second

#define MAX_PULSE_TIME_MS 5000            // Max pump pulse time is 5 seconds
#define MIN_PULSE_TIME_MS 500             // Min pump pulse time is .5 seconds
#define MAX_RESERVOIR_TIME_MS 300000      // Max reservoir capacity 5 minutes 



class Planter {
  public: 
  struct pinConfig { int pump; int led; int sensor; } ;

  private:
  // Parameters set during construction
  struct pinConfig pins ;
  unsigned long  reservoir_capacity_ms ;
  unsigned long  pulse_time_ms ;
  int sensor_dry_volt ;
  int sensor_sat_volt ;
  int sensor_cycle_time_ms ;
  
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
  int getMoisturePercent() ;
  bool isPumpOn() ;
  bool isTimeoutFlagSet() ;
  
  bool setResevervoirCapacity_MS(unsigned long) ;
  bool setPumpPulseTime_MS(unsigned long) ;
 
  String getParameters() ;
  String getDailyHistory() ;

  void setup() ;                                          // Initialze pump during setup() ;
  bool checkStatus() ;                                    // Must be run each loop.  Checks environment
  void reset() ;                                          // Reset after refilling pump
  void pumpOn() ;                                         // Turn on pump
  void pumpOff() ;                                        // Turn off pump
  bool checkPump() ;                                      // Should be run in each loop. Checks for timeout
  void pumpPulse(unsigned long override_pulse_ms = 0) ;   // Pulse pump on/off
  int readMoisture(bool log_result=false) ;               // Read moisture from sensor
} ;


#endif //_PLANTER_HPP_