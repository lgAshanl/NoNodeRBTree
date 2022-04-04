#include "types.h"
#include <random>

namespace Test
{
    //////////////////////////////////////////////////////////////////

    class Rand64
    {
    public:
        explicit Rand64()
          : m_rd(),
            m_gen(m_rd()),
            m_value(m_gen()),
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
                m_value = m_gen();
                m_shift = 0;
            }

            const uint8_t res = (uint8_t)(m_value & 0b111111);
            m_value >>= 6;
            m_shift += 6;
            return res;
        }

    private:

        std::random_device m_rd;
        std::mt19937_64 m_gen;

        uint64_t m_value;

        uint32_t m_shift;
    };

}