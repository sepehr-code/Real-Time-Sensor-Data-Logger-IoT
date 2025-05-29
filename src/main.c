#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "../include/utils.h"
#include "../include/sensor_simulator.h"
#include "../include/hardware_interface.h"
#include "../include/data_logger.h"
#include "../include/data_analyzer.h"

// Global variables for signal handling
static volatile int running = 1;
static data_logger_t* global_logger = NULL;
static hardware_interface_t* global_hw = NULL;

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    printf("\nReceived signal %d. Shutting down gracefully...\n", signal);
    running = 0;
}

// Print real-time status
void print_status(int sample_count, double current_value, const char* sensor_type, 
                 const statistics_t* stats, const anomaly_result_t* anomaly) {
    printf("\r[%d] %s: %.3f | Mean: %.3f | StdDev: %.3f", 
           sample_count, sensor_type, current_value, stats->mean, stats->std_deviation);
    
    if (anomaly && anomaly->is_anomaly) {
        printf(" | ANOMALY! (%.1f)", anomaly->severity);
    }
    
    printf("                    "); // Clear any remaining characters
    fflush(stdout);
}

// Bridge monitoring mode
int run_bridge_monitoring(int hardware_mode, const char* device_path, int duration, 
                         int interval, const char* output_file, double threshold) {
    printf("\n=== Bridge Vibration Monitoring Mode ===\n");
    
    // Initialize components
    data_logger_t logger;
    hardware_interface_t hw;
    statistics_t vibration_stats;
    moving_average_t moving_avg;
    anomaly_config_t anomaly_config;
    
    // Configure anomaly detection
    anomaly_config.threshold_multiplier = threshold;
    anomaly_config.absolute_threshold = 1.0;  // 1 m/s² absolute limit
    anomaly_config.window_size = 50;
    anomaly_config.min_samples_for_analysis = 20;
    
    // Initialize logger
    const char* log_filename = output_file ? output_file : "bridge_vibration";
    if (init_data_logger(&logger, log_filename) != 0) {
        fprintf(stderr, "Failed to initialize data logger\n");
        return -1;
    }
    global_logger = &logger;
    
    // Initialize hardware if needed
    if (hardware_mode) {
        if (init_hardware_interface(&hw, device_path) != 0) {
            cleanup_data_logger(&logger);
            return -1;
        }
        global_hw = &hw;
    } else {
        init_sensor_simulator();
    }
    
    // Initialize analysis components
    init_statistics(&vibration_stats);
    init_moving_average(&moving_avg, 20);  // 20-sample moving average
    
    // Data collection arrays for analysis
    const int max_samples = (duration * 1000) / interval;
    sensor_data_t* vibration_data = malloc(max_samples * sizeof(sensor_data_t));
    if (!vibration_data) {
        fprintf(stderr, "Memory allocation failed\n");
        if (hardware_mode) cleanup_hardware_interface(&hw);
        cleanup_data_logger(&logger);
        return -1;
    }
    
    printf("Starting bridge vibration monitoring...\n");
    printf("Duration: %d seconds | Interval: %d ms | Mode: %s\n", 
           duration, interval, hardware_mode ? "Hardware" : "Simulated");
    printf("Output: %s\n", logger.current_filename);
    printf("Press Ctrl+C to stop early\n\n");
    
    precise_time_t start_time = get_current_time();
    int sample_count = 0;
    int anomaly_count = 0;
    
    while (running && sample_count < max_samples) {
        sensor_data_t data;
        
        // Get sensor data
        if (hardware_mode) {
            if (read_sensor_from_hardware(&hw, &data) != 0) {
                printf("\nWarning: Failed to read from hardware, using simulated data\n");
                data = generate_bridge_vibration_data();
            }
        } else {
            data = generate_bridge_vibration_data();
        }
        
        // Store data for analysis
        vibration_data[sample_count] = data;
        
        // Update statistics
        update_statistics(&vibration_stats, data.value);
        double moving_average = update_moving_average(&moving_avg, data.value);
        
        // Log data
        log_sensor_data(&logger, &data);
        
        // Anomaly detection (after sufficient samples)
        anomaly_result_t anomaly = {0};
        if (sample_count >= anomaly_config.min_samples_for_analysis) {
            finalize_statistics(&vibration_stats);
            anomaly = detect_anomaly(&data, &vibration_stats, &anomaly_config);
            if (anomaly.is_anomaly) {
                anomaly_count++;
                print_anomaly_result(&anomaly);
            }
        }
        
        // Print real-time status (include moving average)
        printf("\r[%d] %s: %.3f | Mean: %.3f | StdDev: %.3f | MA: %.3f", 
               sample_count + 1, "Vibration", data.value, vibration_stats.mean, 
               vibration_stats.std_deviation, moving_average);
        
        if (anomaly.is_anomaly) {
            printf(" | ANOMALY! (%.1f)", anomaly.severity);
        }
        
        printf("                    "); // Clear any remaining characters
        fflush(stdout);
        
        sample_count++;
        
        // Check if duration exceeded
        precise_time_t current_time = get_current_time();
        if (time_diff_ms(start_time, current_time) >= (duration * 1000)) {
            break;
        }
        
        // Sleep for interval
        sleep_ms(interval);
    }
    
    printf("\n\nData collection completed.\n");
    
    // Final analysis
    finalize_statistics(&vibration_stats);
    bridge_analysis_t bridge_analysis = analyze_bridge_vibration(vibration_data, sample_count);
    trend_analysis_t trend = analyze_trend(vibration_data, sample_count, 50);
    
    // Print results
    print_statistics(&vibration_stats, "Bridge Vibration");
    print_bridge_analysis(&bridge_analysis);
    print_trend_analysis(&trend);
    
    printf("\nSummary:\n");
    printf("- Total samples: %d\n", sample_count);
    printf("- Anomalies detected: %d (%.1f%%)\n", 
           anomaly_count, (anomaly_count * 100.0) / sample_count);
    printf("- Data logged to: %s\n", logger.current_filename);
    
    // Cleanup
    free(vibration_data);
    cleanup_moving_average(&moving_avg);
    if (hardware_mode) cleanup_hardware_interface(&hw);
    cleanup_data_logger(&logger);
    cleanup_sensor_simulator();
    
    return 0;
}

// Environmental monitoring mode
int run_environmental_monitoring(int hardware_mode, const char* device_path, int duration, 
                                int interval, const char* output_file, double threshold) {
    printf("\n=== Environmental Monitoring Mode ===\n");
    
    // Initialize components
    data_logger_t logger;
    hardware_interface_t hw;
    statistics_t temp_stats, humidity_stats, pressure_stats;
    
    // Note: Anomaly detection not implemented for environmental mode in this version
    // Could be added later if needed
    (void)threshold; // Suppress unused parameter warning

    // Initialize logger
    const char* log_filename = output_file ? output_file : "environmental_data";
    if (init_data_logger(&logger, log_filename) != 0) {
        fprintf(stderr, "Failed to initialize data logger\n");
        return -1;
    }
    global_logger = &logger;
    
    // Initialize hardware if needed
    if (hardware_mode) {
        if (init_hardware_interface(&hw, device_path) != 0) {
            cleanup_data_logger(&logger);
            return -1;
        }
        global_hw = &hw;
    } else {
        init_sensor_simulator();
    }
    
    // Initialize statistics
    init_statistics(&temp_stats);
    init_statistics(&humidity_stats);
    init_statistics(&pressure_stats);
    
    printf("Starting environmental monitoring...\n");
    printf("Duration: %d seconds | Interval: %d ms | Mode: %s\n", 
           duration, interval, hardware_mode ? "Hardware" : "Simulated");
    printf("Output: %s\n", logger.current_filename);
    printf("Press Ctrl+C to stop early\n\n");
    
    precise_time_t start_time = get_current_time();
    int sample_count = 0;
    
    while (running) {
        sensor_data_t env_data[3];
        int env_count;
        
        // Get environmental data set
        if (hardware_mode) {
            // In hardware mode, try to read individual sensors
            env_count = 0;
            for (int i = 0; i < 3; i++) {
                if (read_sensor_from_hardware(&hw, &env_data[env_count]) == 0) {
                    env_count++;
                }
            }
            if (env_count == 0) {
                printf("\nWarning: Failed to read from hardware, using simulated data\n");
                generate_environmental_data_set(env_data, &env_count);
            }
        } else {
            generate_environmental_data_set(env_data, &env_count);
        }
        
        // Process each sensor reading
        for (int i = 0; i < env_count; i++) {
            // Update appropriate statistics
            switch (env_data[i].type) {
                case SENSOR_TEMPERATURE:
                    update_statistics(&temp_stats, env_data[i].value);
                    break;
                case SENSOR_HUMIDITY:
                    update_statistics(&humidity_stats, env_data[i].value);
                    break;
                case SENSOR_PRESSURE:
                    update_statistics(&pressure_stats, env_data[i].value);
                    break;
                default:
                    break;
            }
            
            // Log data
            log_sensor_data(&logger, &env_data[i]);
        }
        
        sample_count++;
        
        // Print status every 10 samples
        if (sample_count % 10 == 0) {
            printf("\r[%d] T:%.1f°C H:%.1f%% P:%.1fhPa", 
                   sample_count, 
                   env_data[0].value, 
                   env_data[1].value, 
                   env_data[2].value);
            fflush(stdout);
        }
        
        // Check if duration exceeded
        precise_time_t current_time = get_current_time();
        if (time_diff_ms(start_time, current_time) >= (duration * 1000)) {
            break;
        }
        
        // Sleep for interval
        sleep_ms(interval);
    }
    
    printf("\n\nData collection completed.\n");
    
    // Final analysis
    finalize_statistics(&temp_stats);
    finalize_statistics(&humidity_stats);
    finalize_statistics(&pressure_stats);
    
    // Print results
    print_statistics(&temp_stats, "Temperature");
    print_statistics(&humidity_stats, "Humidity");
    print_statistics(&pressure_stats, "Pressure");
    
    printf("\nSummary:\n");
    printf("- Total sample sets: %d\n", sample_count);
    printf("- Data logged to: %s\n", logger.current_filename);
    
    // Cleanup
    if (hardware_mode) cleanup_hardware_interface(&hw);
    cleanup_data_logger(&logger);
    cleanup_sensor_simulator();
    
    return 0;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    char* device_path = NULL;
    int duration = 60;
    int interval = 100;
    char* output_file = NULL;
    double threshold = 3.0;
    int hardware_mode = 0;
    
    int parse_result = parse_command_line_args(argc, argv, &device_path, &duration, 
                                              &interval, &output_file, &threshold, &hardware_mode);
    
    if (parse_result == 1) {
        // Help was shown
        return 0;
    } else if (parse_result == -1) {
        // Error in arguments
        return 1;
    }
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("Real-Time Sensor Data Logger\n");
    printf("============================\n");
    
    // Validate hardware mode requirements
    if (hardware_mode && !device_path) {
        fprintf(stderr, "Error: Hardware mode requires device path\n");
        return 1;
    }
    
    // Ask user for monitoring mode
    printf("\nSelect monitoring mode:\n");
    printf("1. Bridge Vibration Monitoring\n");
    printf("2. Environmental Monitoring (Temperature, Humidity, Pressure)\n");
    printf("Enter choice (1-2): ");
    
    int choice;
    if (scanf("%d", &choice) != 1) {
        fprintf(stderr, "Invalid input\n");
        return 1;
    }
    
    int result = 0;
    
    switch (choice) {
        case 1:
            result = run_bridge_monitoring(hardware_mode, device_path, duration, 
                                         interval, output_file, threshold);
            break;
        case 2:
            result = run_environmental_monitoring(hardware_mode, device_path, duration, 
                                                interval, output_file, threshold);
            break;
        default:
            fprintf(stderr, "Invalid choice\n");
            return 1;
    }
    
    if (result == 0) {
        printf("\nData logging completed successfully!\n");
    } else {
        printf("\nData logging failed with errors.\n");
    }
    
    return result;
} 