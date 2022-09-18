#pragma once

#include <cstdlib>
#include <memory>
#include <deque>
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <fstream>

#include "Queue.hpp"

namespace producer_consumer
{

struct PCException{};

template<typename T> class Framework
{
public:
    using queue_t = threadsafe_containers::Queue<T>;

    using ProducerT = void(std::stop_token stop_token, queue_t& queue);
    using ConsumerT = void(std::stop_token stop_token, queue_t& queue);
    using MainT = void(queue_t& queue);

public:
    template<typename P, typename C, typename M>
    Framework(const P& producer, std::size_t num_of_producers,
              const C& consumer, std::size_t num_of_consumers,
              const M& main_cycle):
        m_producer{producer},
        m_consumer{consumer},
        m_main{main_cycle},
        m_num_of_producers{num_of_producers},
        m_num_of_consumers{num_of_consumers},
        producers_left{num_of_producers},
        consumers_left{num_of_consumers}
    {}

    ~Framework()
    {
        if(producers_left)
        {
            for(auto& t:m_producers)
                t.request_stop();
        }
        if(consumers_left)
        {
            for(auto& t:m_consumers)
                t.request_stop();
        }
        wait();
    }

    void run()
    {
        producers_left = m_num_of_producers;
        consumers_left = m_num_of_consumers;

        auto producer_wrapper = [this](std::stop_token stop_token)
        {
            m_producer(stop_token, m_queue);
            --producers_left;
        };
        auto consumer_wrapper = [this](std::stop_token stop_token)
        {
            m_consumer(stop_token, m_queue);
            --consumers_left;
        };

        m_producers.reserve(m_num_of_producers);
        for(std::size_t cntr {0}; cntr < m_num_of_producers; ++cntr)
            m_producers.emplace_back(producer_wrapper);

        m_consumers.reserve(m_num_of_consumers);
        for(std::size_t cntr {0}; cntr < m_num_of_consumers; ++cntr)
            m_consumers.emplace_back(consumer_wrapper);

        for(auto& prod:m_producers)
        {
            if(prod.joinable())
                prod.detach();
        }
        for(auto& cons:m_consumers)
        {
            if(cons.joinable())
                cons.detach();
        }

        m_main(m_queue);
        wait();
    }

private:
    using threads_cntr_t = std::atomic<std::size_t>;

    /// \brief wait until consumers and producers finish their work
    void wait()
    {
        while(consumers_left || producers_left)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    queue_t m_queue;

    std::function<ProducerT> m_producer;
    std::function<ConsumerT> m_consumer;
    std::function<MainT>     m_main;
    std::size_t m_num_of_producers {1};
    std::size_t m_num_of_consumers {1};

    std::vector<std::jthread> m_producers;
    std::vector<std::jthread> m_consumers;

    threads_cntr_t producers_left {0};
    threads_cntr_t consumers_left {0};
};

}
