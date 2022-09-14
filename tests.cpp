#include <vector>
#include <list>
#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cassert>
#include <numeric>
#include <iterator>
#include <chrono>
#include <cmath>
#include "gtest/gtest.h"

#include "Queue.hpp"

TEST(TEST_QUEUE, serialize_queue)
{
    namespace fs = std::filesystem;
    using namespace threadsafe_containers;
    using value_type = int;

    const fs::path path{"qarchive"};
    Queue<value_type> q{path};
    // clear queue if qarchive exists and not empty
    q.clear();
    q.push(1);
    q.push(3);
    q.push(6);
    q.push(12);

    {
        Queue<value_type> q{path};
        // clear queue if qarchive exists and not empty
        q.clear();
        q.push(1);
        q.push(3);
        q.push(6);
        q.push(12);
        // destroy and save q
    }

    // create newq and load it's value from an archive
    Queue<value_type> newq{path};
    // compare newq against q
    EXPECT_EQ(newq, q);
    while(!newq.empty())
    {
        auto newq_p = newq.pop();
        auto q_p = q.pop();
        //std::cout << "newq.top: " << std::setw(3) << *newq_p
        //          << "; q.top: " << std::setw(3) << *q_p << '\n';
        EXPECT_EQ(*newq_p, *q_p);
    }
}

/// \brief Compare execution time in two cases. First is one producer, one consumer.
///        Second is multiple producers, multiple consumers.
TEST(TEST_QUEUE, producer_consumer)
{
    const auto num_of_cores {std::thread::hardware_concurrency()};
    std::cout << "number of cores is " << num_of_cores << std::endl;
    if(num_of_cores == 1)
        return;

    using namespace std::chrono;
    using namespace threadsafe_containers;
    using clock = system_clock;
    using data_t = std::uint64_t;
    using data_collection_t = std::vector<data_t>;
    using all_data_collection_t = std::vector<data_collection_t>;
    using cond_cntr_t = std::atomic_llong;
    using num_of_elements_t = std::size_t;
    using ranges_t = std::vector<num_of_elements_t>;

    auto make_data = [](std::size_t num_of_producers, const ranges_t& ranges)
    {
        all_data_collection_t data;
        data.reserve(num_of_producers);
        for(auto num_of_elements:ranges)
        {
            data_collection_t element;
            element.reserve(num_of_elements);
            for(std::size_t cntr {0}; cntr < num_of_elements; ++cntr)
                //element.emplace_back(rand() % 24 + cntr);
                element.emplace_back(cntr * 13);
            data.emplace_back(std::move(element));
        }
        return data;
    };

    auto producer = [](Queue<data_t>& queue, const data_collection_t& data, cond_cntr_t& el_left)
    {
        for(auto el:data)
        {
            queue.wait_if_full_push(el);
            --el_left;
        }
    };

    auto consumer = [](Queue<data_t>& queue, const cond_cntr_t& el_left)
    {
        //std::size_t cntr {0};
        while(!queue.empty() || el_left)
        {
            auto el {queue.pop()};
            if(el)
            {
                std::vector<data_t> v;
                const std::size_t N {100000};
                v.reserve(N);
                for(std::size_t cntr {0}; cntr < N; ++cntr)
                    //v.emplace_back(rand() % 100 + *el);
                    v.emplace_back(cntr + *el);
                //using namespace std::chrono_literals;
                //std::this_thread::sleep_for(1ms);
            }
        }
    };

    // create input data
    // create producer
    // move input data into producer
    // create graph
    // create consumer, move graph into consumer
    // create framework
    // move producer and consumer into framework
    // run framework
    auto run = [&producer, &consumer, &make_data]
            (std::size_t num_of_producers, std::size_t num_of_consumers, std::size_t num_of_elements)
    {
        auto split = [&num_of_elements, &num_of_producers]()
        {
            if(num_of_producers == 1)
                return ranges_t(1, num_of_elements);
            const auto size {num_of_elements / num_of_producers};
            ranges_t ranges;
            ranges.reserve(num_of_producers);
            for(std::size_t cntr {0}; cntr < num_of_producers - 1; ++cntr)
                ranges.push_back(size);
            ranges.push_back(num_of_elements - size * (num_of_producers - 1));
            return ranges;
        };

        const auto ranges {split()};
//        std::cout << "ranges: ";
//        for(auto v:ranges)
//            std::cout << v << ' ';
//        std::cout << std::endl;
        const auto data {make_data(num_of_producers, ranges)};
        Queue<data_t> queue;

        cond_cntr_t elements_left (num_of_elements);

        std::vector<std::thread> producers;
        producers.reserve(num_of_producers);
        const auto start {clock::now()};
        for(auto& producers_data:data)
            producers.emplace_back(producer, std::ref(queue), std::ref(producers_data), std::ref(elements_left));

        std::vector<std::thread> consumers;
        consumers.reserve(num_of_consumers);
        for(std::size_t cntr {0}; cntr < num_of_consumers; ++cntr)
            consumers.emplace_back(consumer, std::ref(queue), std::ref(elements_left));

        for(auto& cons:consumers)
        {
            if(cons.joinable())
                cons.join();
        }
        for(auto& prod:producers)
        {
            if(prod.joinable())
                prod.join();
        }
        return std::make_pair(duration_cast<milliseconds>(clock::now() - start).count(), queue.empty());
    };

    constexpr std::size_t num_of_elements {1000};
    /// \note one producers / consumers
    const auto [single_duration, result] {run(1, 1, num_of_elements)};
    ASSERT_TRUE(result);

    for(std::size_t mfactor {1}; mfactor < 6; ++mfactor)
    {
        /// \note multiple producers / consumers. Number of consumers = number of producers.
        //const std::size_t num_of_producers {
        //    static_cast<std::remove_cv_t<decltype(num_of_producers)>>(0.5 * num_of_cores * mfactor)};
        const std::size_t num_of_producers { static_cast<std::remove_cv_t<decltype(num_of_producers)>>
            (num_of_cores > 2 ? 0.5 * num_of_cores * mfactor: num_of_cores * mfactor)};
        const auto [multiple_duration, result2]{run(num_of_producers, num_of_producers, num_of_elements)};
        ASSERT_TRUE(result2);

        std::cout << "single_duration|multiple_duration (ms): "
                  << single_duration << '|' << multiple_duration
                  << "\tnumber of producers|consumers: "
                  << num_of_producers << '|' << num_of_producers
                  << std::endl;
        EXPECT_GE(single_duration, multiple_duration);
    }
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
