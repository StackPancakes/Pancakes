#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

enum class TokenType
{
    IDENTIFIER,
    STRING,
    NUMBER,

    K_SECTION,
    K_END,
    K_FUNCTION,
    K_CLASS,
    K_ONLY,
    K_AS,
    K_RETURN,
    K_MAIN,
    K_DO,
    K_IS,

    COMMA,
    COLON,
    SEMICOLON,
    LPAREN,
    RPAREN,
    DOT,
    EQUAL,
    PLUS,
    MINUS,

    END_OF_FILE,
    UNKNOWN
};

struct SourceLocation
{
    size_t line;
    size_t column;
};


struct Token
{
    TokenType type;
    std::string value;
    SourceLocation error;
};

struct CaseInsensitiveHash
{
    size_t operator()(std::string_view s) const
    {
        size_t h{};
        for (char c : s)
        {
            c &= ~0x20;
            h = h * 131 + static_cast<unsigned char>(c);
        }
        return h;
    }
};

struct CaseInsensitiveEqual
{
    bool operator()(std::string_view a, std::string_view b) const
    {
        if (a.length() != b.length())
            return false;

        for (size_t i{}; i < a.length(); ++i)
            if ((a[i] & ~0x20) != (b[i] & ~0x20))
                return false;

        return true;
    }
};

static std::unordered_map<std::string_view, TokenType, CaseInsensitiveHash, CaseInsensitiveEqual> keywords
{
    { "class", TokenType::K_CLASS },
    { "section", TokenType::K_SECTION },
    { "function", TokenType::K_FUNCTION },
    { "as", TokenType::K_AS },
    { "return", TokenType::K_RETURN },
    { "is", TokenType::K_IS },
    { "do", TokenType::K_DO },
    { "end", TokenType::K_END }
};

class Lexer
{
public:
    Lexer(std::string const& source) : input{ source }, position{ 0 }, line{ 1 }, column{ 1 } 
    {
        if (!input.empty())
            current_char = input[0];
        else
            current_char = '\0';
    }

    Token getNextToken()
    {
        skipWhitespace();

        if (eof())
            return Token{ TokenType::END_OF_FILE, "", { line, column } };

        if (std::isalpha(current_char) || current_char == '_')
            return identifier();

        if (std::isdigit(current_char))
            return number();

        if (current_char == '"' || current_char == '\'')
            return stringLiteral();

        switch (current_char)
        {
        case '(': advance(); return Token{ TokenType::LPAREN, "(", {line, column} };
        case ')': advance(); return Token{ TokenType::RPAREN, ")", {line, column} };
        case ':': advance(); return Token{ TokenType::COLON, ":", {line, column} };
        default:
            advance();
            return Token{ TokenType::UNKNOWN, std::string(1, current_char), {line, column} };
        }
    }

    std::vector<Token> tokenize()
    {
        tokenization.clear();
        while (true)
        {
            Token tok{ getNextToken() };
            tokenization.push_back(tok);
            if (tok.type == TokenType::END_OF_FILE)
                break;
        }
        return tokenization;
    }

private:
    void skipWhitespace()
    {
        while (!eof() && (current_char == ' ' || current_char == '\t' || current_char == '\n'))
            advance();
    }

    Token identifier()
    {
        std::string value;
        size_t start_line{ line };
        size_t start_col{ column };

        while (!eof() && (std::isalnum(current_char) || current_char == '_'))
        {
            value += current_char;
            advance();
        }

        if (auto it{ keywords.find(value) }; it != keywords.end())
            return Token{ it->second, value, { line, column } };

        return Token{ TokenType::IDENTIFIER, value, { line, column } };
    }

    // no comment yet
    /*
    void skipComment();
    */

    Token stringLiteral()
    {
        std::string value;
        size_t start_line{ line };
        size_t start_col{ column };

        char quote_char{ current_char };
        advance();

        while (!eof() && current_char != quote_char)
        {
            value += current_char;
            advance();
        }

        if (current_char == quote_char)
            advance();

        return Token{ TokenType::STRING, value, { start_line, start_col } };
    }

    Token number()
    {
        std::string value;
        size_t start_line{ line };
        size_t start_col{ column };

        while (!eof() && std::isdigit(current_char))
        {
            value += current_char;
            advance();
        }

        return { TokenType::NUMBER, value, { start_line, start_col } };
    }

private:
    void advance() 
    { 
        if (current_char == '\n')
        {
            ++line;
            column = 1;
        }
        else
            ++column;

        ++position;

        current_char = eof() ? '\0' : input[position];
    }
    bool eof() const { return position >= input.length(); }

    std::vector<Token> tokenization;
    std::string input;
    size_t position;
    size_t line;
    size_t column;
    char current_char;
};