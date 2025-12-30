#ifndef PANCTOKEN_HPP
#define PANCTOKEN_HPP

#include <string_view>

namespace panc
{
    enum class TokenType
    {
        IDENTIFIER, STRING, NUMBER,
        K_SECTION, K_END, K_FUNCTION, K_CLASS, K_ONLY, K_AS, K_RETURN, K_MAIN, K_DO, K_IS, K_PROCEDURE,
        COMMA, COLON, SEMICOLON, LPAREN, RPAREN, DOT, EQUAL, PLUS, MINUS,
        UNTERMINATED_STRING, END_OF_FILE, UNKNOWN
    };

    inline constexpr std::string_view TokenTypeToString(TokenType t)
    {
        switch (t)
        {
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::STRING: return "STRING";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::K_SECTION: return "K_SECTION";
        case TokenType::K_END: return "K_END";
        case TokenType::K_FUNCTION: return "K_FUNCTION";
        case TokenType::K_CLASS: return "K_CLASS";
        case TokenType::K_ONLY: return "K_ONLY";
        case TokenType::K_AS: return "K_AS";
        case TokenType::K_RETURN: return "K_RETURN";
        case TokenType::K_MAIN: return "K_MAIN";
        case TokenType::K_DO: return "K_DO";
        case TokenType::K_IS: return "K_IS";
        case TokenType::K_PROCEDURE: return "K_PROCEDURE";
        case TokenType::COMMA: return "COMMA";
        case TokenType::COLON: return "COLON";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::DOT: return "DOT";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::UNTERMINATED_STRING: return "UNTERMINATED_STRING";
        case TokenType::END_OF_FILE: return "END_OF_FILE";
        case TokenType::UNKNOWN: return "UNKNOWN";
        }

        return "INVALID";
    }
}

#endif