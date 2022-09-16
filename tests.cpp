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
#include "serialization.hpp"


TEST(TEST_QUEUE, serializer_exceptions)
{
    using namespace serialization;
    using namespace threadsafe_containers;
    using value_type = int;
    using queue_t = Queue<value_type>;

    try
    {
        Serializer2<queue_t, ArchiveType::TEXT> s{"/temp/qarchive.txt"};
    }
    catch(const Exception& e)
    {
        std::string message {e.what()};
        EXPECT_EQ(message, std::string{"Nonexistent path"});
    }

    try
    {
        Serializer2<queue_t, ArchiveType::TEXT> s{"/temp/"};
    }
    catch(const Exception& e)
    {
        std::string message {e.what()};
        EXPECT_EQ(message, std::string{"Path doesn't contain file name"});
    }

    try
    {
        Serializer2<queue_t, ArchiveType::TEXT> s{fs::current_path()};
    }
    catch(const Exception& e)
    {
        std::string message {e.what()};
        EXPECT_EQ(message, std::string{"Path refers to a directory not a file"});
    }
}


TEST(TEST_QUEUE, serialize_queue_simple)
{
    using namespace serialization;
    namespace fs = std::filesystem;
    using namespace threadsafe_containers;
    using value_type = int;
    using queue_t = Queue<value_type>;

    auto test = [](ArchiveType fmt, const fs::path& path)
    {
        queue_t q;
        q.push(1);
        q.push(3);
        q.push(6);
        q.push(12);

        queue_t newq;

        std::ofstream os{path.string(), std::ofstream::out | std::ofstream::trunc};
        std::ifstream is{path.string(), std::ifstream::in};

        if(fmt == ArchiveType::BINARY)
        {
            {
                boost::archive::binary_oarchive ar{os};
                ar << q;
            }
            EXPECT_FALSE(q.empty());
            {
                boost::archive::binary_iarchive ar{is};
                ar >> newq;
            }
        }
        else if(fmt == ArchiveType::TEXT)
        {
            {
                boost::archive::text_oarchive ar{os};
                ar << q;
            }
            EXPECT_FALSE(q.empty());
            {
                boost::archive::text_iarchive ar{is};
                ar >> newq;
            }
        }
        else if(fmt == ArchiveType::XML)
        {
            {
                boost::archive::xml_oarchive ar{os};
                //ar << boost::serialization::make_nvp("q", q);
                ar << BOOST_SERIALIZATION_NVP(q);
            }
            EXPECT_FALSE(q.empty());
            {
                boost::archive::xml_iarchive ar{is};
                //ar >> boost::serialization::make_nvp("q", newq);
                ar >> BOOST_SERIALIZATION_NVP(newq);
            }
        }

        // compare newq against q
        EXPECT_EQ(newq, q);
        EXPECT_FALSE(q.empty());
        EXPECT_FALSE(newq.empty());
        while(!newq.empty())
        {
            auto newq_p = newq.pop();
            auto q_p = q.pop();
            //std::cout << "newq.top: " << std::setw(3) << *newq_p
            //          << "; q.top: " << std::setw(3) << *q_p << '\n';
            EXPECT_EQ(*newq_p, *q_p);
        }
    };
    test(ArchiveType::BINARY, "qarchive1");
    test(ArchiveType::TEXT,   "qarchive1.txt");
    test(ArchiveType::XML,    "qarchive1.xml");
}


TEST(TEST_QUEUE, serialize_queue)
{
    using namespace serialization;
    namespace fs = std::filesystem;
    using namespace threadsafe_containers;
    using value_type = int;
    using queue_t = Queue<value_type>;

    auto test = [](auto serializer)
    {
        queue_t q;
        q.push(1);
        q.push(3);
        q.push(6);
        q.push(12);

        serializer.clear();
        serializer << q;
        EXPECT_FALSE(q.empty());
        // create newq and load it's value from an archive
        queue_t newq;
        serializer >> newq;
        // compare newq against q
        EXPECT_EQ(newq, q);
        EXPECT_FALSE(q.empty());
        EXPECT_FALSE(newq.empty());
        while(!newq.empty())
        {
            auto newq_p = newq.pop();
            auto q_p = q.pop();
            //std::cout << "newq.top: " << std::setw(3) << *newq_p
            //          << "; q.top: " << std::setw(3) << *q_p << '\n';
            EXPECT_EQ(*newq_p, *q_p);
        }
    };
    test(serialization::Serializer2<queue_t, ArchiveType::BINARY>{"qarchive2"});
    test(serialization::Serializer2<queue_t, ArchiveType::TEXT  >{"qarchive2.txt"});
    test(serialization::Serializer2<queue_t, ArchiveType::XML   >{"qarchive2.xml"});
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

        std::cout << "duration single|multiple (ms): "
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
