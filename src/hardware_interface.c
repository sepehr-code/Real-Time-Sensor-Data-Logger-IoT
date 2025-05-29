#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include "../include/hardware_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

// Initialize hardware interface
int init_hardware_interface(hardware_interface_t* hw, const char* device_path) {
    if (!hw || !device_path) {
        return -1;
    }
    
    // Initialize configuration with defaults
    strcpy(hw->config.device_path, device_path);
    hw->config.baud_rate = B9600;
    hw->config.data_bits = 8;
    hw->config.stop_bits = 1;
    hw->config.parity = 'N';
    hw->config.timeout_ms = 1000;
    
    hw->is_connected = 0;
    hw->fd = -1;
    
    // Open serial port
    hw->fd = open(device_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (hw->fd == -1) {
        fprintf(stderr, "Error: Cannot open device '%s': %s\n", 
                device_path, strerror(errno));
        return -1;
    }
    
    // Save original terminal settings
    if (tcgetattr(hw->fd, &hw->original_termios) != 0) {
        fprintf(stderr, "Error: Cannot get terminal attributes: %s\n", strerror(errno));
        close(hw->fd);
        hw->fd = -1;
        return -1;
    }
    
    // Configure serial port
    if (configure_serial_port(hw, B9600) != 0) {
        close(hw->fd);
        hw->fd = -1;
        return -1;
    }
    
    hw->is_connected = 1;
    printf("Hardware interface initialized: %s\n", device_path);
    
    return 0;
}

// Configure serial port parameters
int configure_serial_port(hardware_interface_t* hw, int baud_rate) {
    if (!hw || hw->fd == -1) {
        return -1;
    }
    
    struct termios tty;
    
    // Get current settings
    if (tcgetattr(hw->fd, &tty) != 0) {
        fprintf(stderr, "Error: Cannot get terminal attributes: %s\n", strerror(errno));
        return -1;
    }
    
    // Set baud rate
    cfsetospeed(&tty, baud_rate);
    cfsetispeed(&tty, baud_rate);
    
    // Configure 8N1 (8 data bits, no parity, 1 stop bit)
    tty.c_cflag &= ~PARENB;        // No parity
    tty.c_cflag &= ~CSTOPB;        // 1 stop bit
    tty.c_cflag &= ~CSIZE;         // Clear data size bits
    tty.c_cflag |= CS8;            // 8 data bits
#ifdef CRTSCTS
    tty.c_cflag &= ~CRTSCTS;       // No hardware flow control
#endif
    tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control lines
    
    // Configure input modes
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // No software flow control
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw input
    
    // Configure output modes
    tty.c_oflag &= ~OPOST; // Raw output
    
    // Configure local modes
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw mode
    
    // Set timeout (deciseconds)
    tty.c_cc[VTIME] = hw->config.timeout_ms / 100;
    tty.c_cc[VMIN] = 0;
    
    // Apply settings
    if (tcsetattr(hw->fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Error: Cannot set terminal attributes: %s\n", strerror(errno));
        return -1;
    }
    
    hw->config.baud_rate = baud_rate;
    return 0;
}

// Read raw data from hardware
int read_hardware_data(hardware_interface_t* hw, char* buffer, size_t buffer_size) {
    if (!hw || !buffer || hw->fd == -1 || !hw->is_connected) {
        return -1;
    }
    
    // Use select for timeout
    fd_set read_fds;
    struct timeval timeout;
    
    FD_ZERO(&read_fds);
    FD_SET(hw->fd, &read_fds);
    
    timeout.tv_sec = hw->config.timeout_ms / 1000;
    timeout.tv_usec = (hw->config.timeout_ms % 1000) * 1000;
    
    int select_result = select(hw->fd + 1, &read_fds, NULL, NULL, &timeout);
    
    if (select_result == -1) {
        fprintf(stderr, "Error in select(): %s\n", strerror(errno));
        return -1;
    } else if (select_result == 0) {
        // Timeout
        return 0;
    }
    
    // Data is available, read it
    ssize_t bytes_read = read(hw->fd, buffer, buffer_size - 1);
    
    if (bytes_read == -1) {
        fprintf(stderr, "Error reading from device: %s\n", strerror(errno));
        return -1;
    }
    
    // Null-terminate the buffer
    buffer[bytes_read] = '\0';
    
    return (int)bytes_read;
}

// Send command to hardware device
int send_hardware_command(hardware_interface_t* hw, const char* command) {
    if (!hw || !command || hw->fd == -1 || !hw->is_connected) {
        return -1;
    }
    
    size_t command_len = strlen(command);
    ssize_t bytes_written = write(hw->fd, command, command_len);
    
    if (bytes_written == -1) {
        fprintf(stderr, "Error writing to device: %s\n", strerror(errno));
        return -1;
    }
    
    // Ensure data is transmitted
    tcdrain(hw->fd);
    
    return (int)bytes_written;
}

// Check if hardware is connected and responsive
int check_hardware_connection(hardware_interface_t* hw) {
    if (!hw || hw->fd == -1) {
        return 0;
    }
    
    // Send a simple ping command
    if (send_hardware_command(hw, "PING\n") <= 0) {
        hw->is_connected = 0;
        return 0;
    }
    
    // Try to read response
    char response[64];
    int bytes_read = read_hardware_data(hw, response, sizeof(response));
    
    if (bytes_read > 0) {
        // Check for expected response
        if (strstr(response, "PONG") || strstr(response, "OK")) {
            hw->is_connected = 1;
            return 1;
        }
    }
    
    // If no proper response, assume still connected but not responsive
    return hw->is_connected;
}

// Parse hardware data into sensor_data_t structure
int parse_hardware_data(const char* raw_data, sensor_data_t* sensor_data) {
    if (!raw_data || !sensor_data) {
        return -1;
    }
    
    // Try different parsing methods
    if (parse_arduino_sensor_data(raw_data, sensor_data) == 0) {
        return 0;
    }
    
    if (parse_modbus_sensor_data(raw_data, sensor_data) == 0) {
        return 0;
    }
    
    // If no parser worked, return error
    return -1;
}

// Read sensor data from hardware (high-level function)
int read_sensor_from_hardware(hardware_interface_t* hw, sensor_data_t* data) {
    if (!hw || !data || !hw->is_connected) {
        return -1;
    }
    
    char buffer[256];
    int bytes_read = read_hardware_data(hw, buffer, sizeof(buffer));
    
    if (bytes_read <= 0) {
        return -1;
    }
    
    // Parse the received data
    if (parse_hardware_data(buffer, data) != 0) {
        return -1;
    }
    
    // Set timestamp
    data->timestamp = get_current_time();
    
    return 0;
}

// Arduino sensor data parser
// Expected format: "SENSOR:TYPE:VALUE:UNIT:DESCRIPTION\n"
// Example: "SENSOR:TEMP:23.45:C:Temperature\n"
int parse_arduino_sensor_data(const char* raw_data, sensor_data_t* sensor_data) {
    if (!raw_data || !sensor_data) {
        return -1;
    }
    
    // Make a copy for parsing
    char data_copy[256];
    strncpy(data_copy, raw_data, sizeof(data_copy) - 1);
    data_copy[sizeof(data_copy) - 1] = '\0';
    
    // Remove newline characters
    char* newline = strchr(data_copy, '\n');
    if (newline) *newline = '\0';
    newline = strchr(data_copy, '\r');
    if (newline) *newline = '\0';
    
    // Parse using strtok
    char* token = strtok(data_copy, ":");
    if (!token || strcmp(token, "SENSOR") != 0) {
        return -1;
    }
    
    // Parse sensor type
    token = strtok(NULL, ":");
    if (!token) return -1;
    
    if (strcmp(token, "TEMP") == 0) {
        sensor_data->type = SENSOR_TEMPERATURE;
    } else if (strcmp(token, "VIB") == 0) {
        sensor_data->type = SENSOR_VIBRATION;
    } else if (strcmp(token, "STRAIN") == 0) {
        sensor_data->type = SENSOR_STRAIN;
    } else if (strcmp(token, "HUM") == 0) {
        sensor_data->type = SENSOR_HUMIDITY;
    } else if (strcmp(token, "PRESS") == 0) {
        sensor_data->type = SENSOR_PRESSURE;
    } else if (strcmp(token, "ACCEL_X") == 0) {
        sensor_data->type = SENSOR_ACCELEROMETER_X;
    } else if (strcmp(token, "ACCEL_Y") == 0) {
        sensor_data->type = SENSOR_ACCELEROMETER_Y;
    } else if (strcmp(token, "ACCEL_Z") == 0) {
        sensor_data->type = SENSOR_ACCELEROMETER_Z;
    } else {
        return -1;
    }
    
    // Parse value
    token = strtok(NULL, ":");
    if (!token) return -1;
    sensor_data->value = atof(token);
    
    // Parse unit
    token = strtok(NULL, ":");
    if (!token) return -1;
    strncpy(sensor_data->unit, token, sizeof(sensor_data->unit) - 1);
    sensor_data->unit[sizeof(sensor_data->unit) - 1] = '\0';
    
    // Parse description
    token = strtok(NULL, ":");
    if (token) {
        strncpy(sensor_data->description, token, sizeof(sensor_data->description) - 1);
        sensor_data->description[sizeof(sensor_data->description) - 1] = '\0';
    } else {
        strcpy(sensor_data->description, "Hardware Sensor");
    }
    
    return 0;
}

// Modbus sensor data parser
// Expected format: "MB:ADDR:REG:VALUE\n"
// Example: "MB:01:0001:2345\n" (Address 1, Register 1, Value 2345)
int parse_modbus_sensor_data(const char* raw_data, sensor_data_t* sensor_data) {
    if (!raw_data || !sensor_data) {
        return -1;
    }
    
    // Make a copy for parsing
    char data_copy[256];
    strncpy(data_copy, raw_data, sizeof(data_copy) - 1);
    data_copy[sizeof(data_copy) - 1] = '\0';
    
    // Remove newline characters
    char* newline = strchr(data_copy, '\n');
    if (newline) *newline = '\0';
    newline = strchr(data_copy, '\r');
    if (newline) *newline = '\0';
    
    // Parse using strtok
    char* token = strtok(data_copy, ":");
    if (!token || strcmp(token, "MB") != 0) {
        return -1;
    }
    
    // Parse address
    token = strtok(NULL, ":");
    if (!token) return -1;
    int address = atoi(token);
    
    // Parse register
    token = strtok(NULL, ":");
    if (!token) return -1;
    int reg = atoi(token);
    
    // Parse value
    token = strtok(NULL, ":");
    if (!token) return -1;
    int raw_value = atoi(token);
    
    // Convert based on register mapping (example mapping)
    switch (reg) {
        case 1:  // Temperature register
            sensor_data->type = SENSOR_TEMPERATURE;
            sensor_data->value = raw_value / 100.0;  // Assume 2 decimal places
            strcpy(sensor_data->unit, "°C");
            strcpy(sensor_data->description, "Modbus Temperature");
            break;
        case 2:  // Humidity register
            sensor_data->type = SENSOR_HUMIDITY;
            sensor_data->value = raw_value / 100.0;
            strcpy(sensor_data->unit, "%");
            strcpy(sensor_data->description, "Modbus Humidity");
            break;
        case 3:  // Pressure register
            sensor_data->type = SENSOR_PRESSURE;
            sensor_data->value = raw_value / 10.0;
            strcpy(sensor_data->unit, "hPa");
            strcpy(sensor_data->description, "Modbus Pressure");
            break;
        default:
            // Unknown register, treat as generic sensor
            sensor_data->type = SENSOR_TEMPERATURE;  // Default
            sensor_data->value = raw_value;
            strcpy(sensor_data->unit, "raw");
            snprintf(sensor_data->description, sizeof(sensor_data->description),
                    "Modbus Addr:%d Reg:%d", address, reg);
            break;
    }
    
    return 0;
}

// Cleanup hardware interface
void cleanup_hardware_interface(hardware_interface_t* hw) {
    if (!hw) return;
    
    if (hw->fd != -1) {
        // Restore original terminal settings
        tcsetattr(hw->fd, TCSANOW, &hw->original_termios);
        
        // Close file descriptor
        close(hw->fd);
        hw->fd = -1;
    }
    
    hw->is_connected = 0;
    
    printf("Hardware interface closed: %s\n", hw->config.device_path);
} 