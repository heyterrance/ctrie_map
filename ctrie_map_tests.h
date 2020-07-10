#pragma once

#include "ctrie_map.h"

namespace ctrie_test {

using namespace ctrie::literals;

struct index_trie_tests 
{
    struct no_insertions {
        static_assert(ctrie::build_index().size() == 0);
    };

    struct single_insertion {
        static constexpr auto theVictim = ctrie::build_index("A"_key);
        static_assert(theVictim.size() == 1);
        static_assert(theVictim.find("A") == 0);
        static_assert(theVictim.find("AA") == 1);
        static_assert(theVictim.find("B") == 1);
    };

    struct multi_insertions {
        static constexpr auto theVictim = ctrie::build_index
            ( "A"_key
            , "B"_key
            , "AB"_key
            , "AA"_key
            , "CD"_key
            );
        static_assert(theVictim.size() == 5);
        static_assert(theVictim.contains("A"));
        static_assert(theVictim.contains("AA"));
        static_assert(!theVictim.contains("AAA"));
    };
};

struct array_map_tests 
{
    struct no_insertions {
        static constexpr auto theVictim = ctrie::build_map<int>();
        static_assert(theVictim.size() == 0);
        static_assert(theVictim.capacity() == 0);
        static_assert(!theVictim.has_default());
    };

    struct single_insertion {
        static constexpr auto theVictim = ctrie::build_map<int>("A"_key = 0xA);
        static_assert(theVictim.size() == 1);
        static_assert(theVictim.capacity() == 1);
        static_assert(!theVictim.has_default());
        static_assert(theVictim[0] == 0xA);
        static_assert(theVictim["A"] == 0xA);
        static_assert(theVictim.find("B") == theVictim.end());
    };

    struct multi_insertions {
        static constexpr auto theVictim = ctrie::build_map<int>
            ( "A"_key = 0xA
            , "ABC"_key = 0xABC
            , "FFF"_key = 0xFFF
            , "AC"_key = 0xAC
            );
        static_assert(theVictim.size() == 4);
        static_assert(theVictim.capacity() == 4);
        static_assert(theVictim["AC"] == 0xAC);
        static_assert(theVictim.find("FF") == theVictim.end(), "Found partially matching key");
        static_assert(theVictim["FFF"] == 0xFFF);
    };

    struct with_default_key {
        static constexpr auto theVictim = ctrie::build_map<int>
            ( "A"_key = 0xA
            , "B"_key = 0xB
            , ctrie::default_key = 0xDEF
            );

        static_assert(theVictim.size() == 2);
        static_assert(theVictim.capacity() == 3);
        static_assert(theVictim.has_default());
        static_assert(theVictim.get_default() == 0xDEF);

        static_assert(theVictim["A"] == 0xA, "Incorrect value for present key");
        static_assert(theVictim["C"] == 0xDEF, "Incorrect value for default key");
        static_assert(theVictim.find("C") == theVictim.end(), "Should not find unknown key");
    };
};

} // namespace ctrie_test