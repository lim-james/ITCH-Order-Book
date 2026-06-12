#pragma once 

#include <cstdint>
#include <fstream>
#include <string>

inline std::uint64_t rdtsc() noexcept {
    std::uint64_t lo, hi;
    __asm__ volatile ("lfence\nrdtsc" : "=a"(lo), "=d"(hi) :: "memory");
    return (hi << 32) | lo;
}

inline double tsc_ghz() {
    std::ifstream f("/proc/cpuinfo");
    std::string line;
    while (std::getline(f, line))
        if (line.starts_with("cpu MHz"))
            return std::stod(line.substr(line.find(':') + 1)) / 1000.0;
    return 3.0; 
}
