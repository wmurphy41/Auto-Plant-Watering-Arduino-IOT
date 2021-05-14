#ifndef _HARDWARE_H_
#define _HARDWARE_H_


// board constants
#define PUMP_CONTROLLER_PIN 8
#define LED_PIN 10
#define SENSOR_PIN A1

// Moisture sensor constants
#define MOISTURE_LOW_VOLT 1000 // Voltage when sensor is saturated
#define MOISTURE_HI_VOLT 3500  // ... when sensor is dry
#define SENSOR_CYCLE_TIME_MS 1000         // Check sensors every second


// Pump and reservoir settings
#define PUMP_RESERVOIR_CAPACITY_MS 90000  // Reservoir runs dry after 90 seconds (initial setting)
#define MAX_RESERVOIR_TIME_MS 300000      // Max reservoir capacity 5 minutes (for error checking)
#define PUMP_PULSE_TIME_MS 5000           // Pulse for 5.0 sec
#define MAX_PULSE_TIME_MS 5000            // Max pump pulse time is 5 seconds
#define MIN_PULSE_TIME_MS 500             // Min pump pulse time is .5 seconds


#endif // _HARDWARE_H_
