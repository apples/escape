#ifndef RESOURCEPOOL_HPP
#define RESOURCEPOOL_HPP

#include <unordered_map>
#include <string>

class ResourcePoolError
    : public std::exception
{
    std::string str;

    template <typename S>
    static void fill_stream(S& s)
    {}

    template <typename S, typename T, typename... Vs>
    static void fill_stream(S& s, T&& t, Vs&&... vs)
    {
        s << std::forward<T>(t);
        fill_stream(s, std::forward<Vs>(vs)...);
    }

    template <typename... Ts>
    static std::string make_string(Ts&&... ts)
    {
        std::stringstream ss;
        fill_stream(ss, std::forward<Ts>(ts)...);
        return ss.str();
    }

public:
    ResourcePoolError()
        : ResourcePoolError("Unknown error.")
    {}

    template <typename T, typename... Vs>
    ResourcePoolError(T&& t, Vs&&... vs)
        : str(make_string("ResourcePoolError: ", std::forward<T>(t), std::forward<Vs>(vs)...))
    {}

    const char* what() const noexcept override
    {
        return str.c_str();
    }
};

template <typename Resource, typename Key = std::string, typename Map = std::unordered_map<Key, Resource>>
class ResourcePool
{
    Map data;

public:
    template <typename K>
    Resource& create(K&& k)
    {
        auto iter = data.find(k);
        if (iter != data.end()) throw ResourcePoolError("Unable to create: already exists!");
        return data.emplace(std::forward<K>(k), Resource{}).first->second;
    }

    template <typename K, typename V>
    Resource& create(K&& k, V&& v)
    {
        auto iter = data.find(k);
        if (iter != data.end()) throw ResourcePoolError("Unable to create: already exists!");
        return data.emplace(std::forward<K>(k), std::forward<V>(v)).first->second;
    }

    template <typename K, typename... Vs>
    Resource& create(K&& k, Vs&&... vs)
    {
        auto iter = data.find(k);
        if (iter != data.end()) throw ResourcePoolError("Unable to create: already exists!");
        return data.emplace(std::forward<K>(k), Resource{std::forward<Vs>(vs)...}).first->second;
    }

    Resource& get(Key const& k)
    {
        auto iter = data.find(k);
        if (iter == data.end()) throw ResourcePoolError("Entry not found!");
        return iter->second;
    }

    Resource const& get(Key const& k) const
    {
        auto iter = data.find(k);
        if (iter == data.end()) throw ResourcePoolError("Entry not found!");
        return iter->second;
    }

    void erase(Key const& k)
    {
        auto iter = data.find(k);
        if (iter == data.end()) throw ResourcePoolError("Entry not found!");
        data.erase(iter);
    }

    void clear()
    {
        data.clear();
    }

    typename Map::size_type size() const
    {
        return data.size();
    }
};

#endif // RESOURCEPOOL_HPP
