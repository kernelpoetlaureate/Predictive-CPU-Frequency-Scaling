#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include "../predictive_model.h"
#include "../metric_collector.h"

static void test_prediction_accuracy(void)
{
    prediction_model_t model;
    cpu_metrics_t metrics;
    int errors = 0;
    int tests = 100;
    init_prediction_model(&model);
    for (int i = 0; i < tests; i++) {
        metrics.timestamp = 0;
        metrics.cpu_util = i < 50 ? (i * 2) : ((100 - i) * 2);
        metrics.freq = 1000000 + (metrics.cpu_util * 15000);
        metrics.irq_count = i * 10;
        metrics.process_switches = i * 5;
        metrics.idle_time = 1000000 - (metrics.cpu_util * 10000);
        metrics.iowait = metrics.cpu_util / 10;
        metrics.runnable_tasks = metrics.cpu_util / 20;
        add_metrics_to_history(&model, &metrics);
        if (model.history_count > 5) {
            u32 predicted = predict_cpu_utilization(&model);
            u32 expected = i < 45 ? ((i + 5) * 2) : ((95 - i) * 2);
            if (abs(predicted - expected) > 20) {
                printk(KERN_ERR "Prediction error: expected %d, got %d\n", expected, predicted);
                errors++;
            }
        }
    }
    printk(KERN_INFO "Prediction accuracy test: %d errors out of %d tests\n", errors, tests);
}

static void test_pattern_recognition(void)
{
    prediction_model_t model;
    cpu_metrics_t metrics[10];
    init_prediction_model(&model);
    for (int i = 0; i < 10; i++) {
        metrics[i].timestamp = 0;
        metrics[i].cpu_util = (i % 5) * 20;
        metrics[i].freq = 1000000 + (metrics[i].cpu_util * 15000);
        metrics[i].irq_count = i * 10;
        metrics[i].process_switches = i * 5;
        metrics[i].idle_time = 1000000 - (metrics[i].cpu_util * 10000);
        metrics[i].iowait = metrics[i].cpu_util / 10;
        metrics[i].runnable_tasks = metrics[i].cpu_util / 20;
        add_metrics_to_history(&model, &metrics[i]);
    }
    update_patterns(&model);
    if (model.pattern_count > 0) {
        printk(KERN_INFO "Pattern recognition test: detected %d patterns\n", model.pattern_count);
    } else {
        printk(KERN_ERR "Pattern recognition test: no patterns detected\n");
    }
}

static int __init test_framework_init(void)
{
    printk(KERN_INFO "Starting predictive governor tests\n");
    test_prediction_accuracy();
    test_pattern_recognition();
    return 0;
}

static void __exit test_framework_exit(void)
{
    printk(KERN_INFO "Predictive governor tests completed\n");
}

module_init(test_framework_init);
module_exit(test_framework_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Giorgi Tchankvetadze");
MODULE_DESCRIPTION("Test framework for predictive CPU frequency scaling");
