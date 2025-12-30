#ifndef PANCSTRING_HPP
#define PANCSTRING_HPP

#include <cstddef>

namespace panc
{
    inline void strcpy(char* dst, std::size_t dstSize, char const* src)
    {
        if (!dst || !src || dstSize == 0) return;

        std::size_t i{ 0 };
        for (; i + 1 < dstSize && src[i] != '\0'; ++i)
            dst[i] = src[i];

        dst[i] = '\0';
    }

    inline void strcat(char* dst, std::size_t dstSize, char const* src)
    {
        if (!dst || !src || dstSize == 0) return;

        std::size_t dstLen{ 0 };
        while (dstLen < dstSize - 1 && dst[dstLen] != '\0')
            dstLen++;

        std::size_t i{ 0 };
        for (; dstLen + i + 1 < dstSize && src[i] != '\0'; ++i)
            dst[dstLen + i] = src[i];

        dst[dstLen + i] = '\0';
    }

    inline char* strrchr(char* str, char ch)
    {
        if (!str) return nullptr;

        char* last{ nullptr };
        for (char* p{ str }; ; ++p)
        {
            if (*p == ch)
                last = p;
            if (*p == '\0')
                break;
        }
        return last;
    }
}

#endif
