#include <algorithm>
#include <map>
#include <thread>
#include <list>
#include <fstream>

#include <stdint.h>

#include <unordered_map>

#include "common.h"
#include "testgen.h"
#include "rbtree.h"

#define key_t uint32_t
#define value_t Test::TestValue*
#define testedmap_t RBTree::RBTree
#define CHECK_UNO 0

namespace Test
{
    //////////////////////////////////////////////////////////////////
    template<class K, class V>
    class TMTSTDMap : public std::map<K, V> {
    public:
        template<typename... Args>
        std::pair<typename std::map<K, V>::iterator, bool> emplace(const K& key, Args&&... args) {
            std::lock_guard<decltype (m_lock)> g(m_lock);
            return std::map<K, V>::emplace(key, std::forward<Args&&>(args)...);
        }

        size_t erase(K key) {
            std::lock_guard<decltype (m_lock)> g(m_lock);
            return std::map<K, V>::erase(key);
        }

    private:
        std::mutex m_lock;
    };

    //////////////////////////////////////////////////////////////////
    template<class K, class V>
    class TMTSTDUnorderedMap : public std::unordered_map<K, V> {
    public:
        template<typename... Args>
        std::pair<typename std::unordered_map<K, V>::iterator, bool> emplace(const K& key, Args&&... args) {
            std::lock_guard<decltype (m_lock)> g(m_lock);
            return std::unordered_map<K, V>::emplace(key, std::forward<Args&&>(args)...);
        }

        size_t erase(K key) {
            std::lock_guard<decltype (m_lock)> g(m_lock);
            return std::unordered_map<K, V>::erase(key);
        }

    private:
        std::mutex m_lock;
    };

    //////////////////////////////////////////////////////////////////

    template<class T>
    Duration BenchMap(const std::vector<TestCommand>& commands, std::vector<value_t>& values, uint32_t nthreads) noexcept
    {
        T map;
        const uint32_t cmd_per_thread = commands.size() / nthreads;

        Timestamp start = Timestamp::Now();

        std::list<std::thread> treads;
        for (uint32_t i = 0; i < nthreads; ++i)
        {
            const TestCommand* start_cmd = &(commands[i * cmd_per_thread]);
            treads.emplace_back(
                [&map, &values](const TestCommand* commands, uint32_t size) -> void
                {
                    for (uint32_t i = 0; i < size; ++i)
                    {
                        const TestCommand& cmd = commands[i];
                        if (cmd.m_is_add)
                            map.emplace(cmd.m_key, values[cmd.m_key]);
                        else
                            map.erase(cmd.m_key);
                    }
                },
                start_cmd, cmd_per_thread);
        }

        for (uint32_t i = 0; i < nthreads; ++i)
        {
            treads.front().join();
            treads.pop_front();
        }

        return Timestamp::Now() - start;
    }

    //////////////////////////////////////////////////////////////////

    class BenchBox
    {
    public:

        BenchBox()
        { }

        BenchBox(const BenchBox& other) = delete;
        BenchBox(BenchBox&& other) noexcept = delete;
        BenchBox& operator=(const BenchBox& other) = delete;
        BenchBox& operator=(BenchBox&& other) noexcept = delete;

        bool run(TestGeneratorBucketed generator, uint32_t sample_size, uint32_t nthreads, uint32_t niterations);

    };

    //--------------------------------------------------------------//

    bool BenchBox::run(TestGeneratorBucketed generator, uint32_t sample_size,
        uint32_t nthreads, uint32_t niterations)
    {
        std::vector<value_t> values = GenValues<std::remove_pointer<value_t>::type>(sample_size);
        std::vector<TestCommand> sample(sample_size, {0, false});

        Duration gen_time;
        Duration map_time;
        Duration origin_time;
        Duration unordered_time;
        for (uint32_t i = 0; i < niterations; ++i) {

            {
                Timestamp start = Timestamp::Now();
                generator(sample, sample_size, 1);
                gen_time += (Timestamp::Now() - start);
            }

            // warm up
            if (1 == nthreads)
                BenchMap<std::map<key_t, value_t>>(sample, values, nthreads);
            else
                BenchMap<TMTSTDMap<key_t, value_t>>(sample, values, nthreads);

            map_time += (1 == nthreads) ?
                BenchMap<testedmap_t<key_t, value_t>>(sample, values, nthreads) :
                BenchMap<testedmap_t<key_t, value_t, std::mutex>>(sample, values, nthreads);

            origin_time += (1 == nthreads) ?
                BenchMap<std::map<key_t, value_t>>(sample, values, nthreads) :
                BenchMap<TMTSTDMap<key_t, value_t>>(sample, values, nthreads);

#if CHECK_UNO
            unordered_time += (1 == nthreads) ?
                BenchMap<std::unordered_map<key_t, value_t>>(sample, values, nthreads) :
                BenchMap<TMTSTDUnorderedMap<key_t, value_t>>(sample, values, nthreads);
#endif
        }

        KillValues(values);

        std::cout << std::fixed << std::setprecision(2) << std::setw(6);
        const auto width = std::setw(9);

        std::cout << "Gen time:      " << width
                  << static_cast<double>(gen_time.Milliseconds())
                  << std::endl;

        const double map_speed = (double)sample_size / map_time.Milliseconds();
        const double origin_speed = (double)sample_size / origin_time.Milliseconds();
        const double origin_diff = ((map_speed / origin_speed) - 1) * 100;

        std::cout << "NoNode time:   " << width
                  << static_cast<double>(map_time.Milliseconds()) << std::endl;
        std::cout << "std::map time: " << width
                  << static_cast<double>(origin_time.Milliseconds())
                  << " rel imp: " << (origin_diff > 0 ? '+' : ' ')
                  << std::setprecision(2) << origin_diff << "%" << std::endl;

#if CHECK_UNO
        const double unordered_speed = (double)sample_size / unordered_time.Milliseconds();
        const double unordered_diff = ((map_speed / unordered_speed) - 1) * 100;

        std::cout << "std::uno time: " << width
                  << static_cast<double>(unordered_time.Milliseconds())
                  << " rel imp: " << (unordered_diff > 0 ? '+' : ' ')
                  << std::setprecision(2) << unordered_diff << "%" << std::endl;
#endif

        return true;
    }

    //--------------------------------------------------------------//

    //////////////////////////////////////////////////////////////////

    TEST(TreeTest, bench_add_small)
    {
        constexpr uint32_t sample_size = 64;
        constexpr uint32_t nthreads = 1;
        constexpr uint32_t niterations = 25000;

        BenchBox tb;
        tb.run(AddTestGeneratorBucketed, sample_size, nthreads, niterations);
    }

    TEST(TreeTest, bench_add_medium)
    {
        constexpr uint32_t sample_size = 1024;
        constexpr uint32_t nthreads = 1;
        constexpr uint32_t niterations = 10000;

        BenchBox tb;
        tb.run(AddTestGeneratorBucketed, sample_size, nthreads, niterations);
    }

    TEST(TreeTest, bench_add_big)
    {
        constexpr uint32_t sample_size = 100000;
        constexpr uint32_t nthreads = 1;
        constexpr uint32_t niterations = 60;

        BenchBox tb;
        tb.run(AddTestGeneratorBucketed, sample_size, nthreads, niterations);
    }

    //--------------------------------------------------------------//

    TEST(TreeTest, bench_add_rem_small)
    {
        constexpr uint32_t sample_size = 64;
        constexpr uint32_t nthreads = 1;
        constexpr uint32_t niterations = 25000;

        BenchBox tb;
        tb.run(AddRemoveTestGeneratorBucketed, sample_size, nthreads, niterations);
    }

    TEST(TreeTest, bench_add_rem_medium)
    {
        constexpr uint32_t sample_size = 1024;
        constexpr uint32_t nthreads = 1;
        constexpr uint32_t niterations = 10000;

        BenchBox tb;
        tb.run(AddRemoveTestGeneratorBucketed, sample_size, nthreads, niterations);
    }

    TEST(TreeTest, bench_add_rem_big)
    {
        constexpr uint32_t sample_size = 100000;
        constexpr uint32_t nthreads = 1;
        constexpr uint32_t niterations = 250;

        BenchBox tb;
        tb.run(AddRemoveTestGeneratorBucketed, sample_size, nthreads, niterations);
    }

    //--------------------------------------------------------------//

    TEST(TreeTest, bench_mt_add_small)
    {
        constexpr uint32_t sample_size = 64;
        constexpr uint32_t nthreads = 8;
        constexpr uint32_t niterations = 6000;

        BenchBox tb;
        tb.run(AddTestGeneratorBucketed, sample_size, nthreads, niterations);
    }

    TEST(TreeTest, bench_mt_add_medium)
    {
        constexpr uint32_t sample_size = 1024;
        constexpr uint32_t nthreads = 8;
        constexpr uint32_t niterations = 2500;

        BenchBox tb;
        tb.run(AddTestGeneratorBucketed, sample_size, nthreads, niterations);
    }

    TEST(TreeTest, bench_mt_add_big)
    {
        constexpr uint32_t sample_size = 100000;
        constexpr uint32_t nthreads = 8;
        constexpr uint32_t niterations = 16;

        BenchBox tb;
        tb.run(AddTestGeneratorBucketed, sample_size, nthreads, niterations);
    }

    //////////////////////////////////////////////////////////////////

}
