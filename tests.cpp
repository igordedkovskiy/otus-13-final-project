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
        std::cout << "newq.top: " << std::setw(3) << *newq_p
                  << "; q.top: " << std::setw(3) << *q_p << '\n';
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

    constexpr std::size_t num_of_elements {1000};

    auto make_data = [num_of_elements](auto num_of_producers)
    {
        return all_data_collection_t(num_of_producers,
                                     data_collection_t(num_of_elements/num_of_producers,1));
    };

    auto producer = [](Queue<data_t>& queue, data_collection_t& data)
    {
        std::cout << "producer " << std::this_thread::get_id() << ": start" << std::endl;
        for(auto it {std::begin(data)}; it != std::end(data);)
        {
            queue.wait_if_full_push(*it);
            it = data.erase(it);
        }
        std::cout << "producer " << std::this_thread::get_id() << ": end" << std::endl;
    };

    auto consumer = [](Queue<data_t>& queue, const all_data_collection_t& data)
    {
        std::cout << "consumer " << std::this_thread::get_id() << ": start" << std::endl;
        auto data_empty = [&data]()
        {
            for(const auto& el:data)
            {
                if(!el.empty())
                    return false;
            }
            return true;
        };

        std::size_t cntr = 0;

        do
        {
            auto el {queue.pop()};
            if(el)
                cntr += *el;
        }
        while(!queue.empty() || !data_empty());
        std::cout << "consumer " << std::this_thread::get_id() << ": end" << std::endl;
    };

    auto run = [&producer, &consumer, &make_data](auto num_of_producers, auto num_of_consumers)
    {
        std::cout << "run start" << std::endl;
        auto data {make_data(num_of_producers)};
        Queue<data_t> queue;

        std::vector<std::thread> producers;
        producers.reserve(num_of_producers);
        for(auto& producers_data:data)
            producers.emplace_back(producer, std::ref(queue), std::ref(producers_data));

        std::vector<std::thread> consumers;
        consumers.reserve(num_of_consumers);
        for(std::size_t cntr {0}; cntr < num_of_consumers; ++cntr)
            consumers.emplace_back(consumer, std::ref(queue), std::ref(data));

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
        std::cout << "run end" << std::endl;
    };

    auto start {clock::now()};
    // create input data
    // create producer
    // move input data into producer
    // create graph
    // create consumer, move graph into consumer
    // create framework
    // move producer and consumer into framework
    // run framework
    run(1, 1);
    const auto single_duration {duration_cast<milliseconds>(clock::now() - start).count()};
    start = clock::now();
    // repeat for multiple tests
    run(4, 4);
    const auto multiple_duration {duration_cast<milliseconds>(clock::now() - start).count()};
    std::cout << "single_duration|multiple_duration (ms): "
              << single_duration << '|'
              << multiple_duration << std::endl;
    EXPECT_LT(single_duration, multiple_duration);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
