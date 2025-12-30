#ifndef PANCDEF_HPP
#define PANCDEF_HPP

#include <cstddef>

namespace panc
{
    constexpr std::size_t MAX_INPUT_SIZE{ 256 * 1024 };
    constexpr std::size_t MAX_TOKENS{ 4096 };
    constexpr std::size_t MAX_SYNTAX_ERRORS{ 256 };
    constexpr std::size_t MAX_STACK_DEPTH{ 256 };
}

#endif
