#include "ctrie_map.h"
#include "ctrie_map_tests.h"

#include <memory>
#include <cstdio>

using namespace ctrie::literals;

constexpr auto m = ctrie::build_map<int>
    ( "AA"_key = 0xAA
    , "BA"_key = 0xBA
    , "A"_key = 0xA
    , "AB"_key = 0xAB
    , ctrie::default_key = 0xBEEF
    );

// Correctly forwards move-only types
auto mu = ctrie::build_map<std::unique_ptr<int>>(
    "*1"_key = std::make_unique<int>(1),
    "*2"_key = std::make_unique<int>(2)
);

int main(int argc, char** argv)
{
    if (auto it = m.begin(); it != m.end()) {
        std::printf("m = [%i", *it++);
        while (it != m.end()) {
            std::printf(", %i", *it++);
        }
        std::printf("]\n");
    } else {
        std::printf("m = []\n");
    }

    for (int i = 1; i != argc; ++i) {
        const char* txt = argv[i];
        if (auto it = m.find(txt); it != m.end()) {
            std::printf("m.find('%s') -> 0x%0X\n", txt, *it);
        } else {
            std::printf("m.find('%s') == m.end()\n", txt);
            std::printf("m[%s] -> 0x%0X\n", txt, m[txt]);
        }
    }

    return 0;
}