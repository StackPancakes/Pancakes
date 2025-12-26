#include <cctype>
#include <cstring>
#include <iostream> // printing only
#include <fstream> // reading the file

static bool isVerbose{};

enum class TokenType
{
    IDENTIFIER, STRING, NUMBER,
    K_SECTION, K_END, K_FUNCTION, K_CLASS, K_ONLY, K_AS, K_RETURN, K_MAIN, K_DO, K_IS, K_PROCEDURE,
    COMMA, COLON, SEMICOLON, LPAREN, RPAREN, DOT, EQUAL, PLUS, MINUS,
    UNTERMINATED_STRING, END_OF_FILE, UNKNOWN
};

struct SourceLocation
{
    std::size_t line, column;
};

struct Token
{
    TokenType type;
    char value[128];
    SourceLocation pos;
};

constexpr std::size_t MAX_TOKENS{ 4096 };
constexpr std::size_t MAX_INPUT_SIZE{ 512 * 1024 };
constexpr std::size_t MAX_BLOCK_TOKENS{ 4096 };

struct KeywordEntry
{
    char const* word;
    TokenType type;
};

static constexpr KeywordEntry keywords[]
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

static char g_inputBuffer[MAX_INPUT_SIZE + 1];
static Token g_tokens[MAX_TOKENS];
static Token g_blockTokens[MAX_BLOCK_TOKENS];

static bool ci_equal(char const* a, char const* b)
{
    while (*a && *b)
    {
        char const ca{ *a }, cb{ *b };
        if ((ca & ~0x20) != (cb & ~0x20)) return false;
        ++a; ++b;
    }
    return *a == *b;
}

class Lexer
{
    char const* input;
    std::size_t length;
    std::size_t position;
    std::size_t line;
    std::size_t column;
    char current_char;

public:
    Lexer(char const* source, std::size_t const len)
        : input{ source }, length{ len }, position{ 0 }, line{ 1 }, column{ 1 }
    {
        current_char = (length > 0) ? input[0] : '\0';
    }

    bool tokenize(Token* outTokens, std::size_t& outCount)
    {
        std::size_t idx{ 0 };
        while (true)
        {
            if (idx >= MAX_TOKENS) return false;
            Token const tok{ getNextToken() };
            outTokens[idx++] = tok;
            if (tok.type == TokenType::END_OF_FILE) break;
        }
        outCount = idx;

        if (isVerbose)
        {
            std::cout << "Tokens:\n";
            for (std::size_t i{ 0 }; i < outCount; ++i)
                std::cout << "[" << outTokens[i].pos.line << ":" << outTokens[i].pos.column << "] "
                << outTokens[i].value << "\n";
        }

        return true;
    }

private:
    Token getNextToken()
    {
        skipWhitespace();

        if (eof()) return makeToken(TokenType::END_OF_FILE, "", line, column);
        if (std::isalpha(static_cast<unsigned char>(current_char)) || current_char == '_') return identifier();
        if (std::isdigit(static_cast<unsigned char>(current_char))) return number();
        if (current_char == '"' || current_char == '\'') return stringLiteral();

        std::size_t const start_line{ line }, start_col{ column };
        char const ch{ current_char };
        advance();

        switch (ch)
        {
        case '(': return makeToken(TokenType::LPAREN, "(", start_line, start_col);
        case ')': return makeToken(TokenType::RPAREN, ")", start_line, start_col);
        case ':': return makeToken(TokenType::COLON, ":", start_line, start_col);
        case ',': return makeToken(TokenType::COMMA, ",", start_line, start_col);
        case ';': return makeToken(TokenType::SEMICOLON, ";", start_line, start_col);
        case '.': return makeToken(TokenType::DOT, ".", start_line, start_col);
        case '=': return makeToken(TokenType::EQUAL, "=", start_line, start_col);
        case '+': return makeToken(TokenType::PLUS, "+", start_line, start_col);
        case '-': return makeToken(TokenType::MINUS, "-", start_line, start_col);
        default:  return makeToken(TokenType::UNKNOWN, &ch, start_line, start_col);
        }
    }

    static Token makeToken(TokenType const type, char const* val, std::size_t const ln, std::size_t const col)
    {
        Token t{ type, {}, {ln, col} };
        std::strncpy(t.value, val, std::numeric_limits<char>::max());
        t.value[127] = '\0';
        return t;
    }

    void skipWhitespace()
    {
        while (!eof() && (current_char == ' ' || current_char == '\t' || current_char == '\n' || current_char == '\r'))
            advance();
    }

    Token identifier()
    {
        char value[128]{};
        std::size_t vi{ 0 };
        std::size_t const start_line{ line }, start_col{ column };

        while (!eof() && (std::isalnum(static_cast<unsigned char>(current_char)) || current_char == '_'))
        {
            if (vi + 1 < 128) value[vi++] = current_char;
            advance();
        }
        value[vi] = '\0';

        for (std::size_t k{ 0 }; k < std::size(keywords); ++k)
            if (ci_equal(value, keywords[k].word))
                return makeToken(keywords[k].type, value, start_line, start_col);

        return makeToken(TokenType::IDENTIFIER, value, start_line, start_col);
    }

    Token number()
    {
        char value[128]{};
        std::size_t vi{ 0 };
        std::size_t start_line{ line }, start_col{ column };

        while (!eof() && std::isdigit(static_cast<unsigned char>(current_char)))
        {
            if (vi + 1 < 128) value[vi++] = current_char;
            advance();
        }

        value[vi] = '\0';
        return makeToken(TokenType::NUMBER, value, start_line, start_col);
    }

    Token stringLiteral()
    {
        char value[128]{};
        std::size_t vi{ 0 };
        std::size_t const start_line{ line }, start_col{ column };
        char const quote{ current_char };
        advance();

        while (!eof() && current_char != quote)
        {
            if (vi + 1 < 128) value[vi++] = current_char;
            advance();
        }

        value[vi] = '\0';

        if (!eof()) advance();
        else return makeToken(TokenType::UNTERMINATED_STRING, value, start_line, start_col);

        return makeToken(TokenType::STRING, value, start_line, start_col);
    }

    void advance()
    {
        if (current_char == '\n') { ++line; column = 1; }
        else ++column;
        ++position;
        current_char = (position >= length) ? '\0' : input[position];
    }

    [[nodiscard]] bool eof() const { return position >= length || current_char == '\0'; }
};

class Parser
{
    Token* tokens;
    std::size_t tokenCount;

public:
    Parser(Token* tokenArray, std::size_t count) : tokens{ tokenArray }, tokenCount{ count }
    {
        visitMain();
    }

private:
    static void syntaxError(char const* message, Token const& tok)
    {
        std::cerr << "Syntax error at line " << tok.pos.line << ", col " << tok.pos.column << ": " << message << "\n";
        if (isVerbose) std::cerr << "Token: '" << tok.value << "'\n";
        std::exit(1);
    }

    void visitMain() const
    {
        std::size_t doIndex{ 0 };
        bool foundMain{ false };

        for (std::size_t i{ 0 }; i < tokenCount; ++i)
        {
            if (tokens[i].type == TokenType::K_PROCEDURE && i + 3 < tokenCount &&
                tokens[i + 1].type == TokenType::IDENTIFIER &&
                tokens[i + 2].type == TokenType::K_AS &&
                tokens[i + 3].type == TokenType::K_MAIN)
            {
                foundMain = true;
                doIndex = i + 4;
                break;
            }
        }

        if (!foundMain) syntaxError("Main procedure not found", tokens[0]);

        while (doIndex < tokenCount && tokens[doIndex].type != TokenType::K_DO) ++doIndex;
        if (doIndex >= tokenCount) syntaxError("'do' keyword missing", tokens[doIndex - 1]);

        std::size_t blockCount{ 0 };
        int depth{ 1 };
        std::size_t cur{ doIndex + 1 };

        while (cur < tokenCount && depth > 0 && blockCount < MAX_BLOCK_TOKENS)
        {
            if (tokens[cur].type == TokenType::K_DO) ++depth;
            else if (tokens[cur].type == TokenType::K_END) --depth;

            if (depth > 0) g_blockTokens[blockCount++] = tokens[cur];
            ++cur;
        }

        std::streambuf* outBuf{ std::cout.rdbuf() };

        for (std::size_t k{ 0 }; k < blockCount; ++k)
        {
            if (std::strcmp(g_blockTokens[k].value, "print_line") == 0 && k + 2 < blockCount)
            {
                outBuf->sputn(g_blockTokens[k + 2].value, static_cast<std::streamsize>(std::strlen(g_blockTokens[k + 2].value)));
                k += 3;
            }
        }
    }
};

int main(int argc, char* argv[])
{
    if (argc < 2) return 64;

    char filePath[512]{};

    for (int i{ 1 }; i < argc; ++i)
    {
        char const* arg{ argv[i] };
        if (std::strcmp(arg, "--verbose") == 0) isVerbose = true;
        else std::strncpy(filePath, arg, sizeof(filePath) - 1);
    }

    if (filePath[0] == '\0') return 1;

    std::ifstream reader{ filePath, std::ios::binary | std::ios::ate };
    if (!reader) return 1;

    std::streamsize const fileSize{ reader.tellg() };
    if (fileSize > static_cast<std::streamsize>(MAX_INPUT_SIZE)) return 1;

    reader.seekg(0, std::ios::beg);
    reader.read(g_inputBuffer, fileSize);
    g_inputBuffer[fileSize] = '\0';

    Lexer lexer{ g_inputBuffer, static_cast<std::size_t>(fileSize) };

    std::size_t tokenCount{ 0 };
    if (!lexer.tokenize(g_tokens, tokenCount)) return 1;

    Parser{ g_tokens, tokenCount };

    return 0;
}
