#ifndef KALEIDOSCOPE_TOKEN_H
#define KALEIDOSCOPE_TOKEN_H

#include <string>

enum Token {
    tok_eof = -1,
    tok_def = -2,
    tok_extern = -3,
    tok_identifier = -4,
    tok_number = -5
};

// 最近读取到的标识符和数值；仅在对应 token 返回后读取。
// extern 只声明变量，实际存储在 token.cpp 中。
extern std::string IdentifierStr;
extern double NumVal;

// 从标准输入读取下一个 token。
int gettok();

#endif
