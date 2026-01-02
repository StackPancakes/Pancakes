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

        SyntaxError() = default;

        SyntaxError(char const* msg, int line, int column)
            : errorLocation{ line, column }
        {
            size_t i = 0;
            while (msg[i] != '\0' && i < sizeof(message) - 1)
            {
                message[i] = msg[i];
                ++i;
            }
            message[i] = '\0';
        }

        SyntaxError& operator=(char const* msg)
        {
            size_t i = 0;
            while (msg[i] != '\0' && i < sizeof(message) - 1)
            {
                message[i] = msg[i];
                ++i;
            }
            message[i] = '\0';
            return *this;
        }

        SyntaxError& operator= (SyntaxError temp)
        {
            *this = temp.message;
            errorLocation = temp.errorLocation;
            return *this;
        }
    };
}

#endif
