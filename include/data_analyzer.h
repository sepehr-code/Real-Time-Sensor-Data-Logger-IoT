#ifndef DATA_ANALYZER_H
#define DATA_ANALYZER_H

#include "sensor_simulator.h"

// Statistical data structure
typedef struct {
    double mean;
    double min;
    double max;
    double std_deviation;
    double variance;
    double median;
    int sample_count;
    double sum;
    double sum_squares;
} statistics_t;

// Moving average filter
typedef struct {
    double* buffer;
    int buffer_size;
    int current_index;
    int sample_count;
    double sum;
} moving_average_t;

// Anomaly detection configuration
typedef struct {
    double threshold_multiplier;  // Multiplier for standard deviation threshold
    double absolute_threshold;    // Absolute threshold value
    int window_size;             // Window size for trend analysis
    int min_samples_for_analysis; // Minimum samples before analysis
} anomaly_config_t;

// Anomaly detection result
typedef struct {
    int is_anomaly;
    double severity;  // How far from normal (in standard deviations)
    char description[128];
    precise_time_t detected_at;
} anomaly_result_t;

// Trend analysis result
typedef struct {
    double slope;           // Rate of change
    double correlation;     // Correlation coefficient
    char trend_direction[16]; // "increasing", "decreasing", "stable"
    double confidence;      // Confidence in trend detection (0-1)
} trend_analysis_t;

// Initialize statistics structure
void init_statistics(statistics_t* stats);

// Update statistics with new data point
void update_statistics(statistics_t* stats, double value);

// Calculate final statistics (call after all data points added)
void finalize_statistics(statistics_t* stats);

// Initialize moving average filter
int init_moving_average(moving_average_t* ma, int window_size);

// Add value to moving average and get current average
double update_moving_average(moving_average_t* ma, double value);

// Get current moving average value
double get_moving_average(moving_average_t* ma);

// Cleanup moving average
void cleanup_moving_average(moving_average_t* ma);

// Anomaly detection functions
anomaly_result_t detect_anomaly(const sensor_data_t* data, const statistics_t* baseline_stats, 
                               const anomaly_config_t* config);

// Detect anomalies in a data array
int detect_anomalies_batch(const sensor_data_t* data_array, int count, 
                          const anomaly_config_t* config, anomaly_result_t* results);

// Trend analysis
trend_analysis_t analyze_trend(const sensor_data_t* data_array, int count, int window_size);

// Rate of change calculation
double calculate_rate_of_change(const sensor_data_t* data_array, int count, int window_size);

// FFT analysis for vibration data (simplified)
int analyze_frequency_spectrum(const double* values, int count, double* dominant_frequency, 
                              double* amplitude);

// Bridge vibration specific analysis
typedef struct {
    double rms_amplitude;
    double peak_amplitude;
    double dominant_frequency;
    int safety_status;  // 0 = safe, 1 = warning, 2 = critical
    char safety_message[128];
} bridge_analysis_t;

bridge_analysis_t analyze_bridge_vibration(const sensor_data_t* vibration_data, int count);

// Print analysis results
void print_statistics(const statistics_t* stats, const char* sensor_name);
void print_anomaly_result(const anomaly_result_t* result);
void print_trend_analysis(const trend_analysis_t* trend);
void print_bridge_analysis(const bridge_analysis_t* analysis);

#endif // DATA_ANALYZER_H 