#pragma once

#include <filesystem>
#include <fstream>
#include <exception>
#include <string>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace serialization
{

namespace fs = std::filesystem;

class Exception: public std::exception
{
public:
    Exception(const char* message);
    const char* what() const noexcept override;
private:
    std::string m_message;
};

//enum class ArchiveFormat: std::uint8_t
//{
//    BIN,
//    TXT,
//    XML
//};

//template<typename T, ArchiveFormat FMT> class Serializer
template<typename T> class Serializer
{
public:
    Serializer(fs::path path):
        m_path{std::move(path)}
    {
        if(!fs::exists(m_path))
            throw Exception{"Nonexistent path"};
        m_error = false;
    }

    void clear()
    {
        std::ofstream stream{this->path().string(), std::ofstream::out | std::ofstream::trunc};
    }

    bool set(fs::path path) noexcept
    {
        if(!fs::exists(path))
        {
            m_error = true;
            return false;
        }
        m_path = std::move(path);
        m_error = false;
    }

    bool valid() const noexcept
    {
        return !m_error;
    }

    void operator<<(const T& q)
    {
        if(!valid())
            return;
        serialize(q);

//        if constexpr(FMT == ArchiveFormat::BIN)
//        {
//            ;
//        }
//        else if constexpr(FMT == ArchiveFormat::TXT)
//        {
//            ;
//        }
//        else if constexpr(FMT == ArchiveFormat::XML)
//        {
//            ;
//        }
    }

    void operator>>(T& q)
    {
        if(!valid())
            return;
        deserialize(q);
    }

protected:
    const fs::path& path() const noexcept
    {
        return m_path;
    }

private:
    virtual void serialize(const T& q) = 0;
    virtual void deserialize(T& q) = 0;
    fs::path m_path;
    bool m_error {true};
};

template<typename T> class BINSerializer: public Serializer<T>
{
public:
    BINSerializer(fs::path path): Serializer<T>{path} {}

private:
    void serialize(const T& q) override
    {
        std::ofstream stream{this->path().string()};
        boost::archive::binary_oarchive ar{stream};
        ar << q;
    }

    void deserialize(T& q) override
    {
        std::ifstream stream{this->path().string()};
        boost::archive::binary_iarchive ar{stream};
        ar >> q;
    }
};

template<typename T> class TXTSerializer: public Serializer<T>
{
public:
    TXTSerializer(fs::path path): Serializer<T>{path} {}

private:
    void serialize(const T& q) override
    {
        std::ofstream stream{this->path().string()};
        boost::archive::text_oarchive ar{stream};
        ar << q;
    }

    void deserialize(T& q) override
    {
        std::ifstream stream{this->path().string()};
        boost::archive::text_iarchive ar{stream};
        ar >> q;
    }
};

template<typename T> class XMLSerializer: public Serializer<T>
{
public:
    XMLSerializer(fs::path path): Serializer<T>{path} {}

private:
    void serialize(const T& q) override
    {
        std::ofstream stream{this->path().string()};
        boost::archive::xml_oarchive ar{stream};
        ar << boost::serialization::make_nvp("queue", q);
    }

    void deserialize(T& q) override
    {
        std::ifstream stream{this->path().string()};
        boost::archive::xml_iarchive ar{stream};
        ar >> boost::serialization::make_nvp("queue", q);
    }
};

}
