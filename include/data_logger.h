#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include "sensor_simulator.h"
#include <stdio.h>

// Logger configuration
typedef struct {
    char filename[256];
    char directory[256];
    int max_file_size_mb;
    int auto_rotate;
    int buffer_size;
    int flush_interval_ms;
} logger_config_t;

// Logger state
typedef struct {
    FILE* file;
    logger_config_t config;
    char current_filename[512];
    long current_file_size;
    int sample_count;
    sensor_data_t* buffer;
    int buffer_index;
    precise_time_t last_flush;
} data_logger_t;

// Initialize data logger
int init_data_logger(data_logger_t* logger, const char* base_filename);

// Configure logger settings
void configure_logger(data_logger_t* logger, logger_config_t config);

// Log single sensor data point
int log_sensor_data(data_logger_t* logger, const sensor_data_t* data);

// Log multiple sensor data points
int log_sensor_data_batch(data_logger_t* logger, const sensor_data_t* data_array, int count);

// Flush buffered data to file
int flush_logger_buffer(data_logger_t* logger);

// Rotate log file (create new file when current gets too large)
int rotate_log_file(data_logger_t* logger);

// Write CSV header
int write_csv_header(data_logger_t* logger);

// Get current log file statistics
void get_logger_stats(data_logger_t* logger, int* sample_count, long* file_size, char* filename);

// Close and cleanup logger
void cleanup_data_logger(data_logger_t* logger);

// Utility functions for data management
int create_data_directory(const char* directory);
int backup_log_file(const char* filename);
int compress_old_logs(const char* directory);

#endif // DATA_LOGGER_H 