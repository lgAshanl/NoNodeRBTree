#pragma once

#include "types.h"
#include <random>

namespace Test
{
    inline uint32_t High(uint64_t value) {
        return (value >> 32) & 0xffffffff;
    }

    inline uint32_t Low(uint64_t value) {
        return value & 0xffffffff;
    }

    //////////////////////////////////////////////////////////////////

    class Rand
    {
    public:
        explicit Rand()
          : m_rd(),
            m_gen(m_rd())
        { }

        Rand(const Rand& other) = delete;
        Rand(Rand&& other) noexcept = delete;
        Rand& operator=(const Rand& other) = delete;
        Rand& operator=(Rand&& other) noexcept = delete;

        inline uint64_t get()
        {
            return m_gen();
        }

    private:

        std::random_device m_rd;
        std::mt19937_64 m_gen;
    };

    //////////////////////////////////////////////////////////////////

    class Rand64
    {
    public:
        explicit Rand64()
          : m_rd(),
            m_value(m_rd.get()),
            m_shift(0)
        { }

        Rand64(const Rand64& other) = delete;
        Rand64(Rand64&& other) noexcept = delete;
        Rand64& operator=(const Rand64& other) = delete;
        Rand64& operator=(Rand64&& other) noexcept = delete;

        inline uint8_t get()
        {
            if (m_shift > 58)
            {
                m_value = m_rd.get();
                m_shift = 0;
            }

            const uint8_t res = (uint8_t)(m_value & 0b111111);
            m_value >>= 6;
            m_shift += 6;
            return res;
        }

    private:
        Rand m_rd;

        uint64_t m_value;

        uint32_t m_shift;
    };

    //////////////////////////////////////////////////////////////////

    class Duration {
    public:
        Duration(uint64_t microseconds = 0)
          : m_microseconds(microseconds)
        { }

        Duration& operator+=(const Duration& other) {
            m_microseconds += other.m_microseconds;
            return *this;
        }

        uint64_t Milliseconds() {
            return m_microseconds / 1000;
        }

    private:
        uint64_t m_microseconds;
    };

    class Timestamp {
    public:
        Timestamp(std::chrono::time_point<std::chrono::steady_clock> time)
          : m_value(time)
        { }

        Duration operator-(const Timestamp& previous) const {
            return Duration(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    m_value - previous.m_value).count());
        }

        static Timestamp Now() {
            return Timestamp(std::chrono::steady_clock::now());
        }

    private:
        std::chrono::time_point<std::chrono::steady_clock> m_value;
    };

}