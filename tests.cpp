#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cassert>
#include <numeric>
#include <iterator>
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

TEST(TEST_QUEUE, producer_consumer)
{
    // create input data
    // create producer
    // move input data into producer
    // create graph
    // create consumer, move graph into consumer
    // create framework
    // move producer and consumer into framework
    // run framework
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
