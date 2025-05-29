#include "../include/data_analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Initialize statistics structure
void init_statistics(statistics_t* stats) {
    if (!stats) return;
    
    stats->mean = 0.0;
    stats->min = INFINITY;
    stats->max = -INFINITY;
    stats->std_deviation = 0.0;
    stats->variance = 0.0;
    stats->median = 0.0;
    stats->sample_count = 0;
    stats->sum = 0.0;
    stats->sum_squares = 0.0;
}

// Update statistics with new data point
void update_statistics(statistics_t* stats, double value) {
    if (!stats) return;
    
    stats->sample_count++;
    stats->sum += value;
    stats->sum_squares += value * value;
    
    if (value < stats->min) stats->min = value;
    if (value > stats->max) stats->max = value;
}

// Calculate final statistics (call after all data points added)
void finalize_statistics(statistics_t* stats) {
    if (!stats || stats->sample_count == 0) return;
    
    // Calculate mean
    stats->mean = stats->sum / stats->sample_count;
    
    // Calculate variance and standard deviation
    stats->variance = (stats->sum_squares / stats->sample_count) - (stats->mean * stats->mean);
    stats->std_deviation = sqrt(stats->variance);
    
    // Note: Median calculation would require storing all values or using a more complex algorithm
    // For simplicity, we'll approximate median as mean for now
    stats->median = stats->mean;
}

// Initialize moving average filter
int init_moving_average(moving_average_t* ma, int window_size) {
    if (!ma || window_size <= 0) return -1;
    
    ma->buffer = malloc(window_size * sizeof(double));
    if (!ma->buffer) return -1;
    
    ma->buffer_size = window_size;
    ma->current_index = 0;
    ma->sample_count = 0;
    ma->sum = 0.0;
    
    // Initialize buffer with zeros
    for (int i = 0; i < window_size; i++) {
        ma->buffer[i] = 0.0;
    }
    
    return 0;
}

// Add value to moving average and get current average
double update_moving_average(moving_average_t* ma, double value) {
    if (!ma || !ma->buffer) return 0.0;
    
    // Remove old value from sum if buffer is full
    if (ma->sample_count >= ma->buffer_size) {
        ma->sum -= ma->buffer[ma->current_index];
    } else {
        ma->sample_count++;
    }
    
    // Add new value
    ma->buffer[ma->current_index] = value;
    ma->sum += value;
    
    // Update index (circular buffer)
    ma->current_index = (ma->current_index + 1) % ma->buffer_size;
    
    return ma->sum / ma->sample_count;
}

// Get current moving average value
double get_moving_average(moving_average_t* ma) {
    if (!ma || ma->sample_count == 0) return 0.0;
    return ma->sum / ma->sample_count;
}

// Cleanup moving average
void cleanup_moving_average(moving_average_t* ma) {
    if (!ma) return;
    
    if (ma->buffer) {
        free(ma->buffer);
        ma->buffer = NULL;
    }
    
    ma->buffer_size = 0;
    ma->current_index = 0;
    ma->sample_count = 0;
    ma->sum = 0.0;
}

// Detect anomaly in single data point
anomaly_result_t detect_anomaly(const sensor_data_t* data, const statistics_t* baseline_stats, 
                               const anomaly_config_t* config) {
    anomaly_result_t result;
    result.is_anomaly = 0;
    result.severity = 0.0;
    result.detected_at = data->timestamp;
    strcpy(result.description, "Normal");
    
    if (!data || !baseline_stats || !config || baseline_stats->sample_count < config->min_samples_for_analysis) {
        return result;
    }
    
    double deviation = fabs(data->value - baseline_stats->mean);
    double threshold = config->threshold_multiplier * baseline_stats->std_deviation;
    
    // Check statistical threshold
    if (deviation > threshold) {
        result.is_anomaly = 1;
        result.severity = deviation / baseline_stats->std_deviation;
        snprintf(result.description, sizeof(result.description),
                "Statistical anomaly: %.2f std devs from mean", result.severity);
    }
    
    // Check absolute threshold
    if (fabs(data->value) > config->absolute_threshold) {
        result.is_anomaly = 1;
        if (result.severity == 0.0) {
            result.severity = fabs(data->value) / config->absolute_threshold;
            snprintf(result.description, sizeof(result.description),
                    "Absolute threshold exceeded: %.2f", data->value);
        }
    }
    
    return result;
}

// Detect anomalies in a data array
int detect_anomalies_batch(const sensor_data_t* data_array, int count, 
                          const anomaly_config_t* config, anomaly_result_t* results) {
    if (!data_array || !config || !results || count <= 0) return -1;
    
    // Calculate baseline statistics
    statistics_t baseline;
    init_statistics(&baseline);
    
    for (int i = 0; i < count; i++) {
        update_statistics(&baseline, data_array[i].value);
    }
    finalize_statistics(&baseline);
    
    // Detect anomalies
    int anomaly_count = 0;
    for (int i = 0; i < count; i++) {
        results[i] = detect_anomaly(&data_array[i], &baseline, config);
        if (results[i].is_anomaly) {
            anomaly_count++;
        }
    }
    
    return anomaly_count;
}

// Trend analysis
trend_analysis_t analyze_trend(const sensor_data_t* data_array, int count, int window_size) {
    trend_analysis_t trend;
    trend.slope = 0.0;
    trend.correlation = 0.0;
    strcpy(trend.trend_direction, "stable");
    trend.confidence = 0.0;
    
    if (!data_array || count < window_size || window_size < 2) {
        return trend;
    }
    
    // Use the last 'window_size' points for trend analysis
    int start_idx = count - window_size;
    
    // Calculate linear regression (least squares)
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;
    
    for (int i = 0; i < window_size; i++) {
        double x = (double)i;
        double y = data_array[start_idx + i].value;
        
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }
    
    double n = (double)window_size;
    double denominator = n * sum_x2 - sum_x * sum_x;
    
    if (fabs(denominator) > 1e-10) {
        trend.slope = (n * sum_xy - sum_x * sum_y) / denominator;
        
        // Calculate correlation coefficient
        double mean_x = sum_x / n;
        double mean_y = sum_y / n;
        
        double sum_dx2 = 0.0, sum_dy2 = 0.0, sum_dxdy = 0.0;
        for (int i = 0; i < window_size; i++) {
            double x = (double)i;
            double y = data_array[start_idx + i].value;
            double dx = x - mean_x;
            double dy = y - mean_y;
            
            sum_dx2 += dx * dx;
            sum_dy2 += dy * dy;
            sum_dxdy += dx * dy;
        }
        
        if (sum_dx2 > 0 && sum_dy2 > 0) {
            trend.correlation = sum_dxdy / sqrt(sum_dx2 * sum_dy2);
            trend.confidence = fabs(trend.correlation);
        }
        
        // Determine trend direction
        if (fabs(trend.slope) < 1e-6) {
            strcpy(trend.trend_direction, "stable");
        } else if (trend.slope > 0) {
            strcpy(trend.trend_direction, "increasing");
        } else {
            strcpy(trend.trend_direction, "decreasing");
        }
    }
    
    return trend;
}

// Rate of change calculation
double calculate_rate_of_change(const sensor_data_t* data_array, int count, int window_size) {
    if (!data_array || count < 2 || window_size < 2) return 0.0;
    
    int start_idx = (count >= window_size) ? count - window_size : 0;
    int end_idx = count - 1;
    
    if (start_idx >= end_idx) return 0.0;
    
    double value_change = data_array[end_idx].value - data_array[start_idx].value;
    double time_change = time_diff_ms(data_array[start_idx].timestamp, 
                                     data_array[end_idx].timestamp) / 1000.0; // Convert to seconds
    
    return (time_change > 0) ? value_change / time_change : 0.0;
}

// Simplified frequency analysis (peak detection)
int analyze_frequency_spectrum(const double* values, int count, double* dominant_frequency, 
                              double* amplitude) {
    if (!values || !dominant_frequency || !amplitude || count < 4) return -1;
    
    *dominant_frequency = 0.0;
    *amplitude = 0.0;
    
    // Simple peak detection approach
    int peak_count = 0;
    double total_time = count * 0.1; // Assuming 100ms intervals
    
    for (int i = 1; i < count - 1; i++) {
        if (values[i] > values[i-1] && values[i] > values[i+1]) {
            peak_count++;
            if (values[i] > *amplitude) {
                *amplitude = values[i];
            }
        }
    }
    
    if (peak_count > 0 && total_time > 0) {
        *dominant_frequency = peak_count / total_time;
    }
    
    return 0;
}

// Bridge vibration specific analysis
bridge_analysis_t analyze_bridge_vibration(const sensor_data_t* vibration_data, int count) {
    bridge_analysis_t analysis;
    analysis.rms_amplitude = 0.0;
    analysis.peak_amplitude = 0.0;
    analysis.dominant_frequency = 0.0;
    analysis.safety_status = 0;
    strcpy(analysis.safety_message, "Insufficient data");
    
    if (!vibration_data || count < 10) {
        return analysis;
    }
    
    // Calculate RMS amplitude
    double sum_squares = 0.0;
    analysis.peak_amplitude = 0.0;
    
    for (int i = 0; i < count; i++) {
        double value = vibration_data[i].value;
        sum_squares += value * value;
        if (value > analysis.peak_amplitude) {
            analysis.peak_amplitude = value;
        }
    }
    
    analysis.rms_amplitude = sqrt(sum_squares / count);
    
    // Extract values for frequency analysis
    double* values = malloc(count * sizeof(double));
    if (values) {
        for (int i = 0; i < count; i++) {
            values[i] = vibration_data[i].value;
        }
        
        double amplitude;
        analyze_frequency_spectrum(values, count, &analysis.dominant_frequency, &amplitude);
        free(values);
    }
    
    // Safety assessment based on typical bridge vibration limits
    if (analysis.rms_amplitude < 0.1 && analysis.peak_amplitude < 0.3) {
        analysis.safety_status = 0;  // Safe
        strcpy(analysis.safety_message, "Normal vibration levels - Bridge is safe");
    } else if (analysis.rms_amplitude < 0.3 && analysis.peak_amplitude < 0.8) {
        analysis.safety_status = 1;  // Warning
        strcpy(analysis.safety_message, "Elevated vibration levels - Monitor closely");
    } else {
        analysis.safety_status = 2;  // Critical
        strcpy(analysis.safety_message, "CRITICAL: Excessive vibration - Immediate inspection required");
    }
    
    return analysis;
}

// Print analysis results
void print_statistics(const statistics_t* stats, const char* sensor_name) {
    if (!stats || !sensor_name) return;
    
    printf("\n=== %s Statistics ===\n", sensor_name);
    printf("Samples: %d\n", stats->sample_count);
    printf("Mean: %.6f\n", stats->mean);
    printf("Min: %.6f\n", stats->min);
    printf("Max: %.6f\n", stats->max);
    printf("Std Dev: %.6f\n", stats->std_deviation);
    printf("Variance: %.6f\n", stats->variance);
}

void print_anomaly_result(const anomaly_result_t* result) {
    if (!result) return;
    
    if (result->is_anomaly) {
        char timestamp_str[64];
        format_timestamp(result->detected_at, timestamp_str, sizeof(timestamp_str));
        printf("ANOMALY DETECTED at %s: %s (Severity: %.2f)\n", 
               timestamp_str, result->description, result->severity);
    }
}

void print_trend_analysis(const trend_analysis_t* trend) {
    if (!trend) return;
    
    printf("\n=== Trend Analysis ===\n");
    printf("Direction: %s\n", trend->trend_direction);
    printf("Slope: %.6f\n", trend->slope);
    printf("Correlation: %.6f\n", trend->correlation);
    printf("Confidence: %.2f%%\n", trend->confidence * 100.0);
}

void print_bridge_analysis(const bridge_analysis_t* analysis) {
    if (!analysis) return;
    
    printf("\n=== Bridge Vibration Analysis ===\n");
    printf("RMS Amplitude: %.6f m/s²\n", analysis->rms_amplitude);
    printf("Peak Amplitude: %.6f m/s²\n", analysis->peak_amplitude);
    printf("Dominant Frequency: %.3f Hz\n", analysis->dominant_frequency);
    printf("Safety Status: ");
    
    switch (analysis->safety_status) {
        case 0:
            printf("SAFE (Green)\n");
            break;
        case 1:
            printf("WARNING (Yellow)\n");
            break;
        case 2:
            printf("CRITICAL (Red)\n");
            break;
        default:
            printf("UNKNOWN\n");
            break;
    }
    
    printf("Message: %s\n", analysis->safety_message);
} 