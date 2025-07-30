#include <linux/string.h>
#include "predictive_model.h"
#include "pattern_recognizer.h"

void update_patterns(prediction_model_t *model)
{
    if (model->history_count < 10)
        return;
    for (int i = 3; i < model->history_count && i < 20; i++) {
        int idx = (model->history_index - i + HISTORY_SIZE) % HISTORY_SIZE;
        cpu_metrics_t *current = &model->history[idx];
        cpu_metrics_t *prev1 = &model->history[(idx - 1 + HISTORY_SIZE) % HISTORY_SIZE];
        cpu_metrics_t *prev2 = &model->history[(idx - 2 + HISTORY_SIZE) % HISTORY_SIZE];
        u8 signature[16];
        generate_pattern_signature(current, prev1, prev2, signature);
        bool found = false;
        for (int j = 0; j < model->pattern_count; j++) {
            if (pattern_signature_match(model->patterns[j].metrics_signature, signature)) {
                model->patterns[j].frequency = (model->patterns[j].frequency * 95 + 5) / 100;
                model->patterns[j].avg_cpu_util = (model->patterns[j].avg_cpu_util * 90 + current->cpu_util * 10) / 100;
                model->patterns[j].target_freq = (model->patterns[j].target_freq * 90 + current->freq * 10) / 100;
                found = true;
                break;
            }
        }
        if (!found && model->pattern_count < MAX_PATTERNS) {
            struct workload_pattern *pattern = &model->patterns[model->pattern_count];
            pattern->pattern_id = model->pattern_count;
            pattern->frequency = 100;
            pattern->duration = 0;
            pattern->avg_cpu_util = current->cpu_util;
            pattern->target_freq = current->freq;
            memcpy(pattern->metrics_signature, signature, sizeof(signature));
            model->pattern_count++;
        }
    }
    int new_count = 0;
    for (int i = 0; i < model->pattern_count; i++) {
        if (model->patterns[i].frequency > 10) {
            if (new_count != i) {
                model->patterns[new_count] = model->patterns[i];
            }
            new_count++;
        }
    }
    model->pattern_count = new_count;
}
