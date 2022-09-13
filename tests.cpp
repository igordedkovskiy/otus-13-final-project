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
    using namespace std::chrono;
    using namespace threadsafe_containers;
    using clock = system_clock;
    using data_t = std::uint64_t;
    using data_collection_t = std::list<data_t>;
    using all_data_collection_t = std::vector<data_collection_t>;
    using cond_cntr_t = std::atomic_llong;

    constexpr std::size_t num_of_elements {1000};

    auto make_data = [num_of_elements](auto num_of_producers)
    {
        return all_data_collection_t(num_of_producers,
                                     data_collection_t(num_of_elements/num_of_producers,1));
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
        std::size_t cntr {0};
        do
        {
            auto el {queue.pop()};
            if(el)
            {
                cntr += *el;
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1ms);
            }
        }
        while(!queue.empty() || el_left);
    };

    // create input data
    // create producer
    // move input data into producer
    // create graph
    // create consumer, move graph into consumer
    // create framework
    // move producer and consumer into framework
    // run framework
    auto run = [&producer, &consumer, &make_data](std::size_t num_of_producers, std::size_t num_of_consumers)
    {
        const auto start {clock::now()};
        const auto data {make_data(num_of_producers)};
        Queue<data_t> queue;

        cond_cntr_t elements_left (num_of_elements);

        std::vector<std::thread> producers;
        producers.reserve(num_of_producers);
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
        EXPECT_TRUE(queue.empty());
        return duration_cast<milliseconds>(clock::now() - start).count();
    };

    const auto single_duration {run(1, 1)};
    // repeat for multiple tests
    const auto multiple_duration {run(4, 4)};
    std::cout << "single_duration|multiple_duration (ms): "
              << single_duration << '|'
              << multiple_duration << std::endl;
    EXPECT_GE(single_duration, multiple_duration);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
