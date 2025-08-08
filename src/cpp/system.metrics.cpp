#include "system.metrics.h"
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#ifdef __linux__
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <sys/sysctl.h>
#endif

namespace TernaryFission {
namespace {
#ifdef __linux__
std::pair<uint64_t, uint64_t> readProcStat() {
    std::ifstream file("/proc/stat");
    std::string cpu;
    uint64_t user = 0, nice = 0, system = 0, idle = 0;
    uint64_t iowait = 0, irq = 0, softirq = 0, steal = 0;
    if (file) {
        file >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    }
    uint64_t idleAll = idle + iowait;
    uint64_t nonIdle = user + nice + system + irq + softirq + steal;
    uint64_t total = idleAll + nonIdle;
    return {total, idleAll};
}
#elif defined(__APPLE__)
std::pair<uint64_t, uint64_t> readHostCPU() {
    host_cpu_load_info_data_t info;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&info, &count) != KERN_SUCCESS) {
        return {0, 0};
    }
    uint64_t user = info.cpu_ticks[CPU_STATE_USER];
    uint64_t nice = info.cpu_ticks[CPU_STATE_NICE];
    uint64_t system = info.cpu_ticks[CPU_STATE_SYSTEM];
    uint64_t idle = info.cpu_ticks[CPU_STATE_IDLE];
    uint64_t total = user + nice + system + idle;
    return {total, idle};
}
#endif
} // anonymous namespace

double getCPUUsagePercent() {
#ifdef __linux__
    auto [total1, idle1] = readProcStat();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto [total2, idle2] = readProcStat();
    uint64_t totald = total2 - total1;
    uint64_t idled = idle2 - idle1;
    if (totald == 0) return 0.0;
    return (static_cast<double>(totald - idled) * 100.0) / totald;
#elif defined(__APPLE__)
    auto [total1, idle1] = readHostCPU();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto [total2, idle2] = readHostCPU();
    uint64_t totald = total2 - total1;
    uint64_t idled = idle2 - idle1;
    if (totald == 0) return 0.0;
    return (static_cast<double>(totald - idled) * 100.0) / totald;
#else
    return 0.0;
#endif
}

MemoryUsage getMemoryUsage() {
    MemoryUsage usage{0.0, 0};
#ifdef __linux__
    std::ifstream status("/proc/self/status");
    std::string line;
    uint64_t rss_kb = 0, hwm_kb = 0;
    while (std::getline(status, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            std::istringstream iss(line.substr(6));
            iss >> rss_kb;
        } else if (line.rfind("VmHWM:", 0) == 0) {
            std::istringstream iss(line.substr(6));
            iss >> hwm_kb;
        }
    }
    uint64_t rss_bytes = rss_kb * 1024;
    uint64_t hwm_bytes = hwm_kb * 1024;
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    uint64_t total = (pages > 0 && page_size > 0) ? static_cast<uint64_t>(pages) * static_cast<uint64_t>(page_size) : 0;
    double percent = total ? (static_cast<double>(rss_bytes) * 100.0) / total : 0.0;
    usage.percent = percent;
    usage.peak_bytes = hwm_bytes;
#elif defined(__APPLE__)
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS) {
        uint64_t rss = info.resident_size;
        uint64_t peak = info.resident_size_max;
        uint64_t phys_mem = 0;
        size_t size = sizeof(phys_mem);
        sysctlbyname("hw.memsize", &phys_mem, &size, NULL, 0);
        double percent = phys_mem ? (static_cast<double>(rss) * 100.0) / phys_mem : 0.0;
        usage.percent = percent;
        usage.peak_bytes = peak;
    }
#endif
    return usage;
}

} // namespace TernaryFission
