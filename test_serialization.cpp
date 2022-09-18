
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
        try
        {
            Serializer<queue_t, ArchiveType::TEXT> s{"/temp/qarchive.txt"};
        }
        catch(const Exception& e)
        {
            std::string message {e.what()};
            EXPECT_EQ(message, std::string{"Nonexistent path"});
        }

        try
        {
            Serializer<queue_t, ArchiveType::TEXT> s{"/temp/"};
        }
        catch(const Exception& e)
        {
            std::string message {e.what()};
            EXPECT_EQ(message, std::string{"Path doesn't contain file name"});
        }

        try
        {
            Serializer<queue_t, ArchiveType::TEXT> s{fs::current_path()};
        }
        catch(const Exception& e)
        {
            std::string message {e.what()};
            EXPECT_EQ(message, std::string{"Path refers to a directory not a file"});
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
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
    try
    {
        test(ArchiveType::BINARY, "qarchive1");
        test(ArchiveType::TEXT,   "qarchive1.txt");
        test(ArchiveType::XML,    "qarchive1.xml");
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
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
    try
    {
        test(serialization::Serializer<queue_t, ArchiveType::BINARY>{"qarchive2"});
        test(serialization::Serializer<queue_t, ArchiveType::TEXT  >{"qarchive2.txt"});
        test(serialization::Serializer<queue_t, ArchiveType::XML   >{"qarchive2.xml"});
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

