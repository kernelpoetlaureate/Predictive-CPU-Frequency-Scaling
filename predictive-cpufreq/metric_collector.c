#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <linux/timekeeping.h>
#include <linux/slab.h>
#include "metric_collector.h"
#include "predictive_model.h"

extern struct predictive_governor_t pred_gov;

void collect_cpu_metrics(int cpu, cpu_metrics_t *metrics)
{
    u64 now = ktime_get_ns();
    struct kernel_cpustat *kstat = &kcpustat_cpu(cpu);
    u64 user, nice, system, idle, iowait, irq, softirq, steal;
    u64 total;

    metrics->timestamp = now;
    metrics->freq = pred_gov.cpu_data[cpu]->policy->cur;

    user = kstat->cpustat[CPUTIME_USER];
    nice = kstat->cpustat[CPUTIME_NICE];
    system = kstat->cpustat[CPUTIME_SYSTEM];
    idle = kstat->cpustat[CPUTIME_IDLE];
    iowait = kstat->cpustat[CPUTIME_IOWAIT];
    irq = kstat->cpustat[CPUTIME_IRQ];
    softirq = kstat->cpustat[CPUTIME_SOFTIRQ];
    steal = kstat->cpustat[CPUTIME_STEAL];

    total = user + nice + system + idle + iowait + irq + softirq + steal;
    if (total > 0) {
        metrics->cpu_util = (u32)((100 * (total - idle)) / total);
        metrics->iowait = (u32)((100 * iowait) / total);
    } else {
        metrics->cpu_util = 0;
        metrics->iowait = 0;
    }

    metrics->irq_count = kstat_irqs_cpu(0, cpu); // Simplified for example
    metrics->runnable_tasks = nr_running();
    metrics->idle_time = idle - pred_gov.cpu_data[cpu]->model.history[(pred_gov.cpu_data[cpu]->model.history_index - 1 + HISTORY_SIZE) % HISTORY_SIZE].idle_time;
    pred_gov.cpu_data[cpu]->last_sample_time = now;
}
