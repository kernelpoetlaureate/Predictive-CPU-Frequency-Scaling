#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/timekeeping.h>
#include "predictive_model.h"
#include "metric_collector.h"
#include "pattern_recognizer.h"

// Per-CPU governor state
typedef struct {
    prediction_model_t model;
    u32 target_freq;
    u32 last_freq;
    u64 last_sample_time;
    struct delayed_work work;
    struct cpufreq_policy *policy;
    spinlock_t lock;
} predictive_cpu_data_t;

// Global governor state
typedef struct {
    predictive_cpu_data_t **cpu_data;
    u32 enabled;
    u32 sample_rate_ms;
    u32 min_freq_change_threshold;
} predictive_governor_t;

static predictive_governor_t pred_gov;

static void predictive_sampling_work(struct work_struct *work);
static int predictive_cpufreq_init(struct cpufreq_policy *policy);
static void predictive_cpufreq_exit(struct cpufreq_policy *policy);
static int predictive_cpufreq_start(struct cpufreq_policy *policy);
static void predictive_cpufreq_stop(struct cpufreq_policy *policy);
static void predictive_cpufreq_limits(struct cpufreq_policy *policy);
u32 calculate_target_frequency(struct cpufreq_policy *policy, u32 predicted_util, prediction_model_t *model);

static int predictive_cpufreq_init(struct cpufreq_policy *policy)
{
    predictive_cpu_data_t *cpu_data;
    int cpu = policy->cpu;
    cpu_data = kzalloc(sizeof(*cpu_data), GFP_KERNEL);
    if (!cpu_data)
        return -ENOMEM;
    spin_lock_init(&cpu_data->lock);
    cpu_data->policy = policy;
    cpu_data->last_freq = policy->cur;
    cpu_data->target_freq = policy->cur;
    cpu_data->last_sample_time = ktime_get_ns();
    init_prediction_model(&cpu_data->model);
    if (!pred_gov.cpu_data)
        return -EINVAL;
    pred_gov.cpu_data[cpu] = cpu_data;
    INIT_DEFERRABLE_WORK(&cpu_data->work, predictive_sampling_work);
    mod_delayed_work(system_wq, &cpu_data->work, msecs_to_jiffies(pred_gov.sample_rate_ms));
    return 0;
}

static void predictive_cpufreq_exit(struct cpufreq_policy *policy)
{
    predictive_cpu_data_t *cpu_data = pred_gov.cpu_data[policy->cpu];
    if (cpu_data) {
        cancel_delayed_work_sync(&cpu_data->work);
        kfree(cpu_data);
        pred_gov.cpu_data[policy->cpu] = NULL;
    }
}

static int predictive_cpufreq_start(struct cpufreq_policy *policy)
{
    predictive_cpu_data_t *cpu_data = pred_gov.cpu_data[policy->cpu];
    if (!cpu_data)
        return -EINVAL;
    mod_delayed_work(system_wq, &cpu_data->work, msecs_to_jiffies(pred_gov.sample_rate_ms));
    return 0;
}

static void predictive_cpufreq_stop(struct cpufreq_policy *policy)
{
    predictive_cpu_data_t *cpu_data = pred_gov.cpu_data[policy->cpu];
    if (cpu_data) {
        cancel_delayed_work_sync(&cpu_data->work);
    }
}

static void predictive_cpufreq_limits(struct cpufreq_policy *policy)
{
    predictive_cpu_data_t *cpu_data = pred_gov.cpu_data[policy->cpu];
    if (cpu_data) {
        if (policy->cur < policy->min)
            __cpufreq_driver_target(policy, policy->min, CPUFREQ_RELATION_L);
        else if (policy->cur > policy->max)
            __cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
    }
}

static void predictive_sampling_work(struct work_struct *work)
{
    predictive_cpu_data_t *cpu_data = container_of(work, predictive_cpu_data_t, work.work);
    struct cpufreq_policy *policy = cpu_data->policy;
    cpu_metrics_t metrics;
    u32 predicted_util, target_freq;
    unsigned long flags;
    collect_cpu_metrics(policy->cpu, &metrics);
    spin_lock_irqsave(&cpu_data->lock, flags);
    add_metrics_to_history(&cpu_data->model, &metrics);
    predicted_util = predict_cpu_utilization(&cpu_data->model);
    target_freq = calculate_target_frequency(policy, predicted_util, &cpu_data->model);
    if (abs(target_freq - cpu_data->last_freq) > pred_gov.min_freq_change_threshold) {
        cpu_data->target_freq = target_freq;
        __cpufreq_driver_target(policy, target_freq, CPUFREQ_RELATION_L);
        cpu_data->last_freq = target_freq;
    }
    if (cpu_data->model.predictions_made % 100 == 0) {
        update_patterns(&cpu_data->model);
    }
    spin_unlock_irqrestore(&cpu_data->lock, flags);
    mod_delayed_work(system_wq, &cpu_data->work, msecs_to_jiffies(pred_gov.sample_rate_ms));
}

u32 calculate_target_frequency(struct cpufreq_policy *policy, u32 predicted_util, prediction_model_t *model)
{
    u32 min_freq = policy->cpuinfo.min_freq;
    u32 max_freq = policy->cpuinfo.max_freq;
    u32 range = max_freq - min_freq;
    u32 adjusted_util = predicted_util;
    if (model->aggressiveness > 50) {
        adjusted_util = predicted_util + ((model->aggressiveness - 50) * predicted_util) / 100;
    } else if (model->aggressiveness < 50) {
        adjusted_util = predicted_util * model->aggressiveness / 50;
    }
    if (adjusted_util > 100)
        adjusted_util = 100;
    u32 target_freq = min_freq + (range * adjusted_util) / 100;
    return cpufreq_frequency_table_target(policy, policy->freq_table, target_freq, CPUFREQ_RELATION_L);
}

static struct cpufreq_governor predictive_governor = {
    .name = "predictive",
    .owner = THIS_MODULE,
    .init = predictive_cpufreq_init,
    .exit = predictive_cpufreq_exit,
    .start = predictive_cpufreq_start,
    .stop = predictive_cpufreq_stop,
    .limits = predictive_cpufreq_limits,
};

static int __init predictive_governor_init(void)
{
    int ret;
    pred_gov.cpu_data = kzalloc(sizeof(predictive_cpu_data_t *) * num_possible_cpus(), GFP_KERNEL);
    if (!pred_gov.cpu_data)
        return -ENOMEM;
    pred_gov.enabled = 0;
    pred_gov.sample_rate_ms = 50;
    pred_gov.min_freq_change_threshold = 100000;
    ret = cpufreq_register_governor(&predictive_governor);
    if (ret) {
        kfree(pred_gov.cpu_data);
        return ret;
    }
    return 0;
}

static void __exit predictive_governor_exit(void)
{
    cpufreq_unregister_governor(&predictive_governor);
    kfree(pred_gov.cpu_data);
}

module_init(predictive_governor_init);
module_exit(predictive_governor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Giorgi Tchankvetadze");
MODULE_DESCRIPTION("Predictive CPU Frequency Scaling Governor");
