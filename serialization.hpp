#pragma once

#include <filesystem>
#include <fstream>
#include <exception>
#include <string>
#include <bitset>

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

template<typename T> class Serializer
{
public:
    Serializer() noexcept = default;

    /// \throws std::filesystem::filesystem_error, serialization::Exception
    Serializer(const fs::path& path)
    {
        set_file_name(path);
    }

    virtual ~Serializer() = default;

    /// \throws The same exceptions as std::fstream
    void clear()
    {
        std::ofstream stream{m_fname, std::ofstream::out | std::ofstream::trunc};
    }

    /// \throws std::filesystem::filesystem_error, serialization::Exception
    void set_file_name(const fs::path& path)
    {
        if(fs::is_directory(path))
            throw Exception{"Path refers to a directory not a file"};
        if(!path.has_filename())
            throw Exception{"Path doesn't contain file name"};

        auto dir {path};
        dir.remove_filename();
        if(!dir.empty())
        {
            if(!fs::exists(dir))
                throw Exception{"Nonexistent path"};
        }

        // File may be absent, but a path to the file must be valid
        m_fname = path.string();
        if(!fs::exists(path))
        {
            using stream_t = std::ofstream;
            stream_t stream{m_fname, stream_t::out | stream_t::trunc};
        }
    }

    /// \throws The same exceptions as std::fstream
    Serializer<T>& operator<<(const T& q)
    {
        using stream_t = std::ofstream;
        stream_t stream{m_fname, stream_t::out | stream_t::app};
        serialize(q, stream);
        return *this;
    }

    /// \throws The same exceptions as std::fstream
    Serializer<T>& operator>>(T& q)
    {
        using stream_t = std::ifstream;
        stream_t stream{m_fname, stream_t::in};
        deserialize(q, stream);
        return *this;
    }

private:
    virtual void serialize(const T& q, std::ofstream& stream) = 0;
    virtual void deserialize(T& q, std::ifstream& stream) = 0;

    std::string m_fname;
};

template<typename T> class BINSerializer: public Serializer<T>
{
public:
    BINSerializer() noexcept = default;
    BINSerializer(fs::path path): Serializer<T>{path} {}

private:
    void serialize(const T& q, std::ofstream& stream) override
    {
        boost::archive::binary_oarchive ar{stream};
        ar << q;
    }

    void deserialize(T& q, std::ifstream& stream) override
    {
        boost::archive::binary_iarchive ar{stream};
        ar >> q;
    }
};

template<typename T> class TXTSerializer: public Serializer<T>
{
public:
    TXTSerializer() noexcept = default;
    TXTSerializer(fs::path path): Serializer<T>{path} {}

private:
    void serialize(const T& q, std::ofstream& stream) override
    {
        boost::archive::text_oarchive ar{stream};
        ar << q;
    }

    void deserialize(T& q, std::ifstream& stream) override
    {
        boost::archive::text_iarchive ar{stream};
        ar >> q;
    }
};


template<typename T> class XMLSerializer: public Serializer<T>
{
public:
    XMLSerializer() noexcept = default;
    XMLSerializer(fs::path path): Serializer<T>{path} {}

private:
    void serialize(const T& q, std::ofstream& stream) override
    {
        boost::archive::xml_oarchive ar{stream};
        //ar << boost::serialization::make_nvp("data", q);
        ar << BOOST_SERIALIZATION_NVP(q);
    }

    void deserialize(T& q, std::ifstream& stream) override
    {
        boost::archive::xml_iarchive ar{stream};
        //ar >> boost::serialization::make_nvp("data", q);
        ar >> BOOST_SERIALIZATION_NVP(q);
    }
};

}
