#ifndef PREDICTIVE_MODEL_H
#define PREDICTIVE_MODEL_H

#include <linux/types.h>

#define PREDICTION_WINDOW_MS 100   // Prediction window size
#define HISTORY_SIZE 100           // Number of historical data points to keep
#define MAX_PATTERNS 20            // Maximum number of workload patterns to track

typedef struct {
    u64 timestamp;                 // Timestamp in nanoseconds
    u32 cpu_util;                  // CPU utilization percentage (0-100)
    u32 freq;                      // Current CPU frequency in KHz
    u32 irq_count;                 // Number of interrupts since last sample
    u32 process_switches;          // Number of context switches
    u64 idle_time;                 // Total idle time since last sample
    u32 iowait;                    // I/O wait percentage
    u32 runnable_tasks;            // Number of tasks in runnable state
} cpu_metrics_t;

typedef struct {
    cpu_metrics_t history[HISTORY_SIZE];  // Ring buffer of historical metrics
    u32 history_index;                    // Current index in history buffer
    u32 history_count;                    // Number of valid entries in history

    // Prediction algorithm parameters
    u32 prediction_window;                // How far ahead to predict (ms)
    u32 aggressiveness;                   // How aggressively to scale (0-100)
    u32 learning_rate;                    // Model adaptation rate

    // Pattern recognition data
    struct workload_pattern {
        u32 pattern_id;                   // Pattern identifier
        u32 frequency;                    // How often this pattern occurs
        u32 duration;                     // Typical duration
        u32 avg_cpu_util;                 // Average CPU utilization for pattern
        u32 target_freq;                  // Optimal frequency for this pattern
        u8 metrics_signature[16];         // Signature of metrics that identify this pattern
    } patterns[MAX_PATTERNS];
    u32 pattern_count;                    // Number of detected patterns

    // Model statistics
    u32 predictions_made;                 // Total number of predictions made
    u32 prediction_errors;                // Number of significant prediction errors
    u32 avg_prediction_error;             // Average prediction error percentage
} prediction_model_t;

void init_prediction_model(prediction_model_t *model);
void add_metrics_to_history(prediction_model_t *model, cpu_metrics_t *metrics);
u32 predict_cpu_utilization(prediction_model_t *model);
int apply_pattern_adjustment(prediction_model_t *model, int base_prediction);
void generate_pattern_signature(cpu_metrics_t *current, cpu_metrics_t *prev1, cpu_metrics_t *prev2, u8 *signature);
bool pattern_signature_match(const u8 *sig1, const u8 *sig2);
void update_patterns(prediction_model_t *model);

#endif // PREDICTIVE_MODEL_H
