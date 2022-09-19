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
template<typename T, std::size_t SIZE = 5> class Queue
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

    void notify_on_not_empty()
    {
        if(m_queue.size() == 1)
//        if(m_queue.size())
            m_on_not_empty.notify_all();
    }

    void notify_on_space_available()
    {
        if(m_queue.size() == SIZE - 1)
//        if(m_queue.size() < SIZE)
            m_on_space_available.notify_all();
    }

    /// \brief  Push value into queue
    /// \return False if queue has no space left to push \b v, true otherwise.
    [[nodiscard]] bool push(T v)
    {
        std::scoped_lock lk {m_mutex};
        if(full_nonblocking())
            return false;
        m_queue.emplace_back(std::move(v));
        notify_on_not_empty();
        return true;
    }

    /// \brief  Dequeue element and place it's value into \b v.
    /// \return False if queue is empty, \b v keeps it's value.
    ///         True otherwise, \b v contains dequeued value.
    [[nodiscard]] bool pop(T& v)
    {
        std::scoped_lock lk {m_mutex};
        if(m_queue.empty())
            return false;
        v = std::move(m_queue.front());
        m_queue.pop_front();
        notify_on_space_available();
        return true;
    }

    /// \brief  Dequeue element and return it's value.
    /// \return nullptr if queue is empty, dequeued value otherwise.
    [[nodiscard]] pointer_type pop()
    {
        std::scoped_lock lk {m_mutex};
        if(m_queue.empty())
            return nullptr;
        auto p {std::make_unique<T>(m_queue.front())};
        m_queue.pop_front();
        notify_on_space_available();
        return p;
    }

    /// \brief Wait until queue is empty.
    void wait_until_empty()
    {
        std::unique_lock lk {m_mutex};
        while(m_queue.empty())
            m_on_not_empty.wait(lk, [this]{ return !m_queue.empty(); });
    }

    /// \brief Wait until queue is full.
    void wait_until_full()
    {
        std::unique_lock lk {m_mutex};
        while(full_nonblocking())
            m_on_space_available.wait(lk, [this]{ return !full_nonblocking(); });
    }

    /// \return True if queue is empty, false otherwise.
    [[nodiscard]] bool empty() const
    {
        std::scoped_lock lk {m_mutex};
        return m_queue.empty();
    }

    /// \return True if queue is false, false otherwise.
    [[nodiscard]] bool full() const
    {
        std::scoped_lock lk {m_mutex};
        return full_nonblocking();
    }

    /// \brief  Wait if queue is full, push \b v into queue.
    void wait_and_push(T v)
    {
        std::unique_lock lk {m_mutex};
        // condition_variable::wait atomically unlocks lk, blocks the current executing thread,
        // and adds it to the list of threads waiting on *this. The thread will be unblocked
        // when notify_all() or notify_all() is executed. It may also be unblocked spuriously.
        // When unblocked, regardless of the reason, lock is reacquired and wait exits.
        // Thus, deadlock is impossible.
        // Overload with predicate may be used to ignore spurious awakenings.
        while(full_nonblocking())
            m_on_space_available.wait(lk, [this]{ return !full_nonblocking(); });
//            m_on_space_available.wait(lk, [this]{ return m_queue.size() < SIZE; });
        m_queue.emplace_back(std::move(v));
        notify_on_not_empty();
    }

    /// \brief Wait until queue is empty, dequeue element and place it's value into \b v.
    void wait_and_pop(T& v)
    {
        std::unique_lock lk {m_mutex};
        while(m_queue.empty())
            m_on_not_empty.wait(lk, [this]{ return !m_queue.empty(); });
        v = std::move(m_queue.front());
        m_queue.pop_front();
        notify_on_space_available();
    }

    /// \brief Wait until queue is empty, dequeue element and return it's value.
    [[nodiscard]] pointer_type wait_and_pop()
    {
        std::unique_lock lk {m_mutex};
        while(m_queue.empty())
            m_on_not_empty.wait(lk, [this]{ return !m_queue.empty(); });
        auto p {std::make_unique<T>(m_queue.front())};
        m_queue.pop_front();
        notify_on_space_available();
        return p;
    }

    void clear()
    {
        std::scoped_lock lk {m_mutex};
        m_queue.clear();
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_queue.size();
    }

    [[nodiscard]] constexpr std::size_t max_size() const noexcept
    {
        return SIZE;
    }

    [[nodiscard]] friend bool operator==(const Queue& l, const Queue& r)
    {
        return l.m_queue == r.m_queue;
    }

private:
    [[nodiscard]] bool full_nonblocking() const noexcept
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
    mutable std::mutex m_mutex;
};

}
