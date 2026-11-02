#include "Parser.h"
#include "token.h"

#include <cstdio>

// Parser 将 lexer 返回的 token 组织成 AST。
int CurTok = tok_eof;

int getNextToken() {
    return CurTok = gettok();
}

std::unique_ptr<ExprAST> LogError(const char *str) {
    std::fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *str) {
    LogError(str);
    return nullptr;
}

