#pragma once

#include <string_view>
#include <array>
#include <utility>
#include <tuple>

namespace ctrie {

template<typename... Entries> class index_node;
template<typename T, typename IndexTrie> class array_map;

template<char... Ks>
struct insert_key {
    template<typename V>
    constexpr std::pair<insert_key, V> operator=(V&& value)
    {
        return std::make_pair(insert_key{}, std::forward<V>(value));
    }
};

constexpr struct default_key_t {
    template<typename V>
    constexpr std::pair<default_key_t, V> operator=(V&& value) const
    {
        return std::make_pair(default_key_t{}, std::forward<V>(value));
    }
} default_key{};

namespace literals {

template<typename T, T... Ks>
constexpr insert_key<Ks...> operator""_key() { return {}; }

} // namespace literals

namespace detail {

template<char K, typename Node> class entry;

template<typename FoundEntry, typename... RemainingEntries> struct found_tag { };
struct not_found_tag { };

template<typename... RemainingEntries>
struct search_record {
    template<typename Entry>
    constexpr search_record<RemainingEntries..., Entry> operator+(Entry) const { return {}; }

    template<typename Entry>
    static constexpr found_tag<Entry, RemainingEntries...> found(Entry) { return {}; }
};

template<char K, typename... Searched>
constexpr not_found_tag find_entry_impl(search_record<Searched...>) { return {}; }

template<char K, char K0, typename Node0, typename... Entries, typename... Searched>
constexpr auto find_entry_impl(
    search_record<Searched...> rec,
    entry<K0, Node0> e, Entries... es)
{
    using result_t = std::conditional_t<
        K == K0,
        decltype((rec + ... + es).found(e)),
        decltype(find_entry_impl<K>(rec + e, es...))
    >;
    return result_t{};
}

template<char K, typename... Entries>
constexpr auto find_entry(Entries... es) { return find_entry_impl<K>(search_record<>{}, es...); }

template<typename>
constexpr int size_until() { return 0; }

template<typename Entry, typename Entry0, typename... Entries>
constexpr int size_until(Entry0, Entries... es)
{
    if constexpr (std::is_same_v<Entry, Entry0>) {
        return 0;
    }
    return Entry0::size() + size_until<Entry>(es...);
}

template<typename T, typename IndexTrie, typename ValuesTup, std::size_t... KeyIdxs, std::size_t... ArrayIdxs>
constexpr auto construct_array_map(
    ValuesTup&& values,
    std::index_sequence<KeyIdxs...>,
    std::index_sequence<ArrayIdxs...>)
{
    constexpr auto find_key_idx = [](int arr_idx) -> std::size_t {
        std::size_t keyIdxs[] = {KeyIdxs...};
        for (int i = 0; i != sizeof...(KeyIdxs); ++i) {
            if (keyIdxs[i] == arr_idx) {
                return i;
            }
        }
        return sizeof...(KeyIdxs);
    };
    return array_map<T, IndexTrie>{
        std::get<find_key_idx(ArrayIdxs)>(std::forward<ValuesTup>(values))...
    };
}

template<char K, typename Node>
class entry {
public:
    static constexpr bool is_leaf() { return false; }
    static constexpr bool is_default() { return false; }

    static constexpr bool matches(char c) { return c == K; }
    static constexpr int size() { return Node::size(); }
    static constexpr int end() { return size(); }
    static constexpr int find(std::string_view s) { return Node::find(s); }

    template<char... Ks>
    static constexpr auto insert(insert_key<Ks...> k)
    {
        auto new_node = Node::insert(k);
        return entry<K, decltype(new_node)>{};
    }
};

struct leaf_node;
struct default_node;

template<>
class entry<0, leaf_node> {
public:
    static constexpr bool is_leaf() { return true; }
    static constexpr bool is_default() { return false; }
    static constexpr bool matches(char) { return false; }
    static constexpr int find(std::string_view) { return 0; }
    static constexpr int size() { return 1; }
    static constexpr int end() { return 1; }
};

template<>
class entry<0, default_node> {
public:
    static constexpr bool is_leaf() { return false; }
    static constexpr bool is_default() { return true; }
    static constexpr bool matches(char) { return false; }
    static constexpr int find(std::string_view) { return 0; }
    static constexpr int size() { return 0; }
    static constexpr int end() { return 0; }
};

using leaf_entry = entry<0, leaf_node>;
using default_entry = entry<0, default_node>;


} // namespace detail

template<typename... Entries>
class index_node {
public:
    static constexpr bool has_leaf() { return (Entries::is_leaf() || ...); }
    static constexpr bool has_default() { return (Entries::is_default() || ...); }

    template<char... Ks>
    constexpr auto operator<<(insert_key<Ks...> k) const { return insert(k); }
    constexpr auto operator<<(default_key_t k) const { return insert(k); }

    template<char K, char... Ks>
    static constexpr auto insert(insert_key<K, Ks...> k)
    {
        return insert_impl(k, detail::find_entry<K>(Entries{}...));
    }

    static constexpr auto insert(insert_key<>)
    {
        static_assert(!has_leaf(), "Inserted duplicate keys");
        return index_node<Entries..., detail::leaf_entry>{};
    }

    static constexpr auto insert(default_key_t)
    {
        static_assert(!has_default(), "Inserted multiple defaults");
        return index_node<Entries..., detail::default_entry>{};
    }

    static constexpr int size() { return (Entries::size() + ... + 0); }
    static constexpr int capacity() { return size() + static_cast<int>(has_default()); }
    static constexpr int end() { return size(); }

    static constexpr bool contains(std::string_view s) { return find(s) != end(); }

    static constexpr int find(std::string_view s)
    {
        if (!s.empty()) {
            int result{end()};
            char c = s.front();
            ((Entries::matches(c) && ((result = call_find<Entries>(s.substr(1))), true)) || ...);
            return result;
        }

        if constexpr (has_leaf()) {
            return call_find<detail::leaf_entry>({});
        }
        return end();
    }

    template<char... Ks>
    static constexpr int find(insert_key<Ks...>)
    {
        constexpr char s[] = {Ks...};
        return find(std::string_view{s, sizeof...(Ks)});
    }

    static constexpr int find(default_key_t)
    {
        static_assert(has_default());
        return end();
    }

private:
    template<typename Entry>
    static constexpr int call_find(std::string_view s)
    {
        auto idx = Entry::find(s);
        if (idx == Entry::end()) {
            return end();
        }
        return idx + detail::size_until<Entry>(Entries{}...);
    }

    template<char K, char... Ks, typename Found, typename... Remaining>
    static constexpr auto insert_impl(insert_key<K, Ks...>, detail::found_tag<Found, Remaining...>)
    {
        auto new_entry = Found::insert(insert_key<Ks...>{});
        return index_node<Remaining..., decltype(new_entry)>{};
    }

    template<char K, char... Ks>
    static constexpr auto insert_impl(insert_key<K, Ks...>, detail::not_found_tag)
    {
        auto new_entry = detail::entry<K, index_node<>>::insert(insert_key<Ks...>{});
        return index_node<Entries..., decltype(new_entry)>{};
    }
};

template<typename T, typename IndexTrie>
class array_map
{
public:
    using array_type = std::array<T, IndexTrie::capacity()>;
    using size_type = typename array_type::size_type;
    using value_type = typename array_type::value_type;
    using reference = typename array_type::reference;
    using const_reference = typename array_type::const_reference;
    using iterator = typename array_type::iterator;
    using const_iterator = typename array_type::const_iterator;

    template<typename... Us>
    constexpr explicit array_map(Us&&... us) :
        _data{{std::forward<Us>(us)...}}
    { }

    static constexpr bool empty() { return IndexTrie::size() == 0; }
    static constexpr size_type size() { return IndexTrie::size(); }
    static constexpr size_type capacity() { return IndexTrie::capacity(); }
    static constexpr size_type index_of(std::string_view s) { return IndexTrie::find(s); }

    constexpr reference operator[](std::string_view s) { return operator[](index_of(s)); }
    constexpr const_reference operator[](std::string_view s) const { return operator[](index_of(s)); }
    constexpr reference operator[](size_type pos) { return _data[pos]; }
    constexpr const_reference operator[](size_type pos) const { return _data[pos]; }

    constexpr iterator find(std::string_view s) { return std::next(std::begin(_data), index_of(s)); }
    constexpr const_iterator find(std::string_view s) const { return std::next(std::begin(_data), index_of(s)); }

    constexpr iterator begin() { return std::begin(_data); }
    constexpr const_iterator begin() const { return std::begin(_data); }
    constexpr iterator end() { return std::next(std::begin(_data), size()); }
    constexpr const_iterator end() const { return std::next(std::begin(_data), size()); }

    static constexpr bool has_default() { return IndexTrie::has_default(); }
    constexpr reference get_default() { return _data.back(); }
    constexpr const_reference get_default() const { return _data.back(); }

private:
    array_type _data;
};

template<typename... InsertKeys>
constexpr auto build_index(InsertKeys... keys) { return (index_node<>{} << ... << keys); }

template<typename T, typename... InsertKeyValues>
constexpr auto build_map(InsertKeyValues... kvs)
{
    constexpr auto index_trie = build_index(kvs.first...);
    return detail::construct_array_map<T, decltype(index_trie)>(
        std::forward_as_tuple(std::forward<typename InsertKeyValues::second_type>(kvs.second)...),
        std::index_sequence<index_trie.find(kvs.first)...>{},
        std::make_index_sequence<index_trie.capacity()>());
}

template<typename T, char... Ks, typename... InsertKeys>
constexpr auto build_map(insert_key<Ks...> key, InsertKeys... keys)
{
    constexpr auto index_trie = build_index(key, keys...);
    return array_map<T, decltype(index_trie)>{};
}

} // namespace ctrie
