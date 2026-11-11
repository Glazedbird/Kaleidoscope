#include "Parser.h"
#include "ASTPrinter.h"
#include "token.h"
#include "Codegen.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

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

enum class Mode { Repl, Parse, AST, EmitIR };

void PrintIR(const llvm::Function &Function, const char *Message) {
    llvm::errs() << Message << "\n";
    Function.print(llvm::errs());
    llvm::errs() << "\n";
    llvm::errs().flush();
}

int MainLoop(Mode mode) {
    const bool PrintAST = mode == Mode::AST;
    const bool GenerateIR = mode == Mode::Repl || mode == Mode::EmitIR;
    const bool Interactive = isatty(fileno(stdin));
    bool HadError = false;
    if (Interactive)
        std::fprintf(stderr, "ready> ");
    getNextToken();
    unsigned AnonymousId = 0;

    while (CurTok != tok_eof) {
        bool Parsed = false;
        switch (CurTok) {
        case ';': // 顶层分号用于分隔语句，本身不产生 AST。
            Parsed = true;
            break;
        case tok_def:
            if (auto Function = ParseDefinition()) {
                if (!GenerateIR) std::printf("Parsed a function definition: %s\n",
                            Function->getPrototype().getName().c_str());
                if (PrintAST)
                    DumpAST(*Function, std::cout);
                Parsed = true;
                if (GenerateIR) {
                    if (auto *IR = Function->codegen()) {
                        if (mode == Mode::Repl)
                            PrintIR(*IR, "Read function definition:");
                    } else {
                        HadError = true;
                    }
                }
            }
            break;
        case tok_extern:
            if (auto Proto = ParseExtern()) {
                if (!GenerateIR) std::printf("Parsed an extern: %s\n", Proto->getName().c_str());
                if (PrintAST)
                    DumpAST(*Proto, std::cout);
                Parsed = true;
                if (GenerateIR) {
                    if (auto *IR = Proto->codegen()) {
                        if (mode == Mode::Repl)
                            PrintIR(*IR, "Read extern:");
                    } else {
                        HadError = true;
                    }
                }
            }
            break;
        default:
            if (auto Function = ParseTopLevelExpr()) {
                if (!GenerateIR) std::printf("Parsed a top-level expression\n");
                if (PrintAST)
                    DumpAST(*Function, std::cout);
                Parsed = true;
                if (GenerateIR) {
                    if (auto *IR = Function->codegen()) {
                        if (mode == Mode::Repl) {
                            PrintIR(*IR, "Read top-level expression:");
                            // Temporary expressions must not collide with the next one.
                            IR->eraseFromParent();
                        } else {
                            IR->setName("__anon_expr." + std::to_string(AnonymousId++));
                        }
                    } else {
                        HadError = true;
                    }
                }
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
    if (mode == Mode::EmitIR && !HadError && !EmitModule()) HadError = true;
    return HadError ? 1 : 0;
}

} // namespace

int main(int argc, char **argv) {
    if (argc == 2 && std::string(argv[1]) == "--tokens")
        return PrintTokens();
    Mode mode = Mode::Repl;
    if (argc == 2) {
        const std::string option = argv[1];
        if (option == "--parse") mode = Mode::Parse;
        else if (option == "--dump-ast") mode = Mode::AST;
        else if (option == "--emit-ir") mode = Mode::EmitIR;
        else if (option != "--repl") {
            std::fprintf(stderr, "Usage: %s [--repl | --parse | --tokens | --dump-ast | --emit-ir]\n", argv[0]);
            return 1;
        }
    } else if (argc != 1) {
        std::fprintf(stderr, "Usage: %s [--repl | --parse | --tokens | --dump-ast | --emit-ir]\n", argv[0]);
        return 1;
    }
    if (mode == Mode::Repl || mode == Mode::EmitIR) InitializeModule();
    return MainLoop(mode);
}
