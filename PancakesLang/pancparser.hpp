#ifndef PANCPARSER_HPP
#define PANCPARSER_HPP

#include "panctoken.hpp"
#include "pancutil.hpp"
#include "pancvar.hpp"

class Parser
{
    panc::Token* tokens;
    std::size_t count;
    std::size_t cursor{ 0 };

public:
    Parser(panc::Token* t, std::size_t c);
    bool run();

private:
    panc::Token const& peek() const;
    bool atEnd() const;
    panc::Token consume();
    bool match(panc::TokenType t);
    void executeMain();
    bool validateStructure() const;
    static void addError(char const* msg, panc::SourceLocation loc);
};

#endif
