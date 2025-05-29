#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

// Get current time with high precision
precise_time_t get_current_time(void) {
    precise_time_t current_time;
    struct timespec ts;
    
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        current_time.timestamp = ts.tv_sec;
        current_time.nanoseconds = ts.tv_nsec;
    } else {
        // Fallback to lower precision
        current_time.timestamp = time(NULL);
        current_time.nanoseconds = 0;
    }
    
    return current_time;
}

// Format timestamp for CSV output
void format_timestamp(precise_time_t time, char* buffer, size_t buffer_size) {
    struct tm* tm_info = localtime(&time.timestamp);
    
    snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec,
             time.nanoseconds / 1000);  // Convert to microseconds
}

// Sleep for specified milliseconds
void sleep_ms(int milliseconds) {
    usleep(milliseconds * 1000);
}

// Calculate time difference in milliseconds
double time_diff_ms(precise_time_t start, precise_time_t end) {
    double diff_sec = difftime(end.timestamp, start.timestamp);
    double diff_ns = (double)(end.nanoseconds - start.nanoseconds);
    
    return (diff_sec * 1000.0) + (diff_ns / 1000000.0);
}

// Trim whitespace from string
void trim_whitespace(char* str) {
    char* end;
    
    // Trim leading space
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return;  // All spaces
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    end[1] = '\0';
}

// Parse command line arguments
int parse_command_line_args(int argc, char* argv[], char** device_path, 
                           int* duration, int* interval, char** output_file, 
                           double* threshold, int* hardware_mode) {
    // Set defaults
    *device_path = NULL;
    *duration = 60;        // 60 seconds default
    *interval = 100;       // 100ms default
    *output_file = NULL;
    *threshold = 3.0;      // 3 standard deviations default
    *hardware_mode = 0;    // Simulated mode default
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--hardware") == 0 && i + 1 < argc) {
            *hardware_mode = 1;
            *device_path = argv[++i];
        } else if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) {
            *duration = atoi(argv[++i]);
            if (*duration <= 0) {
                fprintf(stderr, "Error: Duration must be positive\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc) {
            *interval = atoi(argv[++i]);
            if (*interval <= 0) {
                fprintf(stderr, "Error: Interval must be positive\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            *output_file = argv[++i];
        } else if (strcmp(argv[i], "--threshold") == 0 && i + 1 < argc) {
            *threshold = atof(argv[++i]);
            if (*threshold <= 0) {
                fprintf(stderr, "Error: Threshold must be positive\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Real-Time Sensor Data Logger\n\n");
            printf("Usage: %s [OPTIONS]\n\n", argv[0]);
            printf("Options:\n");
            printf("  --hardware <device>   Use hardware mode with specified device (e.g., /dev/ttyUSB0)\n");
            printf("  --duration <seconds>  Set logging duration in seconds (default: 60)\n");
            printf("  --interval <ms>       Set sampling interval in milliseconds (default: 100)\n");
            printf("  --output <filename>   Set output CSV filename\n");
            printf("  --threshold <value>   Set anomaly detection threshold (default: 3.0)\n");
            printf("  --help, -h            Show this help message\n\n");
            printf("Examples:\n");
            printf("  %s                                    # Simulated mode, 60 seconds\n", argv[0]);
            printf("  %s --duration 300 --interval 50      # Simulated mode, 5 minutes, 50ms interval\n", argv[0]);
            printf("  %s --hardware /dev/ttyUSB0            # Hardware mode with USB device\n", argv[0]);
            return 1;  // Indicate help was shown
        } else {
            fprintf(stderr, "Error: Unknown argument '%s'\n", argv[i]);
            fprintf(stderr, "Use --help for usage information\n");
            return -1;
        }
    }
    
    return 0;  // Success
}

// Clamp value between min and max
double clamp(double value, double min, double max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Map value from one range to another
double map_range(double value, double in_min, double in_max, double out_min, double out_max) {
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
} 