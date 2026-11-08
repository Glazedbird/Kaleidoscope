#include "Parser.h"
#include "ASTPrinter.h"
#include "token.h"

#include <cstdio>
#include <iostream>
#include <string>
#include <unistd.h>

namespace {

// 保留原有的词法分析调试入口。
int PrintTokens() {
    while (true) {
        int Tok = gettok();
        switch (Tok) {
        case tok_eof:
            std::printf("tok_eof\n");
            return 0;
        case tok_def:
            std::printf("tok_def\n");
            break;
        case tok_extern:
            std::printf("tok_extern\n");
            break;
        case tok_identifier:
            std::printf("tok_identifier: %s\n", IdentifierStr.c_str());
            break;
        case tok_number:
            std::printf("tok_number: %g\n", NumVal);
            break;
        default:
            std::printf("character: '%c' (%d)\n", Tok, Tok);
            break;
        }
    }
}

// 丢弃当前出错语句，停在分号、下一条声明或 EOF，保证解析继续推进。
void RecoverFromError() {
    while (CurTok != tok_eof && CurTok != ';' &&
           CurTok != tok_def && CurTok != tok_extern)
        getNextToken();
}

int MainLoop(bool PrintAST) {
    const bool Interactive = isatty(fileno(stdin));
    bool HadError = false;
    if (Interactive)
        std::fprintf(stderr, "ready> ");
    getNextToken();

    while (CurTok != tok_eof) {
        bool Parsed = false;
        switch (CurTok) {
        case ';': // 顶层分号用于分隔语句，本身不产生 AST。
            Parsed = true;
            break;
        case tok_def:
            // 这里的逻辑是这样的 —— 一旦确定是一个def的声明，这里就要对函数的定义进行Parse 
            if (auto Function = ParseDefinition()) {
                std::printf("Parsed a function definition: %s\n",
                            Function->getPrototype().getName().c_str());
                if (PrintAST)
                    DumpAST(*Function, std::cout);
                Parsed = true;
            }
            break;
        case tok_extern:
            if (auto Proto = ParseExtern()) {
                std::printf("Parsed an extern: %s\n", Proto->getName().c_str());
                if (PrintAST)
                    DumpAST(*Proto, std::cout);
                Parsed = true;
            }
            break;
        default:
            if (auto Function = ParseTopLevelExpr()) {
                std::printf("Parsed a top-level expression\n");
                if (PrintAST)
                    DumpAST(*Function, std::cout);
                Parsed = true;
            }
            break;
        }

        if (!Parsed) {
            HadError = true;
            RecoverFromError();
        }
        if (Interactive)
            std::fprintf(stderr, "ready> ");
        if (CurTok == ';')
            getNextToken();
    }
    return HadError ? 1 : 0;
}

} // namespace

int main(int argc, char **argv) {
    if (argc == 2 && std::string(argv[1]) == "--tokens")
        return PrintTokens();
    const bool PrintAST = argc == 2 && std::string(argv[1]) == "--dump-ast";
    if (argc != 1 && !PrintAST) {
        std::fprintf(stderr, "Usage: %s [--tokens | --dump-ast]\n", argv[0]);
        return 1;
    }
    return MainLoop(PrintAST);
}
