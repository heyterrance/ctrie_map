# Compile-time Trie Map
A map backed by a `std::array`, indexed by a compile-time trie.

## Features
* Header-only
* Fully `constexpr`
* _Technically_ more useful than [ctrie](https://github.com/heyterrance/ctrie)

## Usage

```cpp
#include "ctrie_map.h"

using namespace ctrie::literals;

auto m = ctrie::build_array_map<int>(
    "A"_key = 0,
    "AA"_key = 0,
    "B"_key = 0,
    "BC"_key = 0,
);

int get(std::string_view s)
{
    // UB if `s1 is not a valid key
    return m[s];
}

void update(std::string_view s, int i)
{
    if (auto it = m.find(s); it != m.end()) {
        *it += i;
    }
}

void iterate()
{
    // Order of elements is stable, yet undefined
    for (int i : m) {
        std::cout << i << '\n';
    }
}
```

### Default values 
```cpp
constexpr auto m = ctrie::build_array_map<int>(
    "A"_key = 5,
    ctrie::default_key = 0xBEEF
);

// Default value is stored in an additional element at the end of the array
static_assert(m.size() == 1 && m.capacity() == 2);
static_assert(m.has_default());
static_assert(m["???"] == 0xBEEF);
```

## Performance
Yes 

## Dependencies
* C++17