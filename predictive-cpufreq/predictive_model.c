#include <linux/kernel.h>
#include <linux/string.h>
#include "predictive_model.h"

void init_prediction_model(prediction_model_t *model)
{
    memset(model, 0, sizeof(*model));
    model->prediction_window = PREDICTION_WINDOW_MS;
    model->aggressiveness = 50;  // Medium aggressiveness by default
    model->learning_rate = 10;   // Learning rate percentage
    model->history_index = 0;
    model->history_count = 0;
    model->pattern_count = 0;
}

void add_metrics_to_history(prediction_model_t *model, cpu_metrics_t *metrics)
{
    model->history[model->history_index] = *metrics;
    model->history_index = (model->history_index + 1) % HISTORY_SIZE;
    if (model->history_count < HISTORY_SIZE)
        model->history_count++;
}

u32 predict_cpu_utilization(prediction_model_t *model)
{
    if (model->history_count < 5)
        return 50;  // Default prediction if not enough history

    cpu_metrics_t *current = &model->history[(model->history_index - 1 + HISTORY_SIZE) % HISTORY_SIZE];
    cpu_metrics_t *prev1 = &model->history[(model->history_index - 2 + HISTORY_SIZE) % HISTORY_SIZE];
    cpu_metrics_t *prev2 = &model->history[(model->history_index - 3 + HISTORY_SIZE) % HISTORY_SIZE];
    cpu_metrics_t *prev3 = &model->history[(model->history_index - 4 + HISTORY_SIZE) % HISTORY_SIZE];

    int trend1 = (int)current->cpu_util - (int)prev1->cpu_util;
    int trend2 = (int)prev1->cpu_util - (int)prev2->cpu_util;
    int trend3 = (int)prev2->cpu_util - (int)prev3->cpu_util;

    int acceleration = trend1 - trend2;
    int prev_acceleration = trend2 - trend3;

    int prediction = (int)current->cpu_util + 
                     (trend1 * 2 + trend2) / 3 +
                     (acceleration + prev_acceleration) / 4;

    prediction = apply_pattern_adjustment(model, prediction);

    if (prediction < 0)
        prediction = 0;
    if (prediction > 100)
        prediction = 100;

    return (u32)prediction;
}

int apply_pattern_adjustment(prediction_model_t *model, int base_prediction)
{
    if (model->pattern_count == 0)
        return base_prediction;

    cpu_metrics_t *current = &model->history[(model->history_index - 1 + HISTORY_SIZE) % HISTORY_SIZE];
    cpu_metrics_t *prev1 = &model->history[(model->history_index - 2 + HISTORY_SIZE) % HISTORY_SIZE];
    cpu_metrics_t *prev2 = &model->history[(model->history_index - 3 + HISTORY_SIZE) % HISTORY_SIZE];

    u8 signature[16];
    generate_pattern_signature(current, prev1, prev2, signature);

    for (int i = 0; i < model->pattern_count; i++) {
        if (pattern_signature_match(model->patterns[i].metrics_signature, signature)) {
            int pattern_weight = model->patterns[i].frequency;
            int adjustment = model->patterns[i].target_freq - base_prediction;
            return base_prediction + (adjustment * pattern_weight) / 100;
        }
    }
    return base_prediction;
}

void generate_pattern_signature(cpu_metrics_t *current, cpu_metrics_t *prev1, cpu_metrics_t *prev2, u8 *signature)
{
    int util_delta1 = (int)current->cpu_util - (int)prev1->cpu_util;
    int util_delta2 = (int)prev1->cpu_util - (int)prev2->cpu_util;
    int irq_delta1 = (int)current->irq_count - (int)prev1->irq_count;
    int irq_delta2 = (int)prev1->irq_count - (int)prev2->irq_count;
    int iowait_delta1 = (int)current->iowait - (int)prev1->iowait;
    int iowait_delta2 = (int)prev1->iowait - (int)prev2->iowait;
    signature[0] = (u8)((util_delta1 + 100) & 0xFF);
    signature[1] = (u8)((util_delta2 + 100) & 0xFF);
    signature[2] = (u8)((irq_delta1 / 10 + 100) & 0xFF);
    signature[3] = (u8)((irq_delta2 / 10 + 100) & 0xFF);
    signature[4] = (u8)((iowait_delta1 + 100) & 0xFF);
    signature[5] = (u8)((iowait_delta2 + 100) & 0xFF);
    signature[6] = (u8)(current->cpu_util & 0xFF);
    signature[7] = (u8)(current->iowait & 0xFF);
    signature[8] = (u8)((current->runnable_tasks % 256) & 0xFF);
    for (int i = 9; i < 16; i++)
        signature[i] = 0;
}

bool pattern_signature_match(const u8 *sig1, const u8 *sig2)
{
    int distance = 0;
    for (int i = 0; i < 16; i++) {
        u8 diff = sig1[i] ^ sig2[i];
        while (diff) {
            distance += diff & 1;
            diff >>= 1;
        }
    }
    return distance < 8;
}
