#define _USE_MATH_DEFINES

#include "../include/sensor_simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Define M_PI if not available
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Global simulator state
static sensor_config_t sensor_configs[8];  // For each sensor type
static int simulator_initialized = 0;
static unsigned int simulation_step = 0;

// Default sensor configurations
static const sensor_config_t default_configs[] = {
    // SENSOR_TEMPERATURE
    {20.0, 2.0, 0.001, 5.0, 86400.0, 2, 15.0},  // 20°C base, daily cycle
    // SENSOR_VIBRATION
    {0.1, 0.05, 0.0, 0.02, 1.0, 5, 2.0},        // Low vibration baseline
    // SENSOR_STRAIN
    {100.0, 10.0, 0.002, 20.0, 3600.0, 3, 50.0}, // Strain in microstrains
    // SENSOR_HUMIDITY
    {50.0, 5.0, 0.001, 10.0, 43200.0, 1, 20.0},  // 50% RH base
    // SENSOR_PRESSURE
    {1013.25, 2.0, 0.0, 5.0, 21600.0, 1, 30.0},  // Standard atmospheric pressure
    // SENSOR_ACCELEROMETER_X
    {0.0, 0.1, 0.0, 0.05, 0.1, 8, 5.0},          // Accelerometer X-axis
    // SENSOR_ACCELEROMETER_Y
    {0.0, 0.1, 0.0, 0.05, 0.1, 8, 5.0},          // Accelerometer Y-axis
    // SENSOR_ACCELEROMETER_Z
    {9.81, 0.1, 0.0, 0.05, 0.1, 8, 2.0}          // Accelerometer Z-axis (gravity)
};

// Sensor metadata
static const struct {
    const char* unit;
    const char* description;
} sensor_metadata[] = {
    {"°C", "Temperature"},
    {"m/s²", "Vibration Amplitude"},
    {"µε", "Strain"},
    {"%", "Relative Humidity"},
    {"hPa", "Atmospheric Pressure"},
    {"m/s²", "Acceleration X"},
    {"m/s²", "Acceleration Y"},
    {"m/s²", "Acceleration Z"}
};

// Initialize sensor simulator
void init_sensor_simulator(void) {
    if (simulator_initialized) return;
    
    // Initialize random seed
    srand((unsigned int)time(NULL));
    
    // Copy default configurations
    for (int i = 0; i < 8; i++) {
        sensor_configs[i] = default_configs[i];
    }
    
    simulation_step = 0;
    simulator_initialized = 1;
    
    printf("Sensor simulator initialized with %d sensor types\n", 8);
}

// Configure a specific sensor type
void configure_sensor(sensor_type_t type, sensor_config_t config) {
    if (!simulator_initialized) {
        init_sensor_simulator();
    }
    
    if (type >= 0 && type < 8) {
        sensor_configs[type] = config;
        printf("Configured sensor type %d\n", type);
    }
}

// Generate random noise
static double generate_noise(double amplitude) {
    return amplitude * (2.0 * ((double)rand() / RAND_MAX) - 1.0);
}

// Generate seasonal variation
static double generate_seasonal(double amplitude, double period, double time_offset) {
    return amplitude * sin(2.0 * M_PI * time_offset / period);
}

// Generate anomaly if probability triggers
static double generate_anomaly(const sensor_config_t* config) {
    if ((rand() % 100) < config->anomaly_probability) {
        double sign = (rand() % 2) ? 1.0 : -1.0;
        return sign * config->anomaly_magnitude;
    }
    return 0.0;
}

// Generate simulated sensor data
sensor_data_t generate_sensor_data(sensor_type_t type) {
    sensor_data_t data;
    
    if (!simulator_initialized) {
        init_sensor_simulator();
    }
    
    if (type < 0 || type >= 8) {
        // Return invalid data
        data.type = type;
        data.value = 0.0;
        data.timestamp = get_current_time();
        strcpy(data.unit, "N/A");
        strcpy(data.description, "Invalid sensor type");
        return data;
    }
    
    const sensor_config_t* config = &sensor_configs[type];
    
    // Get current time for calculations
    data.timestamp = get_current_time();
    double time_offset = (double)simulation_step * 0.1;  // Assume 100ms intervals
    
    // Calculate base value with trend
    double base_value = config->base_value + (config->trend_rate * time_offset);
    
    // Add seasonal variation
    double seasonal = generate_seasonal(config->seasonal_amplitude, 
                                       config->seasonal_period, time_offset);
    
    // Add noise
    double noise = generate_noise(config->noise_amplitude);
    
    // Add potential anomaly
    double anomaly = generate_anomaly(config);
    
    // Combine all components
    data.value = base_value + seasonal + noise + anomaly;
    
    // Set metadata
    data.type = type;
    strcpy(data.unit, sensor_metadata[type].unit);
    strcpy(data.description, sensor_metadata[type].description);
    
    // Apply sensor-specific constraints
    switch (type) {
        case SENSOR_TEMPERATURE:
            data.value = clamp(data.value, -50.0, 80.0);
            break;
        case SENSOR_HUMIDITY:
            data.value = clamp(data.value, 0.0, 100.0);
            break;
        case SENSOR_PRESSURE:
            data.value = clamp(data.value, 800.0, 1200.0);
            break;
        case SENSOR_VIBRATION:
            data.value = fabs(data.value);  // Vibration is always positive
            break;
        default:
            break;
    }
    
    simulation_step++;
    return data;
}

// Generate bridge vibration data (specialized for structural monitoring)
sensor_data_t generate_bridge_vibration_data(void) {
    sensor_data_t data = generate_sensor_data(SENSOR_VIBRATION);
    
    // Add bridge-specific characteristics
    double time_offset = (double)simulation_step * 0.1;
    
    // Simulate traffic loading (periodic with some randomness)
    double traffic_frequency = 0.1 + 0.05 * sin(time_offset * 0.01);  // Variable frequency
    double traffic_amplitude = 0.02 + 0.01 * sin(time_offset * 0.005);
    double traffic_vibration = traffic_amplitude * sin(2.0 * M_PI * traffic_frequency * time_offset);
    
    // Add wind effects (lower frequency, higher amplitude)
    double wind_vibration = 0.005 * sin(2.0 * M_PI * 0.02 * time_offset);
    
    // Combine with base vibration
    data.value += fabs(traffic_vibration + wind_vibration);
    
    // Ensure realistic bridge vibration levels (typically < 1 m/s²)
    data.value = clamp(data.value, 0.0, 1.0);
    
    strcpy(data.description, "Bridge Vibration");
    
    return data;
}

// Generate environmental data set (temperature, humidity, pressure)
void generate_environmental_data_set(sensor_data_t* data_array, int* count) {
    if (!data_array || !count) return;
    
    data_array[0] = generate_sensor_data(SENSOR_TEMPERATURE);
    data_array[1] = generate_sensor_data(SENSOR_HUMIDITY);
    data_array[2] = generate_sensor_data(SENSOR_PRESSURE);
    
    *count = 3;
    
    // Correlate humidity with temperature (inverse relationship)
    if (data_array[0].value > 25.0) {
        data_array[1].value *= 0.8;  // Reduce humidity when hot
    } else if (data_array[0].value < 10.0) {
        data_array[1].value *= 1.2;  // Increase humidity when cold
    }
    
    // Ensure humidity stays within bounds
    data_array[1].value = clamp(data_array[1].value, 0.0, 100.0);
}

// Cleanup simulator resources
void cleanup_sensor_simulator(void) {
    if (simulator_initialized) {
        printf("Sensor simulator cleaned up. Generated %u samples.\n", simulation_step);
        simulator_initialized = 0;
        simulation_step = 0;
    }
} 