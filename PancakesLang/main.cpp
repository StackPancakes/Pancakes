#include "panclexer.hpp"
#include "pancparser.hpp"
#include "pancvar.hpp"
#include <fstream>
#include <cstring>

static bool isVerbose{ false };

int main(int argc, char* argv[])
{
    if (argc < 2) return 1;
    char const* filePath{ nullptr };
    for (int i{ 1 }; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--verbose") == 0) isVerbose = true;
        else filePath = argv[i];
    }
    if (!filePath) return 1;

    std::ifstream in{ filePath, std::ios::binary | std::ios::ate };
    if (!in) return 1;
    std::streampos const sz{ in.tellg() };
    if (sz <= 0 || sz > static_cast<std::streamsize>(panc::MAX_INPUT_SIZE)) return 1;
    in.seekg(0);
    in.read(parser::inputBuffer, sz);
    parser::inputBuffer[sz] = '\0';

    Lexer lexer{ parser::inputBuffer, static_cast<std::size_t>(sz) };
    lexer >> filePath;
    std::size_t tokenCount{ lexer.tokenizeInto(parser::tokens, panc::MAX_TOKENS) };

    Parser parser{ parser::tokens, tokenCount };
    return parser.run() ? 0 : 1;
}
