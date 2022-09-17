#include <vector>
#include <list>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <numeric>
#include <chrono>
#include "gtest/gtest.h"

#include "Queue.hpp"
#include "serialization.hpp"
#include "ProducerConsumer.hpp"


/// \brief Compare execution time in two cases. First is one producer, one consumer.
///        Second is multiple producers, multiple consumers.
//TEST(TEST_QUEUE, producer_consumer)
//{
//    const auto num_of_cores {std::thread::hardware_concurrency()};
//    std::cout << "number of cores is " << num_of_cores << std::endl;
//    if(num_of_cores == 1)
//        return;

//    using namespace std::chrono;
//    using namespace threadsafe_containers;
//    using clock = system_clock;
//    using data_t = std::uint64_t;
//    using data_collection_t = std::vector<data_t>;
//    using all_data_collection_t = std::vector<data_collection_t>;
//    using cond_cntr_t = std::atomic_llong;
//    using num_of_elements_t = std::size_t;
//    using ranges_t = std::vector<num_of_elements_t>;

//    auto make_data = [](std::size_t num_of_producers, const ranges_t& ranges)
//    {
//        all_data_collection_t data;
//        data.reserve(num_of_producers);
//        for(auto num_of_elements:ranges)
//        {
//            data_collection_t element;
//            element.reserve(num_of_elements);
//            for(std::size_t cntr {0}; cntr < num_of_elements; ++cntr)
//                element.emplace_back(cntr * 13);
//            data.emplace_back(std::move(element));
//        }
//        return data;
//    };

//    auto producer = [](Queue<data_t>& queue, const data_collection_t& data, cond_cntr_t& el_left)
//    {
//        for(auto el:data)
//        {
//            queue.wait_if_full_push(el);
//            --el_left;
//        }
//    };

//    auto consumer = [](Queue<data_t>& queue, const cond_cntr_t& el_left)
//    {
//        while(!queue.empty() || el_left)
//        {
//            auto el {queue.pop()};
//            if(el)
//            {
//                std::vector<data_t> v;
//                const std::size_t N {100000};
//                v.reserve(N);
//                for(std::size_t cntr {0}; cntr < N; ++cntr)
//                    v.emplace_back(cntr + *el);
//            }
//        }
//    };

//    auto run = [&producer, &consumer, &make_data]
//            (std::size_t num_of_producers, std::size_t num_of_consumers, std::size_t num_of_elements)
//    {
//        auto split = [&num_of_elements, &num_of_producers]()
//        {
//            if(num_of_producers == 1)
//                return ranges_t(1, num_of_elements);
//            const auto size {num_of_elements / num_of_producers};
//            ranges_t ranges;
//            ranges.reserve(num_of_producers);
//            for(std::size_t cntr {0}; cntr < num_of_producers - 1; ++cntr)
//                ranges.push_back(size);
//            ranges.push_back(num_of_elements - size * (num_of_producers - 1));
//            return ranges;
//        };

//        const auto ranges {split()};
////        std::cout << "ranges: ";
////        for(auto v:ranges)
////            std::cout << v << ' ';
////        std::cout << std::endl;
//        const auto data {make_data(num_of_producers, ranges)};
//        Queue<data_t> queue;

//        cond_cntr_t elements_left (num_of_elements);

//        std::vector<std::jthread> producers;
//        producers.reserve(num_of_producers);
//        const auto start {clock::now()};
//        for(auto& producers_data:data)
//            producers.emplace_back(producer, std::ref(queue), std::ref(producers_data), std::ref(elements_left));

//        std::vector<std::jthread> consumers;
//        consumers.reserve(num_of_consumers);
//        for(std::size_t cntr {0}; cntr < num_of_consumers; ++cntr)
//            consumers.emplace_back(consumer, std::ref(queue), std::ref(elements_left));

//        for(auto& cons:consumers)
//        {
//            if(cons.joinable())
//                cons.detach();
//        }
//        for(auto& prod:producers)
//        {
//            if(prod.joinable())
//                prod.detach();
//        }
//        while(elements_left || !queue.empty())
//            std::this_thread::sleep_for(10ms);
//        return std::make_pair(duration_cast<milliseconds>(clock::now() - start).count(), queue.empty());
//    };

//    try
//    {
//        constexpr std::size_t num_of_elements {1000};
//        /// \note one producers / consumers
//        const auto [single_duration, result] {run(1, 1, num_of_elements)};
//        ASSERT_TRUE(result);

//        for(std::size_t mfactor {1}; mfactor < 6; ++mfactor)
//        {
//            /// \note multiple producers / consumers. Number of consumers = number of producers.
//            const std::size_t num_of_producers { static_cast<std::remove_cv_t<decltype(num_of_producers)>>
//                (num_of_cores > 2 ? 0.5 * num_of_cores * mfactor: num_of_cores * mfactor)};
//            const auto [multiple_duration, result2]{run(num_of_producers, num_of_producers, num_of_elements)};
//            ASSERT_TRUE(result2);

//            std::cout << "duration single|multiple (ms): "
//                      << single_duration << '|' << multiple_duration
//                      << "\tnumber of producers|consumers: "
//                      << num_of_producers << '|' << num_of_producers
//                      << std::endl;
//            EXPECT_GE(single_duration, multiple_duration);
//        }
//    }
//    catch(const std::exception& e)
//    {
//        std::cerr << e.what() << std::endl;
//    }
//}


TEST(TEST_QUEUE, producer_consumer_framework)
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
    using queue_t = Queue<data_t>;

    auto make_data = [](std::size_t num_of_producers, const ranges_t& ranges)
    {
        all_data_collection_t data;
        data.reserve(num_of_producers);
        for(auto num_of_elements:ranges)
        {
            data_collection_t element;
            element.reserve(num_of_elements);
            for(std::size_t cntr {0}; cntr < num_of_elements; ++cntr)
                element.emplace_back(cntr * 13);
            data.emplace_back(std::move(element));
        }
        return data;
    };

    auto run = [&make_data]
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
        struct PData
        {
            PData(all_data_collection_t&& d):
                data{std::move(d)},
                free_data(data.size(), true)
            {}
            all_data_collection_t data;
            std::vector<bool> free_data;
            std::mutex mutex;
            std::condition_variable cv;
            bool available {true};
        } data{make_data(num_of_producers, ranges)};

        cond_cntr_t elements_left (num_of_elements);

        struct ProducerExc{};

        auto producer = [&data, &elements_left]
                ([[maybe_unused]] std::stop_token stop_token, queue_t& queue, std::size_t num)
        {
//            auto get_data = [&data]()->data_collection_t&
//            {
//                std::unique_lock lk {data.mutex};
//                data.cv.wait(lk, [&data]{ return data.available; });
//                data.available = false;
//                for(std::size_t cntr {0}; cntr < data.data.size(); ++cntr)
//                {
//                    if(data.free_data[cntr])
//                    {
//                        data.free_data[cntr] = false;
//                        data.available = true;
//                        data.cv.notify_all();
//                        return data.data[cntr];
//                    }
//                }
//                throw ProducerExc{};
//            };

//            const auto data {get_data()};
            const auto& d {data.data[num]};
            for(auto el:d)
            {
                queue.wait_if_full_push(el);
                --elements_left;
            }
        };
        auto consumer = [&elements_left]
                ([[maybe_unused]] std::stop_token stop_token, queue_t& queue)
        {
            while(!queue.empty() || elements_left)
            {
                auto el {queue.pop()};
                if(el)
                {
                    std::vector<data_t> v;
                    const std::size_t N {100000};
                    v.reserve(N);
                    for(std::size_t cntr {0}; cntr < N; ++cntr)
                        v.emplace_back(cntr + *el);
                }
            }
        };

        auto main_cycle = [&elements_left](queue_t& queue)
        {
            using namespace std::chrono_literals;
            while(!queue.empty() || elements_left)
            {
                // sleep
                // save queue (serialize)
                // etc.

                std::this_thread::sleep_for(10ms);
                //auto t {system_clock::to_time_t(system_clock::now())};
                //auto now {*std::localtime(&t)};
                //std::string time
                //{
                //    std::to_string(now.tm_year + 1900) + '.' +
                //            std::to_string(now.tm_mon + 1) + '.' +
                //            std::to_string(now.tm_mday) + '-' +
                //            std::to_string(now.tm_hour) + '.' +
                //            std::to_string(now.tm_min) + '.' +
                //            std::to_string(now.tm_sec)
                //};
                //using namespace serialization;
                //Serializer<queue_t, ArchiveType::TEXT> s{"qarchive-" + time + ".txt"};
                //s << queue;
            }
            while(!queue.empty() || elements_left)
                std::this_thread::sleep_for(10ms);
        };

        using namespace producer_consumer;
        Framework<data_t> framework
        {
            producer, num_of_producers,
            consumer, num_of_consumers,
            main_cycle
        };

        bool queue_empty {true};
        const auto start {clock::now()};
        try
        {
            framework.run();
        }
        catch(ProducerExc e)
        {
            std::cerr << "ProducerExc" << std::endl;
            queue_empty = false;
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            queue_empty = false;
        }
        return std::make_pair(duration_cast<milliseconds>(clock::now() - start).count(), queue_empty);
    };

    try
    {
        constexpr std::size_t num_of_elements {1000};
        /// \note one producers / consumers
        const auto [single_duration, result] {run(1, 1, num_of_elements)};
        ASSERT_TRUE(result);

        for(std::size_t mfactor {1}; mfactor < 6; ++mfactor)
        {
            /// \note multiple producers / consumers. Number of consumers = number of producers.
            const std::size_t num_of_producers { static_cast<std::remove_cv_t<decltype(num_of_producers)>>
                (num_of_cores > 2 ? 0.5 * num_of_cores * mfactor: num_of_cores * mfactor)};
            const auto [multiple_duration, result2]{run(num_of_producers, num_of_producers, num_of_elements)};
            ASSERT_TRUE(result2);

            std::cout << "duration single|multiple (ms): "
                      << single_duration << '|' << multiple_duration
                      << "\tnumber of producers|consumers: "
                      << num_of_producers << '|' << num_of_producers
                      << std::endl;
            EXPECT_GE(single_duration, multiple_duration);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

