# Predictive CPU Frequency Scaling

This project demonstrates predictive CPU frequency scaling for RISC/Linux systems. Instead of reacting to workload changes, it uses a simple predictive model to anticipate CPU demand and proactively adjust the CPU frequency.

## Features
- Collects CPU usage data from `/proc/stat`
- Predicts future workload using a moving average
- Adjusts CPU frequency via the Linux CPUFreq interface
- Designed for RISC architectures on Linux

## Usage
1. **Build:**
   ```sh
   gcc predictive_cpu_freq.c -o predictive_cpu_freq
   ```
2. **Run as root:**
   ```sh
   sudo ./predictive_cpu_freq
   ```
   (Root privileges are required to write to CPU frequency sysfs.)

## How it Works
- The program samples CPU usage at regular intervals.
- It maintains a history of recent usage and predicts the next interval's load.
- Based on the prediction, it sets the CPU frequency by writing to `/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed`.

## Notes
- The predictive model is a simple moving average; you can replace it with a more advanced algorithm.
- The code is a skeleton: you must implement the actual CPU usage calculation in `get_cpu_usage()`.
- The program currently only sets the frequency for CPU0. Extend as needed for multi-core systems.

## License
See LICENSE for details.
