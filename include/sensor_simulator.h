#ifndef SENSOR_SIMULATOR_H
#define SENSOR_SIMULATOR_H

#include "utils.h"

// Sensor types
typedef enum {
    SENSOR_TEMPERATURE,
    SENSOR_VIBRATION,
    SENSOR_STRAIN,
    SENSOR_HUMIDITY,
    SENSOR_PRESSURE,
    SENSOR_ACCELEROMETER_X,
    SENSOR_ACCELEROMETER_Y,
    SENSOR_ACCELEROMETER_Z
} sensor_type_t;

// Sensor data structure
typedef struct {
    sensor_type_t type;
    double value;
    precise_time_t timestamp;
    char unit[16];
    char description[64];
} sensor_data_t;

// Simulator configuration
typedef struct {
    double base_value;
    double noise_amplitude;
    double trend_rate;
    double seasonal_amplitude;
    double seasonal_period;
    int anomaly_probability;  // Percentage chance of anomaly per sample
    double anomaly_magnitude;
} sensor_config_t;

// Initialize sensor simulator
void init_sensor_simulator(void);

// Configure a specific sensor type
void configure_sensor(sensor_type_t type, sensor_config_t config);

// Generate simulated sensor data
sensor_data_t generate_sensor_data(sensor_type_t type);

// Generate bridge vibration data (specialized for structural monitoring)
sensor_data_t generate_bridge_vibration_data(void);

// Generate environmental data set (temperature, humidity, pressure)
void generate_environmental_data_set(sensor_data_t* data_array, int* count);

// Cleanup simulator resources
void cleanup_sensor_simulator(void);

#endif // SENSOR_SIMULATOR_H 