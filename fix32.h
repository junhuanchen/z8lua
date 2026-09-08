//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2020 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#if defined(__has_include)
#if __has_include("board_config.h")
#include "board_config.h"
#endif
#endif

#include <stdint.h>    // int32_t, int64_t, …
#include <cmath>       // std::abs
#include <algorithm>   // std::min
#include <type_traits> // std::enable_if
#ifdef PICO_FIX32_PROFILE
#include "esp_cpu.h"
#endif

namespace z8
{

#ifdef PICO_FIX32_PROFILE
struct fix32_div_cache_entry
{
    int32_t numerator;
    int32_t denominator;
    int32_t result;
    bool valid;
};

inline fix32_div_cache_entry fix32_div_cache[256] = {};
inline uint64_t fix32_div_count = 0;
inline uint64_t fix32_div_cache_hits = 0;
inline uint64_t fix32_div_integer_fast = 0;
inline uint64_t fix32_div_cycles = 0;

inline uint64_t fix32_take_div_count()
{
    const uint64_t result = fix32_div_count;
    fix32_div_count = 0;
    return result;
}

inline uint64_t fix32_take_div_cache_hits()
{
    const uint64_t result = fix32_div_cache_hits;
    fix32_div_cache_hits = 0;
    return result;
}

inline uint64_t fix32_take_div_integer_fast()
{
    const uint64_t result = fix32_div_integer_fast;
    fix32_div_integer_fast = 0;
    return result;
}

inline uint64_t fix32_take_div_cycles()
{
    const uint64_t result = fix32_div_cycles;
    fix32_div_cycles = 0;
    return result;
}
#endif

struct fix32
{
    inline fix32() = default;

    // Convert from/to double
    inline fix32(double d)
      : m_bits(int32_t(int64_t(d * 65536.0)))
    {}

    inline operator double() const
    {
        return double(m_bits) * (1.0 / 65536.0);
    }

    // Conversions up to int16_t are safe.
    inline fix32(int8_t x)  : m_bits(int32_t(x << 16)) {}
    inline fix32(uint8_t x) : m_bits(int32_t(x << 16)) {}
    inline fix32(int16_t x) : m_bits(int32_t(x << 16)) {}

    // Anything above int16_t is risky because of precision loss, but Lua
    // does too many implicit conversions from int that we can’t mark this
    // one as explicit.
    inline fix32(int32_t x)  : m_bits(int32_t(x << 16)) {}

    inline explicit fix32(uint16_t x) : m_bits(int32_t(x << 16)) {}
    inline explicit fix32(uint32_t x) : m_bits(int32_t(x << 16)) {}
    inline explicit fix32(int64_t x)  : m_bits(int32_t(x << 16)) {}
    inline explicit fix32(uint64_t x) : m_bits(int32_t(x << 16)) {}

    // Support for long and unsigned long when it is a distinct
    // type from the standard int*_t types, e.g. on Windows.
    template<typename T,
             typename std::enable_if<(std::is_same<T, long>::value ||
                                      std::is_same<T, unsigned long>::value) &&
                                     !std::is_same<T, int32_t>::value &&
                                     !std::is_same<T, uint32_t>::value &&
                                     !std::is_same<T, int64_t>::value &&
                                     !std::is_same<T, uint64_t>::value>::type *...>
    inline explicit fix32(T x) : m_bits(int32_t(x << 16)) {}

    // Explicit casts are all allowed
    inline explicit operator int8_t()   const { return m_bits >> 16; }
    inline explicit operator uint8_t()  const { return m_bits >> 16; }
    inline explicit operator int16_t()  const { return m_bits >> 16; }
    inline explicit operator uint16_t() const { return m_bits >> 16; }
    inline explicit operator int32_t()  const { return m_bits >> 16; }
    inline explicit operator uint32_t() const { return m_bits >> 16; }
    inline explicit operator int64_t()  const { return m_bits >> 16; }
    inline explicit operator uint64_t() const { return m_bits >> 16; }

    // Additional casts for long and unsigned long on architectures where
    // these are not the same types as their cstdint equivalents.
    template<typename T,
             typename std::enable_if<(std::is_same<T, long>::value ||
                                      std::is_same<T, unsigned long>::value) &&
                                     !std::is_same<T, int32_t>::value &&
                                     !std::is_same<T, uint32_t>::value &&
                                     !std::is_same<T, int64_t>::value &&
                                     !std::is_same<T, uint64_t>::value>::type *...>
    inline explicit operator T() const { return T(m_bits >> 16); }

    // Directly initialise bits
    static inline fix32 frombits(int32_t x)
    {
        fix32 ret; ret.m_bits = x; return ret;
    }

    inline int32_t bits() const { return m_bits; }

    // Comparisons
    inline explicit operator bool() const { return bool(m_bits); }
    inline bool operator ==(fix32 x) const { return m_bits == x.m_bits; }
    inline bool operator !=(fix32 x) const { return m_bits != x.m_bits; }
    inline bool operator  <(fix32 x) const { return m_bits  < x.m_bits; }
    inline bool operator  >(fix32 x) const { return m_bits  > x.m_bits; }
    inline bool operator <=(fix32 x) const { return m_bits <= x.m_bits; }
    inline bool operator >=(fix32 x) const { return m_bits >= x.m_bits; }

    // Increments
    inline fix32& operator ++() { m_bits += 0x10000; return *this; }
    inline fix32& operator --() { m_bits -= 0x10000; return *this; }
    inline fix32 operator ++(int) { fix32 ret = *this; ++*this; return ret; }
    inline fix32 operator --(int) { fix32 ret = *this; --*this; return ret; }

    // Math operations
    inline fix32 const &operator +() const { return *this; }
    inline fix32 operator -() const { return frombits(-m_bits); }
    inline fix32 operator ~() const { return frombits(~m_bits); }

    inline fix32 operator +(fix32 x) const { return frombits(m_bits + x.m_bits); }
    inline fix32 operator -(fix32 x) const { return frombits(m_bits - x.m_bits); }
    inline fix32 operator &(fix32 x) const { return frombits(m_bits & x.m_bits); }
    inline fix32 operator |(fix32 x) const { return frombits(m_bits | x.m_bits); }
    inline fix32 operator ^(fix32 x) const { return frombits(m_bits ^ x.m_bits); }

    fix32 operator *(fix32 x) const
    {
        if ((x.m_bits & 0xffff) == 0)
            return frombits(int64_t(m_bits) * (x.m_bits >> 16));
        return frombits(int64_t(m_bits) * x.m_bits >> 16);
    }

    fix32 operator /(fix32 x) const
    {
#ifdef PICO_FIX32_PROFILE
        const uint32_t cycle_start = esp_cpu_get_cycle_count();
        ++fix32_div_count;
        const uint32_t cache_index =
            (uint32_t(m_bits) * 2654435761u ^ uint32_t(x.m_bits)) & 255u;
        fix32_div_cache_entry &cached = fix32_div_cache[cache_index];
        if (cached.valid && cached.numerator == m_bits &&
            cached.denominator == x.m_bits)
        {
            ++fix32_div_cache_hits;
            fix32_div_cycles += esp_cpu_get_cycle_count() - cycle_start;
            return frombits(cached.result);
        }
#endif
        int32_t result_bits;
        // This special case ensures 0x8000/0x1 = 0x8000, not 0x8000.0001
        if (x.m_bits == 0x10000)
        {
            result_bits = m_bits;
#ifdef PICO_FIX32_PROFILE
            ++fix32_div_integer_fast;
#endif
        }
        else if (x.m_bits && (x.m_bits & 0xffff) == 0)
        {
            const int64_t result = int64_t(m_bits) / (x.m_bits / 0x10000);
            using std::abs;
            if (abs(result) <= 0x7fffffffu)
                result_bits = int32_t(result);
            else
                result_bits = (m_bits ^ x.m_bits) >= 0
                    ? int32_t(0x7fffffffu) : int32_t(0x80000001u);
#ifdef PICO_FIX32_PROFILE
            ++fix32_div_integer_fast;
#endif
        }
        else if (x.m_bits)
        {
            using std::abs;
            const int64_t result = int64_t(m_bits) * 0x10000 / x.m_bits;
            if (abs(result) <= 0x7fffffffu)
                result_bits = int32_t(result);
            else
                result_bits = (m_bits ^ x.m_bits) >= 0
                    ? int32_t(0x7fffffffu) : int32_t(0x80000001u);
        }
        else
        {
            // Return 0x8000.0001 (not 0x8000.0000) for -Inf, just like PICO-8
            result_bits = (m_bits ^ x.m_bits) >= 0
                ? int32_t(0x7fffffffu) : int32_t(0x80000001u);
        }

#ifdef PICO_FIX32_PROFILE
        cached = {m_bits, x.m_bits, result_bits, true};
        fix32_div_cycles += esp_cpu_get_cycle_count() - cycle_start;
#endif
        return frombits(result_bits);
    }

    fix32 operator %(fix32 x) const
    {
        // PICO-8 always returns positive values
        x = abs(x);
        int32_t result = x ? m_bits % x.m_bits : m_bits;
        return frombits(result >= 0 ? result : result + x.m_bits);
    }

    inline fix32 operator <<(int y) const
    {
        // If y is negative, use lshr() instead.
        return y < 0 ? lshr(*this, -y) : frombits(y >= 32 ? 0 : bits() << y);
    }

    inline fix32 operator >>(int y) const
    {
        using std::min;
        // If y is negative, use << instead.
        return y < 0 ? *this << -y : frombits(bits() >> min(y, 31));
    }

    inline fix32& operator +=(fix32 x) { return *this = *this + x; }
    inline fix32& operator -=(fix32 x) { return *this = *this - x; }
    inline fix32& operator &=(fix32 x) { return *this = *this & x; }
    inline fix32& operator |=(fix32 x) { return *this = *this | x; }
    inline fix32& operator ^=(fix32 x) { return *this = *this ^ x; }
    inline fix32& operator *=(fix32 x) { return *this = *this * x; }
    inline fix32& operator /=(fix32 x) { return *this = *this / x; }
    inline fix32& operator %=(fix32 x) { return *this = *this % x; }

    // Free functions
    static inline fix32 abs(fix32 a) { return a.m_bits > 0 ? a : -a; }
    static inline fix32 min(fix32 a, fix32 b) { return a < b ? a : b; }
    static inline fix32 max(fix32 a, fix32 b) { return a > b ? a : b; }

    static inline fix32 ceil(fix32 x) { return -floor(-x); }
    static inline fix32 floor(fix32 x) { return frombits(x.m_bits & 0xffff0000); }

    static fix32 pow(fix32 x, fix32 y) { return fix32(std::pow(double(x), double(y))); }

    static inline fix32 lshr(fix32 x, int y)
    {
        // If y is negative, use << instead.
        return y < 0 ? x << -y : frombits(y >= 32 ? 0 : uint32_t(x.bits()) >> y);
    }

    static inline fix32 rotl(fix32 x, int y)
    {
        y &= 0x1f;
        return frombits((x.bits() << y) | (uint32_t(x.bits()) >> (32 - y)));
    }

    static inline fix32 rotr(fix32 x, int y)
    {
        y &= 0x1f;
        return frombits((uint32_t(x.bits()) >> y) | (x.bits() << (32 - y)));
    }

private:
    int32_t m_bits;
};

}

