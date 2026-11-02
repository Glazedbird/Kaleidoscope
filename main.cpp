#include "token.h"

#include <cstdio>

// 测试入口：从标准输入读取代码，并打印每次 gettok() 的结果。
int main() {
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
