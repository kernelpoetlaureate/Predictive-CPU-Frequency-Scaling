// Minimal CPU "shit" predictor
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define HISTORY 5

typedef struct {
    int cpu_util; // How busy was the CPU (0-100)
} cpu_metrics_t;

typedef struct {
    cpu_metrics_t history[HISTORY];
    int history_index;
    int history_count;
} predictor_t;

void predictor_init(predictor_t *p) {
    memset(p, 0, sizeof(*p));
}

void predictor_add(predictor_t *p, int util) {
    p->history[p->history_index].cpu_util = util;
    p->history_index = (p->history_index + 1) % HISTORY;
    if (p->history_count < HISTORY) p->history_count++;
}

// Predict next utilization: just repeat the last value (minimal!)
int predictor_predict(predictor_t *p) {
    if (p->history_count == 0) return 50;
    int last = (p->history_index - 1 + HISTORY) % HISTORY;
    return p->history[last].cpu_util;
}

// Decide frequency: 1GHz for <=50%, 2GHz for >50%
int predictor_freq(int predicted_util) {
    return predicted_util > 50 ? 2000 : 1000;
}

int main() {
    predictor_t p;
    predictor_init(&p);
    int samples[] = {20, 30, 40, 60, 80, 90, 30, 10};
    int n = sizeof(samples)/sizeof(samples[0]);
    for (int i = 0; i < n; i++) {
        predictor_add(&p, samples[i]);
        int pred = predictor_predict(&p);
        int freq = predictor_freq(pred);
        printf("History: ");
        for (int j = 0; j < p.history_count; j++)
            printf("%d ", p.history[j].cpu_util);
        printf("\nPredicted Util: %d%%, Set Freq: %d MHz\n\n", pred, freq);
    }
    return 0;
}
