#pragma once

#include <cstdlib>
#include <memory>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <fstream>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/deque.hpp>

namespace threadsafe_containers
{

namespace fs = std::filesystem;

/// \brief Simple threadsafe queue
template<typename T, std::size_t SIZE = 10> class Queue
{
public:
    using value_type = T;
    using pointer_type = std::unique_ptr<T>;

    Queue() = default;

    Queue(const Queue&) = delete;
    Queue(Queue&&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue& operator=(Queue&&) = delete;

    ~Queue() = default;

    /// \brief  Push value into queue
    /// \return False if queue has no space left to push \b v, true otherwise.
    bool push(T v)
    {
        std::scoped_lock lk {m_mutex};
        if(full())
            return false;
        m_queue.emplace_back(std::move(v));
        if(m_queue.size() == 1)
            m_on_not_empty.notify_all();
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
            m_on_space_available.notify_all();
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
            m_on_space_available.notify_all();
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

    /// \return True if queue is empty, false otherwise.
    bool empty()
    {
        std::scoped_lock lk {m_mutex};
        return m_queue.empty();
    }

    /// \brief  Wait if queue is full, push \b v into queue.
    void wait_if_full_push(T v)
    {
        std::cout << "wait_if_full_push: lock" << std::endl;
        std::unique_lock lk {m_mutex};
        // condition_variable::wait atomically unlocks lk, blocks the current executing thread,
        // and adds it to the list of threads waiting on *this. The thread will be unblocked
        // when notify_all() or notify_all() is executed. It may also be unblocked spuriously.
        // When unblocked, regardless of the reason, lock is reacquired and wait exits.
        // Thus, deadlock is impossible.
        // Overload with predicate may be used to ignore spurious awakenings.
        std::cout << "wait_if_full_push: wait" << std::endl;
        m_on_space_available.wait(lk, [this]{ return !full(); });
        std::cout << "wait_if_full_push: wait end" << std::endl;
        m_queue.emplace_back(std::move(v));
        if(m_queue.size() == 1)
        {
            std::cout << "wait_if_full_push: notify" << std::endl;
            m_on_not_empty.notify_all();
        }
    }

    /// \brief Wait until queue is empty, dequeue element and place it's value into \b v.
    void wait_until_empty_pop(T& v)
    {
        std::unique_lock lk {m_mutex};
        m_on_not_empty.wait(lk, [this]{ return !m_queue.empty(); });
        v = std::move(m_queue.front());
        m_queue.pop_front();
        if(m_queue.size() == SIZE - 1)
            m_on_space_available.notify_all();
    }

    /// \brief Wait until queue is empty, dequeue element and return it's value.
    pointer_type wait_until_empty_pop()
    {
        std::unique_lock lk {m_mutex};
        m_on_not_empty.wait(lk, [this]{ return !m_queue.empty(); });
        auto p {std::make_unique<T>(m_queue.front())};
        m_queue.pop_front();
        if(m_queue.size() == SIZE - 1)
            m_on_space_available.notify_all();
        return p;
    }

    void clear() noexcept
    {
        std::scoped_lock lk {m_mutex};
        m_queue.clear();
    }

    std::size_t size() const noexcept
    {
        return m_queue.size();
    }

    constexpr std::size_t max_size() const noexcept
    {
        return SIZE;
    }

    friend bool operator==(const Queue& l, const Queue& r)
    {
        return l.m_queue == r.m_queue;
    }

private:
    bool full() const noexcept
    {
        return !(m_queue.size() < SIZE);
    }


    friend class boost::serialization::access;
    // When the class Archive corresponds to an output archive, the
    // & operator is defined similar to <<.  Likewise, when the class Archive
    // is a type of input archive the & operator is defined similar to >>.
    template<class Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned int version)
    {
        std::scoped_lock lk {m_mutex};

        constexpr bool is_text_or_bin_arc {
            std::is_same_v<Archive, boost::archive::text_oarchive> ||
            std::is_same_v<Archive, boost::archive::text_iarchive> ||
            std::is_same_v<Archive, boost::archive::binary_oarchive> ||
            std::is_same_v<Archive, boost::archive::binary_iarchive>
        };

        constexpr bool is_xml_arc {
            std::is_same_v<Archive, boost::archive::xml_oarchive> ||
            std::is_same_v<Archive, boost::archive::xml_iarchive>
        };

        if constexpr(is_text_or_bin_arc)
        {
            ar & m_queue;
        }
        else if constexpr(is_xml_arc)
        {
            using boost::serialization::make_nvp;
            //ar & make_nvp("queue", m_queue);
            ar & BOOST_SERIALIZATION_NVP(m_queue);
        }
    }


    using queue_t = std::deque<T>;
    queue_t m_queue;
    std::condition_variable m_on_not_empty;
    std::condition_variable m_on_space_available;
    std::mutex m_mutex;
};

}
