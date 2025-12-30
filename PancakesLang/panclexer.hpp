#ifndef PANCLEXER_HPP
#define PANCLEXER_HPP

#include <cstddef>
#include <ostream>
#include "panctoken.hpp"
#include "pancvar.hpp"

class Lexer
{
    char const* input;
    std::size_t length;
    std::size_t position{ 0 };
    std::size_t line{ 1 };
    std::size_t column{ 1 };

public:
    Lexer(char const* src, std::size_t len);
    std::size_t tokenizeInto(panc::Token* target, std::size_t max);
    std::size_t tokenizeToStream(std::ostream& out) const;

    friend std::ostream& operator<<(std::ostream& out, Lexer const& lexer);
    friend void operator>>(Lexer const& lexer, char const* fileName);

private:
    bool eof() const;
    char peek() const;
    char advance();
    void skip();
    panc::Token next();
};

#endif
