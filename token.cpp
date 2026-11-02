#include "token.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

std::string IdentifierStr;
double NumVal = 0;

int gettok() {

    static int LastChar = ' ';

    while (isspace(LastChar))
        LastChar = getchar();
    
    // 这里是 在提取所谓的 string 或者 keyword 关键词
    if(isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;
        
        // 这里是改变它的
        if(IdentifierStr == "def")
            return tok_def;
        if(IdentifierStr == "extern")
            return tok_extern;
        return tok_identifier;
    }

    // 这里是在 提取数字
    if(isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar =='.');
        
        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    if(LastChar == '#') {
        do
            LastChar = getchar();
            // 检查是否是文末
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');   
        
        // 这是什么情况
        // 其实他是把注释跳过了，然后再按照是否是文末补一个 gettok
        if(LastChar != EOF)
            return gettok();
    }

    if(LastChar == EOF)
        return tok_eof;
    
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;

    
}

