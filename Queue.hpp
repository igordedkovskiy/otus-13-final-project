//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <cstdlib>
#include <memory>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace threadsafe_containers
{

/// \brief Simple threadsafe queue
template<typename T, std::size_t SIZE = 10> class Queue
{
public:
    using pointer_type = std::unique_ptr<T>;

    /// \brief  Push value into queue
    /// \return False if queue has no space left to push \b v, true otherwise.
    bool push(T v)
    {
        {
            std::scoped_lock lk {m_mutex};
            if(full())
                return false;
            m_queue.emplace_back(std::move(v));
        }
        if(m_queue.size() == 1)
            m_on_not_empty.notify_one();
        return true;
    }

    /// \brief  Dequeue element and place it's value into \b v.
    /// \return False if queue is empty, \b v keeps it's value.
    ///         True otherwise, \b v contains dequeued value.
    bool pop(T& v)
    {
        std::scoped_lock lk {m_mutex};
        if(m_queue.empty())
            return false;
        v = std::move(m_queue.front());
        m_queue.pop_front();
        if(m_queue.size() == SIZE - 1)
            m_on_space_available.notify_one();
        return true;
    }

    /// \brief  Dequeue element and return it's value.
    /// \return nullptr if queue is empty, dequeued value otherwise.
    pointer_type pop()
    {
        std::scoped_lock lk {m_mutex};
        if(m_queue.empty())
            return nullptr;
        auto p {std::make_unique<T>(m_queue.front())};
        m_queue.pop_front();
        if(m_queue.size() == SIZE - 1)
            m_on_space_available.notify_one();
        return p;
    }

    /// \brief Wait until queue is empty.
    void wait_until_empty()
    {
       while(m_queue.empty())
       {
           std::unique_lock lk {m_mutex};
           m_on_not_empty.wait(lk, [this]{ return !m_queue.empty(); });
       }
    }

    /// \return True is queue is empty, false otherwise.
    bool empty() const
    {
        std::scoped_lock lk {m_mutex};
        return m_queue.empty();
    }

    /// \brief  Wait if queue is full, push \b v into queue.
    void wait_if_full_push(T v)
    {
        while(true)
        {
            std::unique_lock lk {m_mutex};
            // condition_variable::wait atomically unlocks lk, blocks the current executing thread,
            // and adds it to the list of threads waiting on *this. The thread will be unblocked
            // when notify_all() or notify_one() is executed. It may also be unblocked spuriously.
            // When unblocked, regardless of the reason, lock is reacquired and wait exits.
            // Thus, deadlock is impossible.
            m_on_space_available.wait(lk, [this]{ return !full(); });
            if(!full())
            {
                m_queue.push_back(std::move(v));
                return;
            }
        }
    }

    /// \brief Wait until queue is empty, dequeue element and place it's value into \b v.
    void wait_until_empty_pop(T& v)
    {
        while(true)
        {
            std::unique_lock lk {m_mutex};
            m_on_not_empty.wait(lk, [this]{ return !m_queue.empty(); });
            if(!m_queue.empty())
            {
                v = std::move(m_queue.front());
                m_queue.pop_front();
                if(m_queue.size() == SIZE - 1)
                    m_on_space_available.notify_one();
                return;
            }
        }
    }

    /// \brief Wait until queue is empty, dequeue element and return it's value.
    pointer_type wait_until_empty_pop()
    {
        while(true)
        {
            std::unique_lock lk {m_mutex};
            m_on_not_empty.wait(lk, [this]{ return !m_queue.empty(); });
            if(!m_queue.empty())
            {
                auto p {std::make_unique<T>(m_queue.front())};
                m_queue.pop_front();
                if(m_queue.size() == SIZE - 1)
                    m_on_space_available.notify_one();
                return p;
            }
        }
    }

    std::size_t size() const noexcept
    {
        return m_queue.size();
    }

    constexpr std::size_t max_size() const noexcept
    {
        return SIZE;
    }

private:
    bool full() const noexcept
    {
        return !(m_queue.size() < SIZE);
    }

    using queue_t = std::deque<T>;
    queue_t m_queue;
    std::condition_variable m_on_not_empty;
    std::condition_variable m_on_space_available;
    std::mutex m_mutex;
};

}
