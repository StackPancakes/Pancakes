#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int isVerbose = 0;

typedef enum TokenType
{
    IDENTIFIER, STRING, NUMBER,
    K_SECTION, K_END, K_FUNCTION, K_CLASS, K_ONLY, K_AS, K_RETURN, K_MAIN, K_DO, K_IS, K_PROCEDURE,
    COMMA, COLON, SEMICOLON, LPAREN, RPAREN, DOT, EQUAL, PLUS, MINUS,
    UNTERMINATED_STRING, END_OF_FILE, UNKNOWN
} TokenType;

typedef struct SourceLocation
{
    size_t line;
    size_t column;
} SourceLocation;

typedef struct Token
{
    TokenType type;
    char value[128];
    SourceLocation pos;
} Token;

enum
{
    MAX_TOKENS = 4096,
    MAX_INPUT_SIZE = 512 * 1024,
    MAX_BLOCK_TOKENS = 4096
};

typedef struct KeywordEntry
{
    char const* word;
    TokenType type;
} KeywordEntry;

static const KeywordEntry keywords[] =
{
    { "class", K_CLASS },
    { "section", K_SECTION },
    { "function", K_FUNCTION },
    { "procedure", K_PROCEDURE },
    { "as", K_AS },
    { "main", K_MAIN },
    { "return", K_RETURN },
    { "is", K_IS },
    { "do", K_DO },
    { "end", K_END }
};

static char g_inputBuffer[MAX_INPUT_SIZE + 1];
static Token g_tokens[MAX_TOKENS];
static Token g_blockTokens[MAX_BLOCK_TOKENS];


static int ci_equal(char const* a, char const* b)
{
    while (*a && *b)
    {
        if ((*a & ~0x20) != (*b & ~0x20)) return 0;
        a++, b++;
    }
    return *a == *b;
}

static Token makeToken(TokenType type, char const* val, size_t ln, size_t col)
{
    Token t;
    t.type = type;
    t.pos.line = ln;
    t.pos.column = col;
    if (val)
    {
        strncpy(t.value, val, sizeof(t.value) - 1);
        t.value[sizeof(t.value) - 1] = '\0';
    }
    else
        t.value[0] = '\0';
    return t;
}


typedef struct Lexer
{
    char const* input;
    size_t length;
    size_t position;
    size_t line;
    size_t column;
    char current_char;
} Lexer;


static inline void lexer_init(Lexer* lex, const char* src, size_t len)
{
    lex->input = src;
    lex->length = len;
    lex->position = 0;
    lex->line = 1;
    lex->column = 1;
    lex->current_char = (len > 0) ? src[0] : '\0';
}

static inline int lexer_eof(Lexer* lex)
{
    return lex->position >= lex->length || lex->current_char == '\0';
}

static inline void lexer_advance(Lexer* lex)
{
    if (lex->current_char == '\n') lex->line++, lex->column = 1;
    else lex->column++;
    lex->position++;
    lex->current_char = (lex->position >= lex->length) ? '\0' : lex->input[lex->position];
}

static void lexer_skipWhitespace(Lexer* lex)
{
    while (!lexer_eof(lex) && (lex->current_char == ' ' || lex->current_char == '\t' || lex->current_char == '\n' || lex->current_char == '\r'))
        lexer_advance(lex);
}

static Token lexer_stringLiteral(Lexer* lex)
{
    char value[128] = { 0 };
    size_t vi = 0, start_line = lex->line, start_col = lex->column;
    char quote = lex->current_char;
    lexer_advance(lex);
    while (!lexer_eof(lex) && lex->current_char != quote)
    {
        if (vi + 1 < sizeof(value)) value[vi++] = lex->current_char;
        lexer_advance(lex);
    }
    value[vi] = '\0';
    if (!lexer_eof(lex)) lexer_advance(lex);
    else return makeToken(UNTERMINATED_STRING, value, start_line, start_col);
    return makeToken(STRING, value, start_line, start_col);
}

static Token lexer_number(Lexer* lex)
{
    char value[128] = { 0 };
    size_t vi = 0, start_line = lex->line, start_col = lex->column;
    while (!lexer_eof(lex) && isdigit((unsigned char)lex->current_char))
    {
        if (vi + 1 < sizeof(value)) value[vi++] = lex->current_char;
        lexer_advance(lex);
    }
    value[vi] = '\0';
    return makeToken(NUMBER, value, start_line, start_col);
}

static Token lexer_identifier(Lexer* lex)
{
    char value[128] = { 0 };
    size_t vi = 0, start_line = lex->line, start_col = lex->column;
    while (!lexer_eof(lex) && (isalnum((unsigned char)lex->current_char) || lex->current_char == '_'))
    {
        if (vi + 1 < sizeof(value)) value[vi++] = lex->current_char;
        lexer_advance(lex);
    }
    value[vi] = '\0';
    for (size_t k = 0; k < sizeof(keywords) / sizeof(keywords[0]); ++k)
        if (ci_equal(value, keywords[k].word))
            return makeToken(keywords[k].type, value, start_line, start_col);
    return makeToken(IDENTIFIER, value, start_line, start_col);
}

static Token lexer_getNextToken(Lexer* lex)
{
    lexer_skipWhitespace(lex);
    if (lexer_eof(lex)) return makeToken(END_OF_FILE, "", lex->line, lex->column);

    if (isalpha((unsigned char)lex->current_char) || lex->current_char == '_')
        return lexer_identifier(lex);
    if (isdigit((unsigned char)lex->current_char))
        return lexer_number(lex);
    if (lex->current_char == '"' || lex->current_char == '\'')
        return lexer_stringLiteral(lex);

    size_t ln = lex->line, col = lex->column;
    char ch = lex->current_char;
    lexer_advance(lex);
    switch (ch)
    {
    case '(': return makeToken(LPAREN, "(", ln, col);
    case ')': return makeToken(RPAREN, ")", ln, col);
    case ':': return makeToken(COLON, ":", ln, col);
    case ',': return makeToken(COMMA, ",", ln, col);
    case ';': return makeToken(SEMICOLON, ";", ln, col);
    case '.': return makeToken(DOT, ".", ln, col);
    case '=': return makeToken(EQUAL, "=", ln, col);
    case '+': return makeToken(PLUS, "+", ln, col);
    case '-': return makeToken(MINUS, "-", ln, col);
    default: { char tmp[2] = { ch,'\0' }; return makeToken(UNKNOWN, tmp, ln, col); }
    }
}

static int lexer_tokenize(Lexer* lex, Token* outTokens, size_t* outCount)
{
    size_t idx = 0;
    while (1)
    {
        if (idx >= MAX_TOKENS) return 0;
        Token t = lexer_getNextToken(lex);
        outTokens[idx++] = t;
        if (t.type == END_OF_FILE) break;
    }
    *outCount = idx;
    if (isVerbose)
    {
        printf("Tokens:\n");
        for (size_t i = 0; i < *outCount; i++)
            printf("[%zu:%zu] %s\n", outTokens[i].pos.line, outTokens[i].pos.column, outTokens[i].value);
    }
    return 1;
}


typedef struct Parser
{
    Token* tokens;
    size_t tokenCount;
} Parser;

static void syntaxError(char const* msg, Token* tok)
{
    fprintf(stderr, "Syntax error at line %zu col %zu: %s\n", tok->pos.line, tok->pos.column, msg);
    if (isVerbose) fprintf(stderr, "Token: '%s'\n", tok->value);
    exit(1);
}

static void parser_visitMain(Parser* self)
{
    size_t doIndex = 0;
    int foundMain = 0;
    for (size_t i = 0; i < self->tokenCount; i++)
    {
        if (self->tokens[i].type == K_PROCEDURE && i + 3 < self->tokenCount &&
            self->tokens[i + 1].type == IDENTIFIER &&
            self->tokens[i + 2].type == K_AS &&
            self->tokens[i + 3].type == K_MAIN)
        {
            foundMain = 1;
            doIndex = i + 4;
            break;
        }
    }
    if (!foundMain) syntaxError("Main procedure not found", &self->tokens[0]);
    while (doIndex < self->tokenCount && self->tokens[doIndex].type != K_DO) doIndex++;
    if (doIndex >= self->tokenCount) syntaxError("'do' keyword missing", &self->tokens[(doIndex == 0) ? 0 : doIndex - 1]);

    size_t blockCount = 0;
    int depth = 1;
    size_t cur = doIndex + 1;
    while (cur < self->tokenCount && depth > 0 && blockCount < MAX_BLOCK_TOKENS)
    {
        if (self->tokens[cur].type == K_DO) depth++;
        else if (self->tokens[cur].type == K_END) depth--;
        if (depth > 0) g_blockTokens[blockCount++] = self->tokens[cur];
        cur++;
    }

    for (size_t k = 0; k < blockCount; k++)
    {
        if (strcmp(g_blockTokens[k].value, "print_line") == 0 && k + 2 < blockCount)
        {
            fwrite(g_blockTokens[k + 2].value, 1, strlen(g_blockTokens[k + 2].value), stdout);
            fputc('\n', stdout);
            k += 3;
        }
    }
}


int main(int const argc, char* argv[])
{
    if (argc < 2) return 64;
    char filePath[512] = { 0 };
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--verbose") == 0) isVerbose = 1;
        else strncpy(filePath, argv[i], sizeof(filePath) - 1);
    }
    if (filePath[0] == '\0') return 1;

    FILE* f = fopen(filePath, "rb");
    if (!f) return 1;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 1; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return 1; }
    if (sz > MAX_INPUT_SIZE) { fclose(f); return 1; }
    rewind(f);
    size_t n = fread(g_inputBuffer, 1, (size_t)sz, f);
    fclose(f);
    g_inputBuffer[n] = '\0';

    Lexer lexer;
    lexer_init(&lexer, g_inputBuffer, n);

    size_t tokenCount = 0;
    if (!lexer_tokenize(&lexer, g_tokens, &tokenCount)) return 1;

    Parser parser = { g_tokens,tokenCount };
    parser_visitMain(&parser);

    return 0;
}
