
#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// CPU vendor enums
#define CPU_VENDOR_UNKNOWN 0
#define CPU_VENDOR_INTEL   1
#define CPU_VENDOR_AMD     2

// CPU feature flags
#define CPU_FEATURE_FPU    (1 << 0)
#define CPU_FEATURE_MMX    (1 << 23)
#define CPU_FEATURE_SSE    (1 << 25)
#define CPU_FEATURE_SSE2   (1 << 26)

struct cpuinfo_x86 {
    int vendor;
    int family;
    int model;
    int stepping;
    int features;
    int mhz;
    int l1_cache_size;
    int l2_cache_size;
    int cores;
    int threads;
};

extern struct cpuinfo_x86 boot_cpu_data;

void cpu_detect(void);
void cpu_init(void);
void print_cpu_info(void);

#endif // CPU_H
