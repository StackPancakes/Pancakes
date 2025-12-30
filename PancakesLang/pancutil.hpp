#ifndef PANCUTIL_HPP
#define PANCUTIL_HPP

#include "panctoken.hpp"

namespace panc
{
    struct SourceLocation
    {
        std::size_t line{ 0 };
        std::size_t column{ 0 };
    };

    struct Token
    {
        TokenType type{ TokenType::UNKNOWN };
        std::string_view value{};
        SourceLocation position{};
    };

    struct BlockInfo
    {
        TokenType openKind{ TokenType::UNKNOWN };
        std::string_view name{};
        SourceLocation position{};
    };

    struct SyntaxError
    {
        char message[256]{};
        SourceLocation errorLocation{};
    };
}

#endif
