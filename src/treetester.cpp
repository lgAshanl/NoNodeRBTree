#include <algorithm>
#include <map>
#include <thread>
#include <list>
#include <fstream>

#include <gtest/gtest.h>

#include "common.h"
#include "rbtree.h"

namespace Test
{
    struct TestValue;
}

#define key_t uint32_t
#define value_t Test::TestValue*
#define testedmap_t RBTree::RBTree
#define NVALUES 64
#define CHECK_ALWAYS 0

namespace Test
{
    struct TestCommand;

    typedef void (*TestGenerator)(std::vector<TestCommand>& sample, uint32_t sample_size);

    //////////////////////////////////////////////////////////////////

    struct TestCommand
    {
        key_t m_key;

        bool m_is_add;
    };

    //////////////////////////////////////////////////////////////////

    struct TestValue
    {
        TestValue* m_left;

        TestValue* m_right;

        TestValue* m_parent;

        key_t m_key;
    };

    //////////////////////////////////////////////////////////////////

    class TestBox
    {
    public:

        TestBox()
        { }

        TestBox(const TestBox& other) = delete;
        TestBox(TestBox&& other) noexcept = delete;
        TestBox& operator=(const TestBox& other) = delete;
        TestBox& operator=(TestBox&& other) noexcept = delete;

    public:

        bool run(TestGenerator generator, uint32_t sample_size,
                 uint32_t niterations, uint32_t ntreads);

        bool run_custom(const std::vector<TestCommand>& sample);

    private:

        static bool check(
            std::map<key_t, value_t>& origin,
            testedmap_t<key_t, value_t>& tested,
            std::vector<std::pair<bool,bool>>* vreturns,
            uint32_t vreturns_size);

        static void dump(
            uint32_t id,
            const std::vector<TestCommand>* vsamples,
            uint32_t vsamples_size);

    private:

        static std::vector<value_t> genValues();

        static void killValues(std::vector<value_t>& values);

    private:

        static inline bool execOriginFn(
            std::map<key_t, value_t>& map, const TestCommand& cmd, std::vector<value_t>& values) noexcept;

        static inline bool execTestedFn(
            testedmap_t<key_t, value_t>& map, const TestCommand& cmd, std::vector<value_t>& values) noexcept;
    };

    //--------------------------------------------------------------//

    bool TestBox::run(TestGenerator generator, uint32_t sample_size,
                 uint32_t niterations, uint32_t ntreads)
    {
        std::list<std::thread> treads;

        for (uint32_t i = 0; i < ntreads; ++i)
        {
            treads.emplace_back(
                [](int32_t id, TestGenerator generator, uint32_t niterations, uint32_t size) -> void
                {
                    std::vector<value_t> values = genValues();
                    std::vector<TestCommand> sample(size, {0, 0});
                    std::vector<std::pair<bool,bool>> returns(size, {0, 0});

                    testedmap_t<key_t, value_t> tested;
                    std::map<key_t, value_t> standard;

                    for (uint32_t iteration = 0; iteration < niterations; ++iteration)
                    {
                        generator(sample, size);

#if CHECK_ALWAYS
                        for (uint32_t i = 0; i < size; ++i)
                        {
                            returns[i].first = execOriginFn(standard, sample[i], values);
                            returns[i].second = execTestedFn(tested, sample[i], values);

                            if (!check(standard, tested, &returns, 1))
                            {
                                dump(id, &sample, 1);
                                exit(666);
                                return;
                            }
                        }

#else

                        for (uint32_t i = 0; i < size; ++i)
                        {
                            returns[i].first = execOriginFn(standard, sample[i], values);
                        }

                        for (uint32_t i = 0; i < size; ++i)
                        {
                            returns[i].second = execTestedFn(tested, sample[i], values);
                        }
#endif

                        if (!check(standard, tested, &returns, 1))
                        {
                            dump(id, &sample, 1);
                            exit(666);
                            return;
                        }

                        tested.clear();
                        standard.clear();
                    }

                    killValues(values);
                },
                i, generator, niterations, sample_size
                );
        }

        for (uint32_t i = 0; i < ntreads; ++i)
        {
            treads.front().join();
            treads.pop_front();
        }

        return true;
    }

    //--------------------------------------------------------------//

    bool TestBox::run_custom(const std::vector<TestCommand>& sample)
    {
        const size_t size = sample.size();
        std::vector<value_t> values = genValues();
        std::vector<std::pair<bool,bool>> returns(size, {0, 0});

        testedmap_t<key_t, value_t> tested;
        std::map<key_t, value_t> standard;

#if CHECK_ALWAYS
        for (uint32_t i = 0; i < size; ++i)
        {
            returns[i].first = execOriginFn(standard, sample[i], values);
            returns[i].second = execTestedFn(tested, sample[i], values);

            if (!check(standard, tested, &returns, 1))
            {
                dump(0, &sample, 1);
                exit(666);
                return false;
            }
        }

#else

        for (uint32_t i = 0; i < size; ++i)
        {
            returns[i].first = execOriginFn(standard, sample[i], values);
        }

        for (uint32_t i = 0; i < size; ++i)
        {
            returns[i].second = execTestedFn(tested, sample[i], values);
        }
#endif

        if (!check(standard, tested, &returns, 1))
        {
            dump(0, &sample, 1);
            exit(666);
            return false;
        }

        tested.clear();
        standard.clear();

        killValues(values);

        return true;
    }

    //--------------------------------------------------------------//

    bool TestBox::check(
        std::map<key_t, value_t>& origin,
        testedmap_t<key_t, value_t>& tested,
        std::vector<std::pair<bool,bool>>* vreturns,
        uint32_t vreturns_size)
    {
        for (uint32_t v = 0; v < vreturns_size; ++v)
        {
            const std::vector<std::pair<bool,bool>>& returns = vreturns[v];
            const size_t size = returns.size();
            for (size_t i = 0; i < size; ++i)
            {
                if (returns[i].first != returns[i].second)
                {
                    return false;
                }
            }
        }

        const size_t size = origin.size();
        if (origin.size() != tested.size())
        {
            return false;
        }

        std::vector<std::pair<key_t, value_t>> origin_v(origin.begin(), origin.end());
        std::vector<std::pair<key_t, value_t>> tested_v(tested.begin(), tested.end());
        if (origin_v.size() != tested_v.size())
        {
            return false;
        }
        assert(origin_v.size() == tested_v.size());

        std::sort(origin_v.begin(), origin_v.end());
        // tested_v should be already sorted
        //std::sort(tested_v.begin(), tested_v.end());

        bool flag = true;

        for (uint32_t i = 0; i < size; ++i)
        {
            flag &= (origin_v[i] == tested_v[i]);
        }

        return flag && tested.checkRB();
    }

    //--------------------------------------------------------------//

    void TestBox::dump(
        uint32_t id,
        const std::vector<TestCommand>* vsamples,
        uint32_t vsamples_size)
    {
        std::setfill('0'); std::setw(3);
        for (uint32_t i = 0; i < vsamples_size; ++i)
        {
            std::string filename = "dump" + std::to_string(id) + "_" + std::to_string(i) + ".dump";
            std::ofstream file;
            file.open(filename);

            for (auto cmd : vsamples[i])
            {
                const char* const op = (cmd.m_is_add) ? "ADD" : "RMV";
                file << std::string(op) << " " << id << " " << i << "/n";
            }
            file.close();
        }
    }

    //--------------------------------------------------------------//

    std::vector<value_t> TestBox::genValues()
    {
        std::vector<value_t> res;
        res.reserve(NVALUES);

        for (uint32_t i = 0; i < NVALUES; ++i)
        {
            res.emplace_back(new TestValue());
        }

        return res;
    }

    //--------------------------------------------------------------//

    void TestBox::killValues(std::vector<value_t>& values)
    {
        for (value_t val : values)
        {
            delete val;
        }
    }

    //--------------------------------------------------------------//

    bool TestBox::execOriginFn(
        std::map<key_t, value_t>& map, const TestCommand& cmd, std::vector<value_t>& values) noexcept
    {
        if (cmd.m_is_add)
        {
            return map.emplace(cmd.m_key, values[cmd.m_key]).second;
        }
        else
        {
            return (0 == map.erase(cmd.m_key));
        }
    }

    //--------------------------------------------------------------//

    bool TestBox::execTestedFn(
        testedmap_t<key_t, value_t>& map, const TestCommand& cmd, std::vector<value_t>& values) noexcept
    {
        if (cmd.m_is_add)
        {
            return map.emplace(cmd.m_key, values[cmd.m_key]).second;
        }
        else
        {
            return (0 == map.erase(cmd.m_key));
        }
    }

    //////////////////////////////////////////////////////////////////

    void AddTestGenerator(std::vector<TestCommand>& sample, uint32_t sample_size)
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
                if (key == 64)
                    key = 0;
                shift = (uint64_t)1 << key;

                is_in_map = (vector & shift);
            }

            sample[i] = {key, !is_in_map};

            vector ^= shift;
        }
    }

    //--------------------------------------------------------------//

    TEST(TreeTest, brut_add_small_sample)
    {
        constexpr uint32_t sample_size = 10;
        constexpr uint32_t niterations = 10000;
        constexpr uint32_t nthreads = 12;

        TestBox tb;
        tb.run(AddTestGenerator, sample_size, niterations, nthreads);
    }

    //--------------------------------------------------------------//

    TEST(TreeTest, brut_add_max_sample)
    {
        constexpr uint32_t sample_size = 64;
        constexpr uint32_t niterations = 1000;
        constexpr uint32_t nthreads = 12;

        TestBox tb;
        tb.run(AddTestGenerator, sample_size, niterations, nthreads);
    }

    //////////////////////////////////////////////////////////////////

    void AddRemoveTestGenerator(std::vector<TestCommand>& sample, uint32_t sample_size)
    {
        //sample.clear();
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

    //--------------------------------------------------------------//

    TEST(TreeTest, brut_add_remove_manual)
    {
        GTEST_SKIP();
        constexpr uint32_t sample_size = 100000;
        constexpr uint32_t niterations = 10000;
        constexpr uint32_t nthreads = 16;

        TestBox tb;
        tb.run(AddRemoveTestGenerator, sample_size, niterations, nthreads);
    }

    //--------------------------------------------------------------//

    TEST(TreeTest, brut_add_remove_small_sample)
    {
        constexpr uint32_t sample_size = 20;
        constexpr uint32_t niterations = 10000;
        constexpr uint32_t nthreads = 12;

        TestBox tb;
        tb.run(AddRemoveTestGenerator, sample_size, niterations, nthreads);
    }

    //--------------------------------------------------------------//

    TEST(TreeTest, brut_add_remove_big_sample)
    {
        constexpr uint32_t sample_size = 10000;
        constexpr uint32_t niterations = 10;
        constexpr uint32_t nthreads = 12;

        TestBox tb;
        tb.run(AddRemoveTestGenerator, sample_size, niterations, nthreads);
    }

    //////////////////////////////////////////////////////////////////
    //                           custom tests                       //
    //////////////////////////////////////////////////////////////////

    TEST(TreeTest, add1)
    {
        std::vector<TestCommand> sample = {
            {37, true},
            {21, true},
            {20, true},
            {38, true},
            {14, true},
            {45, true},
            {18, true},
            {9, true},
            {57, true},
            {6, true},
        };

        TestBox tb;
        tb.run_custom(sample);
    }

    //--------------------------------------------------------------//

    TEST(TreeTest, add_rem1)
    {
        std::vector<TestCommand> sample = {
            {36, true},
            {44, true},
            {17, true},
            {31, true},
            {40, true},
            {58, true},
            {42, true},
            {40, false},
            {18, true},
            {14, true},
        };

        TestBox tb;
        tb.run_custom(sample);
    }

    //--------------------------------------------------------------//

    TEST(TreeTest, add_rem2)
    {
        std::vector<TestCommand> sample = {
            {32, true},
            {29, true},
            {2 , true},
            {61, true},
            {62, true},
            {57, true},
            {62, false},
            {10, true},
            {5 , true},
            {4 , true},
        };

        TestBox tb;
        tb.run_custom(sample);
    }

    //--------------------------------------------------------------//

    TEST(TreeTest, add_rem3)
    {
        std::vector<TestCommand> sample = {
            { 9, true},
            {60, true},
            {18, true},
            {32, true},
            { 9, false},
            { 7, true},
            {41, true},
            {36, true},
            { 0, true},
            {43, true},
        };

        TestBox tb;
        tb.run_custom(sample);
    }

    //--------------------------------------------------------------//

    TEST(TreeTest, add_rem4)
    {
        std::vector<TestCommand> sample = {
            {61, true},
            {48, true},
            {31, true},
            {15, true},
            {25, true},
            {48, false},
            {41, true},
            { 5, true},
            {25, false},
            { 7, true},
        };

        TestBox tb;
        tb.run_custom(sample);
    }

    //--------------------------------------------------------------//

    TEST(TreeTest, add_rem5)
    {
        std::vector<TestCommand> sample = {
            {29, true},
            {51, true},
            {40, true},
            {45, true},
            { 2, true},
            { 0, true},
            {13, true},
            {51, false},
            {30, true},
            {45, false},
        };

        TestBox tb;
        tb.run_custom(sample);
    }

    //--------------------------------------------------------------//

    TEST(TreeTest, add_rem6)
    {
        std::vector<TestCommand> sample = {
            {35, true},
            { 8, true},
            {49, true},
            {19, true},
            {17, true},
            { 1, true},
            {12, true},
            {45, true},
            {25, true},
            {47, true},
            { 0, true},
            {20, true},
            {30, true},
            {57, true},
            {31, true},
            { 1, false},
            {61, true},
            {51, true},
            { 8, false},
            {44, true},
        };

        TestBox tb;
        tb.run_custom(sample);
    }

    //--------------------------------------------------------------//
}
