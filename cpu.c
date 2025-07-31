/* cpu.c - Minimal CPU detection implementation */
#include "cpu.h"

struct cpuinfo_x86 boot_cpu_data;

/* Basic CPUID wrapper */
unsigned int cpuid(unsigned int op, unsigned int *eax, unsigned int *ebx, 
                   unsigned int *ecx, unsigned int *edx) {
    unsigned int function = op;
    
    __asm__ __volatile__(
        "cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "0" (function)
    );
    
    return *eax;
}

/* Detect CPU vendor */
static void get_cpu_vendor(void) {
    unsigned int eax, ebx, ecx, edx;
    char vendor_string[13];
    
    cpuid(0, &eax, &ebx, &ecx, &edx);
    
    /* Get vendor string */
    vendor_string[0] = ebx & 0xff;
    vendor_string[1] = (ebx >> 8) & 0xff;
    vendor_string[2] = (ebx >> 16) & 0xff;
    vendor_string[3] = (ebx >> 24) & 0xff;
    vendor_string[4] = edx & 0xff;
    vendor_string[5] = (edx >> 8) & 0xff;
    vendor_string[6] = (edx >> 16) & 0xff;
    vendor_string[7] = (edx >> 24) & 0xff;
    vendor_string[8] = ecx & 0xff;
    vendor_string[9] = (ecx >> 8) & 0xff;
    vendor_string[10] = (ecx >> 16) & 0xff;
    vendor_string[11] = (ecx >> 24) & 0xff;
    vendor_string[12] = '\0';
    
    /* Identify vendor */
    if (strcmp(vendor_string, "GenuineIntel") == 0) {
        boot_cpu_data.vendor = CPU_VENDOR_INTEL;
    } else if (strcmp(vendor_string, "AuthenticAMD") == 0) {
        boot_cpu_data.vendor = CPU_VENDOR_AMD;
    } else {
        boot_cpu_data.vendor = CPU_VENDOR_UNKNOWN;
    }
}

/* Get CPU family/model/stepping */
static void get_cpu_signature(void) {
    unsigned int eax, ebx, ecx, edx;
    
    cpuid(1, &eax, &ebx, &ecx, &edx);
    
    boot_cpu_data.family = (eax >> 8) & 0xf;
    boot_cpu_data.model = (eax >> 4) & 0xf;
    boot_cpu_data.stepping = eax & 0xf;
    
    /* Handle extended family/model */
    if (boot_cpu_data.family == 0xf) {
        boot_cpu_data.family += (eax >> 20) & 0xff;
        boot_cpu_data.model += ((eax >> 16) & 0xf) << 4;
    }
}

/* Get basic features */
static void get_cpu_features(void) {
    unsigned int eax, ebx, ecx, edx;
    
    cpuid(1, &eax, &ebx, &ecx, &edx);
    
    /* Check basic features */
    if (edx & (1 << 0)) boot_cpu_data.features |= CPU_FEATURE_FPU;
    if (edx & (1 << 23)) boot_cpu_data.features |= CPU_FEATURE_MMX;
    if (edx & (1 << 25)) boot_cpu_data.features |= CPU_FEATURE_SSE;
    if (edx & (1 << 26)) boot_cpu_data.features |= CPU_FEATURE_SSE2;
}

/* Main CPU detection function */
void cpu_detect(void) {
    get_cpu_vendor();
    get_cpu_signature();
    get_cpu_features();
    
    /* Set default values for what we can't detect */
    boot_cpu_data.mhz = 1000;        /* Default to 1GHz */
    boot_cpu_data.l1_cache_size = 32;  /* Default 32KB L1 */
    boot_cpu_data.l2_cache_size = 256; /* Default 256KB L2 */
    boot_cpu_data.cores = 1;         /* Default single core */
    boot_cpu_data.threads = 1;       /* Default single thread */
}

/* Initialize CPU */
void cpu_init(void) {
    /* Set up control registers */
    __asm__ __volatile__(
        "mov %%cr0, %%rax\n"
        "or $0x10, %%rax\n"          /* Set WP (Write Protect) bit */
        "mov %%rax, %%cr0\n"
        : : : "rax"
    );
    
    /* Enable cache */
    __asm__ __volatile__(
        "mov %%cr4, %%rax\n"
        "or $0x80, %%rax\n"          /* Set PGE (Page Global Enable) */
        "mov %%rax, %%cr4\n"
        : : : "rax"
    );
}

void print_cpu_info(void) {
    const char *vendor_str;
    
    switch (boot_cpu_data.vendor) {
        case CPU_VENDOR_INTEL: vendor_str = "Intel"; break;
        case CPU_VENDOR_AMD: vendor_str = "AMD"; break;
        default: vendor_str = "Unknown"; break;
    }
    
    printf("CPU: %s Family %d Model %d Stepping %d\n",
           vendor_str, boot_cpu_data.family, 
           boot_cpu_data.model, boot_cpu_data.stepping);
    printf("Clock: %d MHz\n", boot_cpu_data.mhz);
    printf("Cache: L1 %dKB, L2 %dKB\n", 
           boot_cpu_data.l1_cache_size, boot_cpu_data.l2_cache_size);
    printf("Cores: %d, Threads: %d\n", 
           boot_cpu_data.cores, boot_cpu_data.threads);
}

int main(void) {
    cpu_detect();
    print_cpu_info();
    return 0;
}
