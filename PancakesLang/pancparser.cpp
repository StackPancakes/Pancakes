#include "pancparser.hpp"
#include "pancstring.hpp"
#include <iostream>

Parser::Parser(panc::Token* t, std::size_t c) : tokens(t), count(c) {}

bool Parser::run()
{
    if (!validateStructure()) return false;
    cursor = 0;
    while (!atEnd())
    {
        if (match(panc::TokenType::K_PROCEDURE))
        {
            if (peek().type == panc::TokenType::IDENTIFIER) consume();
            if (match(panc::TokenType::K_AS) && match(panc::TokenType::K_MAIN))
            {
                executeMain();
                return true;
            }
        }
        consume();
    }
    std::cerr << "Runtime Error: main procedure not found\n";
    return false;
}

panc::Token const& Parser::peek() const { return tokens[cursor]; }
bool Parser::atEnd() const { return cursor >= count || peek().type == panc::TokenType::END_OF_FILE; }
panc::Token Parser::consume() { return atEnd() ? tokens[count - 1] : tokens[cursor++]; }
bool Parser::match(panc::TokenType t)
{
    if (!atEnd() && peek().type == t) { consume(); return true; }
    return false;
}

void Parser::executeMain()
{
    while (!atEnd() && !match(panc::TokenType::K_DO)) consume();
    int depth{ 1 };
    while (!atEnd() && depth > 0)
    {
        if (peek().type == panc::TokenType::K_DO) depth++;
        else if (peek().type == panc::TokenType::K_END)
        {
            panc::Token const& next{ (cursor + 1 < count) ? tokens[cursor + 1] : tokens[cursor] };
            if (next.type == panc::TokenType::K_PROCEDURE || next.type == panc::TokenType::K_FUNCTION)
            {
                depth--;
                consume();
                consume();
                continue;
            }
        }
        if (depth > 0)
            if (panc::Token const t{ consume() }; t.type == panc::TokenType::IDENTIFIER && t.value == "print_line")
                if (match(panc::TokenType::LPAREN) && peek().type == panc::TokenType::STRING)
                {
                    std::cout << consume().value << '\n';
                    match(panc::TokenType::RPAREN);
                }
    }
}

bool Parser::validateStructure() const
{
    parser::parseStack.clear();
    parser::syntaxErrors.clear();
    std::size_t i{ 0 };
    while (i < count)
    {
        panc::Token const& tok{ tokens[i] };
        if (tok.type == panc::TokenType::K_CLASS || tok.type == panc::TokenType::K_FUNCTION || tok.type == panc::TokenType::K_PROCEDURE)
        {
            std::string_view name{ (i + 1 < count) ? tokens[i + 1].value : "unknown" };
            parser::parseStack.push_back({ tok.type, name, tok.position });
            i++;
        }
        else if (tok.type == panc::TokenType::K_END)
        {
            if (parser::parseStack.empty()) { addError("Unexpected 'end'", tok.position); i++; continue; }
            if ((i + 1 < count ? tokens[i + 1].type : panc::TokenType::UNKNOWN) == parser::parseStack.back().openKind) i += 2;
            else { addError("Mismatched block closure", tok.position); i++; }
            parser::parseStack.pop_back();
        }
        else i++;
    }
    while (!parser::parseStack.empty()) { addError("Missing 'end' for block", parser::parseStack.back().position); parser::parseStack.pop_back(); }
    if (!parser::syntaxErrors.empty())
    {
        for (auto const& e : parser::syntaxErrors)
            std::cerr << "Syntax Error: " << e.message << " at Line " << e.errorLocation.line << '\n';
        return false;
    }
    return true;
}

void Parser::addError(char const* msg, panc::SourceLocation loc)
{
    panc::SyntaxError e{};
    panc::strcpy(e.message, sizeof(e.message), msg);
    e.errorLocation = loc;
    parser::syntaxErrors.push_back(e);
}
