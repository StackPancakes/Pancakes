#ifndef PANC_PANCAKES_HPP
#define PANC_PANCAKES_HPP

#include <string>

namespace panc
{
    class parser
    {
    public:
        parser(std::string const& buffer)
            : buffer{ buffer }, pos{}, line{ 1 }
        {

        }

        char peek() const
        {
            return pos < buffer.size() ? buffer[pos] : '\0';
        }

        char advance()
        {
            if (pos < buffer.size())
            {
                char c{ buffer[pos++] };
                if (c == '\n')
                    ++line;
                return c;
            }
            return '\0';
        }

        bool eof() const
        {
            return pos >= buffer.size();
        }

    private:
        std::string buffer;
        size_t pos;
        size_t line;
    };
}

#endif