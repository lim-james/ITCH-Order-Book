#pragma once 

#include <pthread.h>
#include <sched.h>

inline void pin_to_core(int core) noexcept {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}
