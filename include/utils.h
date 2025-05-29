#ifndef UTILS_H
#define UTILS_H

#include <time.h>
#include <stdint.h>

// Time utilities
typedef struct {
    time_t timestamp;
    long nanoseconds;
} precise_time_t;

// Get current time with high precision
precise_time_t get_current_time(void);

// Format timestamp for CSV output
void format_timestamp(precise_time_t time, char* buffer, size_t buffer_size);

// Sleep for specified milliseconds
void sleep_ms(int milliseconds);

// Calculate time difference in milliseconds
double time_diff_ms(precise_time_t start, precise_time_t end);

// String utilities
void trim_whitespace(char* str);
int parse_command_line_args(int argc, char* argv[], char** device_path, 
                           int* duration, int* interval, char** output_file, 
                           double* threshold, int* hardware_mode);

// Math utilities
double clamp(double value, double min, double max);
double map_range(double value, double in_min, double in_max, double out_min, double out_max);

#endif // UTILS_H 