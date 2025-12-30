#ifndef PANCVAR_HPP
#define PANCVAR_HPP

#include "pancdef.hpp"
#include "pancutil.hpp"
#include "pancarray.hpp"

namespace parser
{
    inline char inputBuffer[panc::MAX_INPUT_SIZE + 1];
    inline panc::Token tokens[panc::MAX_TOKENS];
    inline panc::array<panc::SyntaxError, panc::MAX_SYNTAX_ERRORS> syntaxErrors;
    inline panc::array<panc::BlockInfo, panc::MAX_STACK_DEPTH> parseStack;
}

#endif