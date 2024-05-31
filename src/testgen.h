#pragma once

#include <algorithm>
#include <map>
#include <unordered_set>
#include <thread>
#include <list>
#include <fstream>

#include <gtest/gtest.h>

#include "common.h"

#define key_t uint32_t
// TODO: fix
#define MAX_KEY 64

namespace Test
{
    //////////////////////////////////////////////////////////////////

    struct TestValue
    {
        TestValue* m_left;

        TestValue* m_right;

        TestValue* m_parent;

        key_t m_key;
    };

    //////////////////////////////////////////////////////////////////

    struct TestCommand;

    typedef void (*TestGenerator)(std::vector<TestCommand>& sample, uint32_t sample_size);

    typedef void (*TestGeneratorBucketed)(std::vector<TestCommand>& sample, uint32_t sample_size, uint32_t nbuckets);

    //////////////////////////////////////////////////////////////////

    struct TestCommand
    {
        key_t m_key;

        bool m_is_add;
    };

    //////////////////////////////////////////////////////////////////

    inline void AddTestGenerator(std::vector<TestCommand>& sample, uint32_t sample_size)
    {
        Rand64 rand;

        uint64_t vector = 0;

        for (uint32_t i = 0; i < sample_size; ++i)
        {
            uint32_t key = rand.get();
            uint64_t shift = (uint64_t)1 << key;

            bool is_in_map = (vector & shift);

            while (is_in_map)
            {
                ++key;
                if (MAX_KEY == key)
                    key = 0;
                shift = (uint64_t)1 << key;

                is_in_map = (vector & shift);
            }

            sample[i] = {key, true};

            vector ^= shift;
        }
    }

    //////////////////////////////////////////////////////////////////

    inline void AddTestGeneratorBucketed(std::vector<TestCommand>& sample, uint32_t sample_size, uint32_t nbuckets)
    {
        assert(0 == (sample_size % nbuckets));
        for (uint32_t i = 0; i < sample_size; ++i)
        {
            sample[i] = {i, true};
        }

        Rand rand;

        const uint32_t bucket_size = sample_size / nbuckets;
        for (uint32_t bucket = 0; bucket < nbuckets; ++bucket) {
            const uint32_t bucket_start = bucket * bucket_size;
            for (uint32_t i = 0; i < bucket_size; ++i)
            {
                uint64_t random = rand.get();
                uint32_t key1 = bucket_start + (High(random) % bucket_size);
                uint32_t key2 = bucket_start + (Low(random) % bucket_size);

                std::swap(sample[key1], sample[key2]);
            }
        }
    }

    //////////////////////////////////////////////////////////////////

    inline void AddRemoveTestGeneratorBucketed(std::vector<TestCommand>& sample, uint32_t sample_size, uint32_t nbuckets)
    {
        // TODO: buckets
        (void)nbuckets;
        Rand64 rand;

        uint64_t vector = 0;

        for (uint32_t i = 0; i < sample_size; ++i)
        {
            const uint32_t key = rand.get();
            const uint64_t shift = (uint64_t)1 << key;

            const bool is_in_map = (vector & shift);

            sample[i] = {key, !is_in_map};

            vector ^= shift;
        }
    }

    //////////////////////////////////////////////////////////////////

    inline void AddRemoveTestGenerator(std::vector<TestCommand>& sample, uint32_t sample_size)
    {
        Rand64 rand;

        uint64_t vector = 0;

        for (uint32_t i = 0; i < sample_size; ++i)
        {
            const uint32_t key = rand.get();
            const uint64_t shift = (uint64_t)1 << key;

            const bool is_in_map = (vector & shift);

            sample[i] = {key, !is_in_map};

            vector ^= shift;
        }
    }

    //////////////////////////////////////////////////////////////////

    template<class T>
    inline std::vector<T*> GenValues(uint32_t size)
    {
        std::vector<T*> res;
        res.reserve(size);

        for (uint32_t i = 0; i < size; ++i)
        {
            res.emplace_back(new T());
        }

        return res;
    }

    //--------------------------------------------------------------//

    template<class T>
    void KillValues(std::vector<T*>& values)
    {
        for (T*& val : values)
        {
            delete val;
        }
    }
}
