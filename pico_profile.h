#pragma once

#include <cstddef>
#include <cstdint>

enum pico_profile_api : uint8_t {
    PICO_PROFILE_MGET,
    PICO_PROFILE_BAND,
    PICO_PROFILE_LSHR,
    PICO_PROFILE_MIN,
    PICO_PROFILE_MAX,
    PICO_PROFILE_FLR,
    PICO_PROFILE_NEXT,
    PICO_PROFILE_PAL,
    PICO_PROFILE_API_COUNT
};

#ifdef PICO_VM_OPCODE_PROFILE
void pico_vm_profile_start(uint32_t frames);
void pico_vm_profile_end_frame();
#else
inline void pico_vm_profile_start(uint32_t) {}
inline void pico_vm_profile_end_frame() {}
#endif

#ifdef PICO_PERF_PROFILE
#include "esp_cpu.h"

inline uint64_t pico_profile_calls[PICO_PROFILE_API_COUNT] = {};
inline uint64_t pico_profile_cycles[PICO_PROFILE_API_COUNT] = {};
inline uint64_t pico_profile_c_calls = 0;
inline uint64_t pico_profile_c_cycles = 0;

class pico_profile_scope {
public:
    explicit pico_profile_scope(pico_profile_api api)
        : api_(api), start_(esp_cpu_get_cycle_count()) {}

    ~pico_profile_scope() {
        ++pico_profile_calls[api_];
        pico_profile_cycles[api_] += esp_cpu_get_cycle_count() - start_;
    }

private:
    pico_profile_api api_;
    uint32_t start_;
};

class pico_profile_c_call_scope {
public:
    pico_profile_c_call_scope() : start_(esp_cpu_get_cycle_count()) {}

    ~pico_profile_c_call_scope() {
        ++pico_profile_c_calls;
        pico_profile_c_cycles += esp_cpu_get_cycle_count() - start_;
    }

private:
    uint32_t start_;
};

inline void pico_profile_take(pico_profile_api api, uint64_t* calls,
                              uint64_t* cycles) {
    *calls = pico_profile_calls[api];
    *cycles = pico_profile_cycles[api];
    pico_profile_calls[api] = 0;
    pico_profile_cycles[api] = 0;
}

inline void pico_profile_take_c_calls(uint64_t* calls, uint64_t* cycles) {
    *calls = pico_profile_c_calls;
    *cycles = pico_profile_c_cycles;
    pico_profile_c_calls = 0;
    pico_profile_c_cycles = 0;
}

inline void pico_profile_reset() {
    for (size_t api = 0; api < PICO_PROFILE_API_COUNT; ++api) {
        pico_profile_calls[api] = 0;
        pico_profile_cycles[api] = 0;
    }
    pico_profile_c_calls = 0;
    pico_profile_c_cycles = 0;
}

#define PICO_PROFILE_SCOPE(api) pico_profile_scope pico_profile_scope_##api(api)
#define PICO_PROFILE_C_CALL_SCOPE() pico_profile_c_call_scope pico_profile_c_call_scope_instance
#else
inline void pico_profile_take(pico_profile_api, uint64_t* calls,
                              uint64_t* cycles) {
    *calls = 0;
    *cycles = 0;
}

inline void pico_profile_take_c_calls(uint64_t* calls, uint64_t* cycles) {
    *calls = 0;
    *cycles = 0;
}

inline void pico_profile_reset() {}

#define PICO_PROFILE_SCOPE(api) ((void)0)
#define PICO_PROFILE_C_CALL_SCOPE() ((void)0)
#endif
