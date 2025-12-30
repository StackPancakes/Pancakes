#include "panclexer.hpp"
#include "pancstring.hpp"
#include <cctype>
#include <cstring>
#include <iostream>
#include <fstream>

namespace
{
    constexpr struct { char const* word; panc::TokenType type; } keywords[]
    {
        {"class", panc::TokenType::K_CLASS},
        {"section", panc::TokenType::K_SECTION},
        {"function", panc::TokenType::K_FUNCTION},
        {"procedure", panc::TokenType::K_PROCEDURE},
        {"as", panc::TokenType::K_AS},
        {"main", panc::TokenType::K_MAIN},
        {"return", panc::TokenType::K_RETURN},
        {"is", panc::TokenType::K_IS},
        {"do", panc::TokenType::K_DO},
        {"end", panc::TokenType::K_END}
    };

    bool ci_equal_n(char const* a, std::size_t len, char const* b)
    {
        std::size_t const blen{ std::strlen(b) };
        if (len != blen) return false;
        for (std::size_t i{ 0 }; i < len; ++i)
            if ((static_cast<unsigned char>(a[i]) & ~0x20) != (static_cast<unsigned char>(b[i]) & ~0x20))
                return false;
        return true;
    }
}

Lexer::Lexer(char const* src, std::size_t len) : input(src), length(len) {}

std::size_t Lexer::tokenizeInto(panc::Token* target, std::size_t max)
{
    std::size_t count{ 0 };
    while (count < max)
    {
        panc::Token const t{ next() };
        if (t.type != panc::TokenType::UNKNOWN)
        {
            target[count++] = t;
            if (t.type == panc::TokenType::END_OF_FILE)
                break;
        }
    }
    return count;
}

std::size_t Lexer::tokenizeToStream(std::ostream& out) const
{
    std::size_t count{ 0 };
    Lexer copy{ *this };
    while (true)
    {
        panc::Token const t{ copy.next() };
        if (t.type != panc::TokenType::UNKNOWN)
        {
            out << "Token Type: <" << panc::TokenTypeToString(t.type)
                << "> Word: \"" << t.value
                << "\" Position: { Line: " << t.position.line
                << ", Column: " << t.position.column << " }\n";
            count++;
        }
        if (t.type == panc::TokenType::END_OF_FILE)
            break;
    }
    return count;
}

std::ostream& operator<<(std::ostream& out, Lexer const& lexer)
{
    lexer.tokenizeToStream(out);
    return out;
}

void operator>>(Lexer const& lexer, char const* fileName)
{
    char outName[512]{};
    panc::strcpy(outName, sizeof(outName), fileName);
    if (char* dot{ panc::strrchr(outName, '.') }) *dot = '\0';
    panc::strcat(outName, sizeof(outName), "Token.txt");
    std::ofstream out{ outName, std::ios::out | std::ios::trunc };
    if (!out) return;
    lexer.tokenizeToStream(out);
}

bool Lexer::eof() const
{
    return position >= length || input[position] == '\0';
}

char Lexer::peek() const
{
    return eof() ? '\0' : input[position];
}

char Lexer::advance()
{
    char const c{ peek() };
    ++position;
    if (c == '\n') { ++line; column = 1; }
    else ++column;
    return c;
}

void Lexer::skip()
{
    while (!eof())
        if (char const c{ peek() }; std::isspace(static_cast<unsigned char>(c)) || static_cast<unsigned char>(c) == 0xA0)
            advance();
        else
            break;
}

panc::Token Lexer::next()
{
    skip();
    std::size_t const ln{ line }, col{ column };
    if (eof()) return { panc::TokenType::END_OF_FILE, "", { ln, col } };
    char const c{ advance() };

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
    {
        std::size_t const start{ position - 1 };
        while (!eof() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_'))
            advance();
        std::size_t len{ position - start };
        for (auto const& kw : keywords)
            if (ci_equal_n(input + start, len, kw.word))
                return { kw.type, {input + start, len}, { ln, col } };
        return { panc::TokenType::IDENTIFIER, {input + start, len}, { ln, col } };
    }

    if (std::isdigit(static_cast<unsigned char>(c)))
    {
        std::size_t const start{ position - 1 };
        while (!eof() && std::isdigit(static_cast<unsigned char>(peek())))
            advance();
        return { panc::TokenType::NUMBER, {input + start, position - start}, { ln, col } };
    }

    if (c == '"' || c == '\'')
    {
        char const q{ c };
        std::size_t const start{ position };
        while (!eof() && peek() != q) advance();
        if (eof()) return { panc::TokenType::UNTERMINATED_STRING, {input + start, position - start}, { ln, col } };
        advance();
        return { panc::TokenType::STRING, {input + start, position - start - 1}, { ln, col } };
    }

    panc::TokenType t{};
    switch (c)
    {
    case '(': t = panc::TokenType::LPAREN; break;
    case ')': t = panc::TokenType::RPAREN; break;
    case ',': t = panc::TokenType::COMMA; break;
    case ':': t = panc::TokenType::COLON; break;
    case ';': t = panc::TokenType::SEMICOLON; break;
    case '.': t = panc::TokenType::DOT; break;
    case '=': t = panc::TokenType::EQUAL; break;
    case '+': t = panc::TokenType::PLUS; break;
    case '-': t = panc::TokenType::MINUS; break;
    default: return { panc::TokenType::UNKNOWN, "", { ln, col } };
    }
    return { t, {input + (position - 1), 1}, { ln, col } };
}
