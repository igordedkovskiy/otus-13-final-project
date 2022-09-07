// homework #11: mapreduce prefix

#include <iostream>
#include <type_traits>
#include <cassert>
#include <utility>
#include <chrono>
#include <exception>

/// \brief Enables std::pair serialization
#include <boost/serialization/utility.hpp>

//#include "input.hpp"
#include "Queue.hpp"

struct Exception: public std::exception
{
    Exception(std::string s): m_m(std::move(s)) {}
    const char* what() const noexcept override { return m_m.c_str(); }
    std::string m_m;
};

//int main(int argc, const char* argv[])
int main()
{
    std::locale::global(std::locale(""));
    try
    {
//        auto [opts, res] {parse_cmd_line(argc, argv)};
//        if(!res)
//            return 0;
        using type = std::pair<std::size_t, int>;

        auto push = [](threadsafe_containers::Queue<type>& q, std::mutex& m, std::size_t n)
        {
            //const auto th_id {std::hash<std::thread::id>()(std::this_thread::get_id())};
            for(std::size_t cntr = 0; ; ++cntr)
            {
                q.wait_if_full_push(std::make_pair(n, cntr));
                std::scoped_lock lk{m};
                std::cout << "push " << n << ':' << cntr << " size: " << q.size() << '\n';
            }
        };
        auto pull = [](threadsafe_containers::Queue<type>& q, std::mutex& m)
        {
            while(true)
            {
                auto p {q.wait_until_empty_pop()};
                std::scoped_lock lk{m};
                std::cout << "pull " << p->first << ':' << p->second << " size: " << q.size() << '\n';
            }
        };

        threadsafe_containers::Queue<type> q;
        std::mutex m;
        std::thread producer1{push, std::ref(q), std::ref(m), 1};
        std::thread producer2{push, std::ref(q), std::ref(m), 2};
        std::thread producer3{push, std::ref(q), std::ref(m), 3};
        std::thread consumer1{pull, std::ref(q), std::ref(m)};
        producer1.detach();
        producer2.detach();
        producer3.detach();
        consumer1.detach();
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(5ms);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
