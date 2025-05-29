#ifndef HARDWARE_INTERFACE_H
#define HARDWARE_INTERFACE_H

#include "sensor_simulator.h"  // For sensor_data_t structure
#include <termios.h>

// Hardware interface configuration
typedef struct {
    char device_path[256];
    int baud_rate;
    int data_bits;
    int stop_bits;
    char parity;
    int timeout_ms;
} hardware_config_t;

// Hardware interface state
typedef struct {
    int fd;  // File descriptor for serial port
    struct termios original_termios;
    hardware_config_t config;
    int is_connected;
} hardware_interface_t;

// Initialize hardware interface
int init_hardware_interface(hardware_interface_t* hw, const char* device_path);

// Configure serial port parameters
int configure_serial_port(hardware_interface_t* hw, int baud_rate);

// Read raw data from hardware
int read_hardware_data(hardware_interface_t* hw, char* buffer, size_t buffer_size);

// Parse hardware data into sensor_data_t structure
int parse_hardware_data(const char* raw_data, sensor_data_t* sensor_data);

// Send command to hardware device
int send_hardware_command(hardware_interface_t* hw, const char* command);

// Check if hardware is connected and responsive
int check_hardware_connection(hardware_interface_t* hw);

// Read sensor data from hardware (high-level function)
int read_sensor_from_hardware(hardware_interface_t* hw, sensor_data_t* data);

// Cleanup hardware interface
void cleanup_hardware_interface(hardware_interface_t* hw);

// Hardware-specific parsers for different sensor protocols
int parse_arduino_sensor_data(const char* raw_data, sensor_data_t* sensor_data);
int parse_modbus_sensor_data(const char* raw_data, sensor_data_t* sensor_data);

#endif // HARDWARE_INTERFACE_H 