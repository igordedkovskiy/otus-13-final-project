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

    using ProducerT = void(std::stop_token stop_token, queue_t& queue, std::size_t num);
    using ConsumerT = void(std::stop_token stop_token, queue_t& queue);

    //using threads_cntr_t = std::atomic<std::size_t>;
    //using MainT = void(queue_t& queue, threads_cntr_t& producers_left, threads_cntr_t& consumers_left);
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
        m_num_of_consumers{num_of_consumers}
    {}

    ~Framework()
    {
        //if(m_queue.empty())
    }

    void run()
    {
        using threads_cntr_t = std::atomic<std::size_t>;
        threads_cntr_t producers_left {m_num_of_producers};
        threads_cntr_t consumers_left {m_num_of_consumers};

        auto producer_wrapper = [this, &producers_left](std::stop_token stop_token, std::size_t num)
        {
            m_producer(stop_token, m_queue, num);
            --producers_left;
        };
        auto consumer_wrapper = [this, &consumers_left](std::stop_token stop_token)
        {
            m_consumer(stop_token, m_queue);
            --consumers_left;
        };

        m_producers.reserve(m_num_of_producers);
        for(std::size_t cntr {0}; cntr < m_num_of_producers; ++cntr)
            //m_producers.emplace_back(m_producer, std::ref(m_queue), cntr);
            m_producers.emplace_back(producer_wrapper, cntr);

        m_consumers.reserve(m_num_of_consumers);
        for(std::size_t cntr {0}; cntr < m_num_of_consumers; ++cntr)
            //m_consumers.emplace_back(m_consumer, std::ref(m_queue));
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

        m_main(m_queue);//, producers_left, consumers_left);
        while(consumers_left || producers_left)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

private:
    queue_t m_queue;

    std::function<ProducerT> m_producer;
    std::function<ConsumerT> m_consumer;
    std::function<MainT>     m_main;
    std::size_t m_num_of_producers {1};
    std::size_t m_num_of_consumers {1};

    std::vector<std::jthread> m_producers;
    std::vector<std::jthread> m_consumers;
};

}
