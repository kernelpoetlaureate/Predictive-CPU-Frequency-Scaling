// cpu.h
// CPU module: Simulated Process Control Block (PCB) for a real CPU
#ifndef CPU_H
#define CPU_H

#include <stdint.h>

// Simulated CPU PCB structure
typedef struct {
    uint32_t id;            // CPU/core ID
    double usage;           // Current usage (0.0 - 1.0)
    int freq;               // Current frequency (kHz)
    int predicted_freq;     // Predicted frequency (kHz)
    double usage_history[10]; // Usage history for prediction
    int history_idx;        // Index for usage history
} cpu_pcb_t;

// Initialize CPU PCB
void cpu_init(cpu_pcb_t *cpu, uint32_t id);

// Update usage and frequency
void cpu_update(cpu_pcb_t *cpu, double usage, int freq);

// Predict next frequency
int cpu_predict_freq(cpu_pcb_t *cpu);

#endif // CPU_H
