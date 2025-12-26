#include <cctype>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string_view>
#include "pancstring.hpp"

static bool isVerbose{ false };

namespace panc
{
    template<typename T, std::size_t Capacity>
    struct array
    {
        T data[Capacity]{};
        std::size_t count{ 0 };

        void push_back(T const& v)
        {
            if (count < Capacity)
                data[count++] = v;
        }

        void pop_back()
        {
            if (count > 0)
                --count;
        }

        void clear()
        {
            count = 0;
        }

        [[nodiscard]] bool empty() const
        {
            return count == 0;
        }

        [[nodiscard]] std::size_t size() const
        {
            return count;
        }

        T& back()
        {
            return data[count - 1];
        }

        [[nodiscard]] T const& back() const
        {
            return data[count - 1];
        }

        T& operator[](std::size_t i)
        {
            return data[i];
        }

        T const& operator[](std::size_t i) const
        {
            return data[i];
        }

        T* begin()
        {
            return data;
        }

        T* end()
        {
            return data + count;
        }
    };
}

enum class TokenType
{
    IDENTIFIER, STRING, NUMBER,
    K_SECTION, K_END, K_FUNCTION, K_CLASS, K_ONLY, K_AS, K_RETURN, K_MAIN, K_DO, K_IS, K_PROCEDURE,
    COMMA, COLON, SEMICOLON, LPAREN, RPAREN, DOT, EQUAL, PLUS, MINUS,
    UNTERMINATED_STRING, END_OF_FILE, UNKNOWN
};

static constexpr std::string_view TokenTypeToString(TokenType t)
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
    default: return "INVALID";
    }
}

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

constexpr std::size_t MAX_INPUT_SIZE{ 256 * 1024 };
constexpr std::size_t MAX_TOKENS{ 4096 };
constexpr std::size_t MAX_SYNTAX_ERRORS{ 256 };
constexpr std::size_t MAX_STACK_DEPTH{ 256 };

static char g_inputBuffer[MAX_INPUT_SIZE + 1];
static Token g_tokens[MAX_TOKENS];
static panc::array<SyntaxError, MAX_SYNTAX_ERRORS> g_syntaxErrors;
static panc::array<BlockInfo, MAX_STACK_DEPTH> g_parseStack;

static constexpr struct { char const* word; TokenType type; } keywords[]
{
    {"class", TokenType::K_CLASS},
    {"section", TokenType::K_SECTION},
    {"function", TokenType::K_FUNCTION},
    {"procedure", TokenType::K_PROCEDURE},
    {"as", TokenType::K_AS},
    {"main", TokenType::K_MAIN},
    {"return", TokenType::K_RETURN},
    {"is", TokenType::K_IS},
    {"do", TokenType::K_DO},
    {"end", TokenType::K_END}
};

static bool ci_equal_n(char const* a, std::size_t len, char const* b)
{
    std::size_t blen{ std::strlen(b) };
    if (len != blen)
        return false;

    for (std::size_t i{ 0 }; i < len; ++i)
    {
        unsigned char ca{ static_cast<unsigned char>(a[i]) };
        unsigned char cb{ static_cast<unsigned char>(b[i]) };
        if ((ca & ~0x20) != (cb & ~0x20))
            return false;
    }
    return true;
}

class Lexer
{
    char const* input;
    std::size_t length;
    std::size_t position{ 0 };
    std::size_t line{ 1 };
    std::size_t column{ 1 };

public:
    Lexer(char const* src, std::size_t len) : input{ src }, length{ len } {}

    std::size_t tokenizeInto(Token* target, std::size_t max)
    {
        std::size_t count{ 0 };
        while (count < max)
            if (Token const t{ next() }; t.type != TokenType::UNKNOWN)
            {
                target[count++] = t;
                if (t.type == TokenType::END_OF_FILE)
                    break;
            }
        return count;
    }

private:
    [[nodiscard]] bool eof() const
    {
        return position >= length || input[position] == '\0';
    }

    [[nodiscard]] char peek() const
    {
        return eof() ? '\0' : input[position];
    }

    char advance()
    {
        char const c{ peek() };
        ++position;
        if (c == '\n')
        {
            ++line; column = 1;
        }
        else
            ++column;
        return c;
    }

    void skip()
    {
        while (!eof())
        {
            if (char const c{ peek() }; std::isspace(static_cast<unsigned char>(c)) || static_cast<unsigned char>(c) == 0xA0)
                advance();
            else
                break;
        }
    }

    Token next()
    {
        skip();
        std::size_t const ln{ line }, col{ column };
        if (eof())
            return { TokenType::END_OF_FILE, "", {ln, col} };

        char const c{ advance() };

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
        {
            std::size_t const start{ position - 1 };
            while (!eof() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_'))
                advance();

            std::size_t len{ position - start };

            for (auto const& [word, type] : keywords)
                if (ci_equal_n(input + start, len, word))
                    return { type, {input + start, len}, {ln, col} };

            return { TokenType::IDENTIFIER, {input + start, len}, {ln, col} };
        }

        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            std::size_t const start{ position - 1 };
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek())))
                advance();

            return { TokenType::NUMBER, {input + start, position - start}, {ln, col} };
        }

        if (c == '"' || c == '\'')
        {
            char const q{ c };
            std::size_t const start{ position };

            while (!eof() && peek() != q)
                advance();
            if (eof())
                return { TokenType::UNTERMINATED_STRING, {input + start, position - start}, {ln, col} };

            advance();
            return { TokenType::STRING, {input + start, position - start - 1}, {ln, col} };
        }

        TokenType t{};
        switch (c)
        {
        case '(': t = TokenType::LPAREN; break;
        case ')': t = TokenType::RPAREN; break;
        case ',': t = TokenType::COMMA; break;
        case ':': t = TokenType::COLON; break;
        case ';': t = TokenType::SEMICOLON; break;
        case '.': t = TokenType::DOT; break;
        case '=': t = TokenType::EQUAL; break;
        case '+': t = TokenType::PLUS; break;
        case '-': t = TokenType::MINUS; break;
        default: return { TokenType::UNKNOWN, "", {ln, col} };
        }
        return { t, {input + (position - 1), 1}, {ln, col} };
    }

public:
    std::size_t tokenizeToStream(std::ostream& out) const
    {
        std::size_t count{ 0 };
        Lexer copy{ *this };

        while (true)
        {
            auto [type, value, position] { copy.next() };
            if (type != TokenType::UNKNOWN)
            {
                out << "Token Type: <" << TokenTypeToString(type)
                    << "> Word: \"" << value
                    << "\" Position: { Line: " << position.line
                    << ", Column: " << position.column << " }\n";
                count++;
            }

            if (type == TokenType::END_OF_FILE)
                break;
        }
        return count;
    }

    friend std::ostream& operator<<(std::ostream& out, Lexer const& lexer)
    {
        lexer.tokenizeToStream(out);
        return out;
    }

    friend void operator>>(Lexer const& lexer, char const* fileName)
    {
        char outName[512]{};
        panc::strcpy(outName, sizeof(outName), fileName);

        if (char* dot{ panc::strrchr(outName, '.') })
            *dot = '\0';

        panc::strcat(outName, sizeof(outName), "Token.txt");

        std::ofstream out{ outName, std::ios::out | std::ios::trunc };
        if (!out)
        {
            std::cerr << "Failed to open token dump file: " << outName << '\n';
            return;
        }

        lexer.tokenizeToStream(out);
    }

};

class Parser
{
    Token* tokens;
    std::size_t count;
    std::size_t cursor{ 0 };

public:
    Parser(Token* t, std::size_t c) : tokens{ t }, count{ c } {}

    bool run()
    {
        if (!validateStructure())
            return false;

        cursor = 0;
        while (!atEnd())
        {
            if (match(TokenType::K_PROCEDURE))
            {
                if (peek().type == TokenType::IDENTIFIER)
                    consume();
                if (match(TokenType::K_AS) && match(TokenType::K_MAIN))
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

private:
    [[nodiscard]] Token const& peek() const
    {
        return tokens[cursor];
    }

    [[nodiscard]] bool atEnd() const
    {
        return cursor >= count || peek().type == TokenType::END_OF_FILE;
    }

    Token consume()
    {
        return atEnd() ? tokens[count - 1] : tokens[cursor++];
    }

    bool match(TokenType t)
    {
        if (!atEnd() && peek().type == t)
        {
            consume();
            return true;
        }
        return false;
    }

    void executeMain()
    {
        while (!atEnd() && !match(TokenType::K_DO))
            consume();

        int depth{ 1 };
        while (!atEnd() && depth > 0)
        {
            if (peek().type == TokenType::K_DO)
                depth++;
            else if (peek().type == TokenType::K_END)
            {
                Token const& next{ (cursor + 1 < count) ? tokens[cursor + 1] : tokens[cursor] };
                if (next.type == TokenType::K_PROCEDURE || next.type == TokenType::K_FUNCTION)
                {
                    depth--;
                    consume();
                    consume();
                    continue;
                }
            }

            if (depth > 0)
                if (Token const t{ consume() }; t.type == TokenType::IDENTIFIER && t.value == "print_line")
                    if (match(TokenType::LPAREN) && peek().type == TokenType::STRING)
                    {
                        std::cout << consume().value << '\n';
                        match(TokenType::RPAREN);
                    }
        }
    }

    [[nodiscard]] bool validateStructure() const 
    {
        g_parseStack.clear();
        g_syntaxErrors.clear();

        std::size_t i{ 0 };
        while (i < count)
        {
            Token const& tok{ tokens[i] };

            if (tok.type == TokenType::K_CLASS || tok.type == TokenType::K_FUNCTION || tok.type == TokenType::K_PROCEDURE)
            {
                std::string_view name{ (i + 1 < count) ? tokens[i + 1].value : "unknown" };
                g_parseStack.push_back({ tok.type, name, tok.position });
                i++;
            }
            else if (tok.type == TokenType::K_END)
            {
                if (g_parseStack.empty())
                {
                    addError("Unexpected 'end'", tok.position);
                    i++;
                    continue;
                }

                if ((i + 1 < count ? tokens[i + 1].type : TokenType::UNKNOWN) == g_parseStack.back().openKind)
                    i += 2;
                else
                {
                    addError("Mismatched block closure", tok.position);
                    i++;
                }
                g_parseStack.pop_back();
            }
            else
                i++;
        }

        while (!g_parseStack.empty())
        {
            addError("Missing 'end' for block", g_parseStack.back().position);
            g_parseStack.pop_back();
        }

        if (!g_syntaxErrors.empty())
        {
            for (auto const& e : g_syntaxErrors)
                std::cerr << "Syntax Error: " << e.message << " at Line " << e.errorLocation.line << '\n';
            return false;
        }
        return true;
    }

    static void addError(char const* msg, SourceLocation loc)
    {
        SyntaxError e{};
        panc::strcpy(e.message, sizeof(e.message), msg);
        e.errorLocation = loc;
        g_syntaxErrors.push_back(e);
    }
};

int main(int argc, char* argv[])
{
    if (argc < 2)
        return 1;

    char const* filePath{ nullptr };
    for (int i{ 1 }; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--verbose") == 0)
            isVerbose = true;
        else
            filePath = argv[i];
    }

    if (!filePath)
        return 1;

    std::ifstream in{ filePath, std::ios::binary | std::ios::ate };
    if (!in)
        return 1;

    std::streamsize const sz{ in.tellg() };
    if (sz <= 0 || sz > static_cast<std::streamsize>(MAX_INPUT_SIZE))
        return 1;

    in.seekg(0);
    in.read(g_inputBuffer, sz);
    g_inputBuffer[sz] = '\0';

    Lexer lexer{ g_inputBuffer, static_cast<std::size_t>(sz) };
    lexer >> filePath;
    std::size_t tokenCount{ lexer.tokenizeInto(g_tokens, MAX_TOKENS) };

    Parser parser{ g_tokens, tokenCount };
    return parser.run() ? 0 : 1;
}
