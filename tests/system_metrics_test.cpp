#include "system.metrics.h"
#include <thread>
#include <cassert>
#include <iostream>
#include <chrono>

using namespace TernaryFission;

int main() {
    // We create CPU load to ensure non-zero usage
    std::thread busy([](){
        volatile uint64_t x = 0;
        auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
        while (std::chrono::steady_clock::now() < end) { x++; }
    });
    double cpu = getCPUUsagePercent();
    busy.join();
    if (cpu <= 0.0) {
        std::cerr << "CPU usage percent should be > 0, got " << cpu << std::endl;
        return 1;
    }
    MemoryUsage mem = getMemoryUsage();
    if (mem.percent <= 0.0 || mem.peak_bytes == 0) {
        std::cerr << "Memory metrics not collected" << std::endl;
        return 1;
    }
    std::cout << "cpu=" << cpu << " mem%=" << mem.percent << " peak=" << mem.peak_bytes << std::endl;
    return 0;
}
