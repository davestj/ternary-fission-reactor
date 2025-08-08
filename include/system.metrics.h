#ifndef SYSTEM_METRICS_H
#define SYSTEM_METRICS_H

#include <cstdint>

namespace TernaryFission {

struct MemoryUsage {
    double percent;
    uint64_t peak_bytes;
};

// We sample CPU usage over a short interval and return percentage [0,100]
double getCPUUsagePercent();

// We retrieve current memory usage percent and peak resident set size
MemoryUsage getMemoryUsage();

} // namespace TernaryFission

#endif // SYSTEM_METRICS_H
