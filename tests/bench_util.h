#pragma once
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

inline double to_us(std::chrono::nanoseconds ns) {
    return std::chrono::duration<double, std::micro>(ns).count();
}

inline void print_latency_report(const char* label, std::chrono::nanoseconds total,
        double ops_per_sec, std::vector<uint64_t>& samples_ns) {
    (void)samples_ns;
    std::cout << label << ": " << to_us(total) << " us, "
              << ops_per_sec << " ops/sec"
              << "\n";
}
