#include "../include/data_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

// Initialize data logger
int init_data_logger(data_logger_t* logger, const char* base_filename) {
    if (!logger || !base_filename) {
        return -1;
    }
    
    // Set default configuration
    logger->config.max_file_size_mb = 10;
    logger->config.auto_rotate = 1;
    logger->config.buffer_size = 100;
    logger->config.flush_interval_ms = 1000;
    strcpy(logger->config.directory, "data");
    
    // Create data directory if it doesn't exist
    create_data_directory(logger->config.directory);
    
    // Generate filename with timestamp
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    snprintf(logger->current_filename, sizeof(logger->current_filename),
             "%s/%s_%04d%02d%02d_%02d%02d%02d.csv",
             logger->config.directory,
             base_filename,
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec);
    
    // Open file for writing
    logger->file = fopen(logger->current_filename, "w");
    if (!logger->file) {
        fprintf(stderr, "Error: Cannot create log file '%s': %s\n", 
                logger->current_filename, strerror(errno));
        return -1;
    }
    
    // Initialize buffer
    logger->buffer = malloc(logger->config.buffer_size * sizeof(sensor_data_t));
    if (!logger->buffer) {
        fclose(logger->file);
        return -1;
    }
    
    // Initialize state
    logger->current_file_size = 0;
    logger->sample_count = 0;
    logger->buffer_index = 0;
    logger->last_flush = get_current_time();
    
    // Write CSV header
    write_csv_header(logger);
    
    printf("Data logger initialized: %s\n", logger->current_filename);
    return 0;
}

// Configure logger settings
void configure_logger(data_logger_t* logger, logger_config_t config) {
    if (!logger) return;
    
    logger->config = config;
    
    // Reallocate buffer if size changed
    if (config.buffer_size != logger->config.buffer_size) {
        free(logger->buffer);
        logger->buffer = malloc(config.buffer_size * sizeof(sensor_data_t));
        logger->buffer_index = 0;
    }
}

// Write CSV header
int write_csv_header(data_logger_t* logger) {
    if (!logger || !logger->file) return -1;
    
    int bytes_written = fprintf(logger->file, 
        "Timestamp,Sensor_Type,Value,Unit,Description\n");
    
    if (bytes_written > 0) {
        logger->current_file_size += bytes_written;
        fflush(logger->file);
    }
    
    return bytes_written > 0 ? 0 : -1;
}

// Log single sensor data point
int log_sensor_data(data_logger_t* logger, const sensor_data_t* data) {
    if (!logger || !data) return -1;
    
    // Add to buffer
    logger->buffer[logger->buffer_index] = *data;
    logger->buffer_index++;
    logger->sample_count++;
    
    // Check if buffer is full or flush interval reached
    precise_time_t current_time = get_current_time();
    double time_since_flush = time_diff_ms(logger->last_flush, current_time);
    
    if (logger->buffer_index >= logger->config.buffer_size || 
        time_since_flush >= logger->config.flush_interval_ms) {
        return flush_logger_buffer(logger);
    }
    
    return 0;
}

// Log multiple sensor data points
int log_sensor_data_batch(data_logger_t* logger, const sensor_data_t* data_array, int count) {
    if (!logger || !data_array || count <= 0) return -1;
    
    for (int i = 0; i < count; i++) {
        if (log_sensor_data(logger, &data_array[i]) != 0) {
            return -1;
        }
    }
    
    return 0;
}

// Flush buffered data to file
int flush_logger_buffer(data_logger_t* logger) {
    if (!logger || !logger->file || logger->buffer_index == 0) return 0;
    
    char timestamp_str[64];
    const char* sensor_type_names[] = {
        "Temperature", "Vibration", "Strain", "Humidity", 
        "Pressure", "Accel_X", "Accel_Y", "Accel_Z"
    };
    
    for (int i = 0; i < logger->buffer_index; i++) {
        const sensor_data_t* data = &logger->buffer[i];
        
        // Format timestamp
        format_timestamp(data->timestamp, timestamp_str, sizeof(timestamp_str));
        
        // Get sensor type name
        const char* type_name = (data->type >= 0 && data->type < 8) ? 
                               sensor_type_names[data->type] : "Unknown";
        
        // Write CSV line
        int bytes_written = fprintf(logger->file, "%s,%s,%.6f,%s,%s\n",
                                   timestamp_str,
                                   type_name,
                                   data->value,
                                   data->unit,
                                   data->description);
        
        if (bytes_written > 0) {
            logger->current_file_size += bytes_written;
        }
    }
    
    // Flush to disk
    fflush(logger->file);
    
    // Reset buffer
    logger->buffer_index = 0;
    logger->last_flush = get_current_time();
    
    // Check if file rotation is needed
    if (logger->config.auto_rotate && 
        logger->current_file_size > (logger->config.max_file_size_mb * 1024 * 1024)) {
        return rotate_log_file(logger);
    }
    
    return 0;
}

// Rotate log file (create new file when current gets too large)
int rotate_log_file(data_logger_t* logger) {
    if (!logger) return -1;
    
    // Close current file
    if (logger->file) {
        fclose(logger->file);
        printf("Rotated log file: %s (%.2f MB)\n", 
               logger->current_filename, 
               logger->current_file_size / (1024.0 * 1024.0));
    }
    
    // Generate new filename
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    char base_name[256];
    // Extract base name from current filename
    char* last_slash = strrchr(logger->current_filename, '/');
    if (last_slash) {
        strcpy(base_name, last_slash + 1);
        char* first_underscore = strchr(base_name, '_');
        if (first_underscore) {
            *first_underscore = '\0';
        }
    } else {
        strcpy(base_name, "sensor_data");
    }
    
    snprintf(logger->current_filename, sizeof(logger->current_filename),
             "%s/%s_%04d%02d%02d_%02d%02d%02d.csv",
             logger->config.directory,
             base_name,
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec);
    
    // Open new file
    logger->file = fopen(logger->current_filename, "w");
    if (!logger->file) {
        fprintf(stderr, "Error: Cannot create new log file '%s': %s\n", 
                logger->current_filename, strerror(errno));
        return -1;
    }
    
    // Reset file size and write header
    logger->current_file_size = 0;
    write_csv_header(logger);
    
    printf("New log file created: %s\n", logger->current_filename);
    return 0;
}

// Get current log file statistics
void get_logger_stats(data_logger_t* logger, int* sample_count, long* file_size, char* filename) {
    if (!logger) return;
    
    if (sample_count) *sample_count = logger->sample_count;
    if (file_size) *file_size = logger->current_file_size;
    if (filename) strcpy(filename, logger->current_filename);
}

// Close and cleanup logger
void cleanup_data_logger(data_logger_t* logger) {
    if (!logger) return;
    
    // Flush any remaining buffered data
    flush_logger_buffer(logger);
    
    // Close file
    if (logger->file) {
        fclose(logger->file);
        logger->file = NULL;
    }
    
    // Free buffer
    if (logger->buffer) {
        free(logger->buffer);
        logger->buffer = NULL;
    }
    
    printf("Data logger closed. Total samples logged: %d\n", logger->sample_count);
}

// Create data directory if it doesn't exist
int create_data_directory(const char* directory) {
    struct stat st = {0};
    
    if (stat(directory, &st) == -1) {
        if (mkdir(directory, 0755) == 0) {
            printf("Created data directory: %s\n", directory);
            return 0;
        } else {
            fprintf(stderr, "Error creating directory '%s': %s\n", 
                    directory, strerror(errno));
            return -1;
        }
    }
    
    return 0;  // Directory already exists
}

// Backup log file (simple copy with .bak extension)
int backup_log_file(const char* filename) {
    if (!filename) return -1;
    
    char backup_name[512];
    snprintf(backup_name, sizeof(backup_name), "%s.bak", filename);
    
    FILE* src = fopen(filename, "rb");
    if (!src) return -1;
    
    FILE* dst = fopen(backup_name, "wb");
    if (!dst) {
        fclose(src);
        return -1;
    }
    
    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dst);
    }
    
    fclose(src);
    fclose(dst);
    
    printf("Backup created: %s\n", backup_name);
    return 0;
}

// Compress old logs (placeholder - would use gzip in real implementation)
int compress_old_logs(const char* directory) {
    printf("Log compression not implemented (would compress files in %s)\n", directory);
    return 0;
} 