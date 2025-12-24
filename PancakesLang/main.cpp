#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>

static bool isVerbose{};

namespace fs = std::filesystem;

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
    K_PROCEDURE,

    COMMA,
    COLON,
    SEMICOLON,
    LPAREN,
    RPAREN,
    DOT,
    EQUAL,
    PLUS,
    MINUS,

    UNTERMINATED_STRING,
    END_OF_FILE,
    UNKNOWN
};

enum class BlockKind
{
    FUNCTION,
    PROCEDURE,
    CLASS
};

constexpr std::string_view TokenTypeToString(TokenType type)
{
    switch (type)
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
    return "UNKNOWN";
}

constexpr std::string_view BlockKindToString(BlockKind type)
{
    switch (type)
    {
    case BlockKind::CLASS: return "CLASS";
    case BlockKind::FUNCTION: return "FUNCTION";
    case BlockKind::PROCEDURE: return "PROCEDURE";
    }
    return "UNKNOWN";
}

struct SourceLocation
{
    size_t line;
    size_t column;
};

struct Token
{
    TokenType type;
    std::string value;
    SourceLocation position;
};

struct SyntaxError
{
    std::string message{};
    SourceLocation errorLocation{};
};

struct BlockInfo
{
    BlockKind kind;
    std::string name;
    SourceLocation position;
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
    { "procedure", TokenType::K_PROCEDURE },
    { "as", TokenType::K_AS },
    { "main", TokenType::K_MAIN },
    { "return", TokenType::K_RETURN },
    { "is", TokenType::K_IS },
    { "do", TokenType::K_DO },
    { "end", TokenType::K_END }
};

class Lexer
{
    std::vector<Token> tokenization;
    std::string input;
    size_t position;
    size_t line;
    size_t column;
    char current_char;

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

        if (std::isalpha(static_cast<unsigned char>(current_char)) || current_char == '_')
            return identifier();

        if (std::isdigit(static_cast<unsigned char>(current_char)))
            return number();

        if (current_char == '"' || current_char == '\'')
            return stringLiteral();

        size_t start_line{ line };
        size_t start_col{ column };
        char ch{ current_char };

        advance();

        switch (ch)
        {
        case '(': return Token{ TokenType::LPAREN, "(", { start_line, start_col } };
        case ')': return Token{ TokenType::RPAREN, ")", { start_line, start_col } };
        case ':': return Token{ TokenType::COLON, ":", { start_line, start_col } };
        case ',': return Token{ TokenType::COMMA, ",", { start_line, start_col } };
        case ';': return Token{ TokenType::SEMICOLON, ";", { start_line, start_col } };
        case '.': return Token{ TokenType::DOT, ".", { start_line, start_col } };
        case '=': return Token{ TokenType::EQUAL, "=", { start_line, start_col } };
        case '+': return Token{ TokenType::PLUS, "+", { start_line, start_col } };
        case '-': return Token{ TokenType::MINUS, "-", { start_line, start_col } };
        default:  return Token{ TokenType::UNKNOWN, std::string(1, ch), { start_line, start_col } };
        }
    }

    std::vector<Token>& tokenize()
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
        while (!eof() && (current_char == ' ' || current_char == '\t' ||
            current_char == '\n' || current_char == '\r'))
            advance();
    }

    Token identifier()
    {
        std::string value;
        size_t start_line{ line };
        size_t start_col{ column };

        while (!eof() && (std::isalnum(static_cast<unsigned char>(current_char)) || current_char == '_'))
        {
            value += current_char;
            advance();
        }

        if (auto it{ keywords.find(value) }; it != keywords.end())
            return Token{ it->second, value, { start_line, start_col } };

        return Token{ TokenType::IDENTIFIER, value, { start_line, start_col } };
    }

    // no comment yet
    /*
    void skipComment();
    */

    Token number()
    {
        std::string value;
        size_t start_line{ line };
        size_t start_col{ column };

        while (!eof() && std::isdigit(static_cast<unsigned char>(current_char)))
        {
            value += current_char;
            advance();
        }

        return { TokenType::NUMBER, value, { start_line, start_col } };
    }


    Token stringLiteral()
    {
        std::string value;
        size_t start_line{ line };
        size_t start_col{ column };
        char quote{ current_char };

        advance();

        while (!eof() && current_char != quote)
        {
            value += current_char;
            advance();
        }

        if (eof())
            return Token{ TokenType::UNTERMINATED_STRING, value, { start_line, start_col } };

        advance();
        return Token{ TokenType::STRING, value, { start_line, start_col } };
    }

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

 public:
     friend std::ostream& operator<<(std::ostream& out, Lexer const& other)
     {
         Lexer copy{ other };

         for (Token const& tok : copy.tokenize())
             out << "Token Type: <" << TokenTypeToString(tok.type)
             << "> Word: \"" << tok.value
             << "\" Position: { Line: " << tok.position.line
             << ", Column: " << tok.position.column << " }\n";

         return out;
     }
};

class Parser
{
    Lexer lexer;
    std::vector<Token> tokens;

public:
    Parser(Lexer const& lex) : lexer{ lex }, tokens{ lexer.tokenize() }
    {
        auto syntaxErrors{ checkSyntax() };
        if (!syntaxErrors.empty())
        {
            std::cerr << "Syntax Errors found:\n";
            for (auto const& err : syntaxErrors)
            {
                std::cerr << "- " << err.message
                    << " at Line: " << err.errorLocation.line
                    << ", Column: " << err.errorLocation.column << '\n';
            }
            std::exit(1);
        }

        if (isVerbose)
            std::cout << "Syntax is valid!\n";

        try
        {
            visitMain();
        }
        catch (std::runtime_error const& e)
        {
            std::cerr << "[Runtime Error] " << e.what() << '\n';
            std::exit(1);
        }
    }


    std::vector<SyntaxError> checkSyntax()
    {
        std::vector<BlockInfo> block_stack;
        std::vector<SyntaxError> errors;

        for (size_t i = 0; i < tokens.size(); ++i)
        {
            Token const& token = tokens[i];

            if (token.type == TokenType::K_END)
            {
                if (block_stack.empty())
                {
                    errors.push_back({ "Too many 'end' keywords", token.position });
                    continue;
                }

                if (i + 1 >= tokens.size())
                {
                    errors.push_back({ "'end' without following keyword", token.position });
                    continue;
                }

                Token const& nextToken{ tokens[i + 1] };
                BlockInfo const& top{ block_stack.back() };

                bool mismatch{ false };

                switch (top.kind)
                {
                case BlockKind::CLASS: mismatch = nextToken.type != TokenType::K_CLASS; break;
                case BlockKind::FUNCTION: mismatch = nextToken.type != TokenType::K_FUNCTION; break;
                case BlockKind::PROCEDURE: mismatch = nextToken.type != TokenType::K_PROCEDURE; break;
                }

                if (mismatch)
                    errors.push_back({ "Mismatched 'end' for " + std::string{ BlockKindToString(top.kind) } +
                        " '" + top.name + "'", nextToken.position });

                block_stack.pop_back();
                ++i;
                continue;
            }

            switch (token.type)
            {
            case TokenType::K_CLASS:
            case TokenType::K_FUNCTION:
            case TokenType::K_PROCEDURE:
            {
                std::string name{ "unknown" };
                if (i + 1 < tokens.size() && tokens[i + 1].type == TokenType::IDENTIFIER)
                    name = tokens[i + 1].value;

                BlockKind kind;
                switch (token.type)
                {
                case TokenType::K_CLASS: kind = BlockKind::CLASS; break;
                case TokenType::K_FUNCTION: kind = BlockKind::FUNCTION; break;
                case TokenType::K_PROCEDURE: kind = BlockKind::PROCEDURE; break;
                default: kind = BlockKind::FUNCTION; break;
                }

                block_stack.push_back({ kind, name, token.position });
                break;
            }
            default: break;
            }
        }
        
        while (!block_stack.empty())
        {
            BlockInfo const& top{ block_stack.back() };
            errors.push_back({ "Missing 'end' for " + std::string{ BlockKindToString(top.kind) } +
                " '" + top.name + "'", top.position });
            block_stack.pop_back();
        }

        return errors;
    }

    void visitMain()
    {
        size_t n{ tokens.size() };
        bool foundMain{};
        size_t isIndex{};

        for (size_t i{}; i < n; ++i)
        {
            if (tokens[i].type != TokenType::K_PROCEDURE)
                continue;

            size_t j{ i + 1 };
            if (j < n && tokens[j].type == TokenType::IDENTIFIER)
                ++j;

            if (j + 2 < n &&
                tokens[j].type == TokenType::K_AS &&
                tokens[j + 1].type == TokenType::K_MAIN &&
                tokens[j + 2].type == TokenType::K_IS)
            {
                foundMain = true;
                isIndex = j + 2;
                break;
            }
        }

        if (!foundMain)
            throw std::runtime_error("main function/procedure doesn't exist");

        size_t doIndex{ isIndex + 1 };
        while (doIndex < n && tokens[doIndex].type != TokenType::K_DO)
            ++doIndex;

        if (doIndex >= n)
            throw std::runtime_error("Expected 'do' after 'is' in main");

        std::vector<Token> blockTokens;
        int depth{ 1 };
        size_t cur{ doIndex + 1 };

        while (cur < n && depth > 0)
        {
            if (tokens[cur].type == TokenType::K_DO)
                ++depth;
            else if (tokens[cur].type == TokenType::K_END)
                if (cur + 1 < n && tokens[cur + 1].type == TokenType::K_PROCEDURE)
                    --depth;

            if (depth > 0)
                blockTokens.push_back(tokens[cur]);

            ++cur;
        }

        for (size_t k{}; k < blockTokens.size(); ++k)
        {
            if (blockTokens[k].type == TokenType::IDENTIFIER &&
                blockTokens[k].value == "print_line")
            {
                size_t argIndex{ k + 1 };

                if (argIndex < blockTokens.size() && blockTokens[argIndex].type == TokenType::LPAREN)
                    ++argIndex;

                if (argIndex < blockTokens.size() && blockTokens[argIndex].type == TokenType::STRING)
                {
                    std::cout << blockTokens[argIndex].value << '\n';
                    k = argIndex;
                    if (k + 1 < blockTokens.size() && blockTokens[k + 1].type == TokenType::RPAREN)
                        ++k;
                }
            }
        }
    }

};

int main(int const argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: pancakesC [--verbose] <file.cakes>\n";
        return 64;
    }

    std::string filePath;

    for (int i{ 1 }; i < argc; ++i)
    {
        std::string arg{ argv[i] };

        if (arg == "--verbose")
            isVerbose = true;
        else
            filePath = arg;
    }

    if (filePath.empty())
    {
        std::cerr << "Error: No .cakes file provided\n";
        std::cerr << "Usage: pancakesC [--verbose] <file.cakes>\n";
        return 1;
    }

    fs::path file{ filePath };
    if (file.is_relative())
    {
        try
        {
            file = fs::canonical(file);
        }
        catch (std::exception const&)
        {
            std::cerr << "Couldn't find " << file << '\n';
            return 1;
        }
    }

    std::ifstream reader{ file };
    if (!reader)
    {
        std::cerr << "Error: failed to open " << file << " for reading\n";
        return 1;
    }

    std::ostringstream stream;
    stream << reader.rdbuf();
    std::string buffer{ stream.str() };

    Lexer lexer{ buffer };

    if (isVerbose)
        std::cout << lexer << '\n';

    Parser parse{ lexer };
}
