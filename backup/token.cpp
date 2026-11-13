#include "token.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

std::string IdentifierStr;
double NumVal = 0;

namespace {

// 统一读取入口，方便在一处断点观察每一次字符读取。
// int getchar() {
//     const int Value = getchar();
// #ifdef DEBUG
//     const bool debug_eof = Value == EOF;
//     // EOF 没有对应字符；此时仅以 '\0' 占位，通过 debug_eof 判断。
//     [[maybe_unused]] const char debug_char =
//         debug_eof ? '\0' : static_cast<char>(Value);
// #endif
//     return Value; // 在这一行设置断点，查看 Value、debug_char 和 debug_eof。
// }

} // namespace

int gettok() {
    // 这里是 static 的 怪不得
    static int LastChar = ' ';

    while (isspace(LastChar))
        LastChar = getchar();
    
    // 
    // 这里是 在提取所谓的 string 或者 keyword 关键词
    if(isalpha(LastChar)) {
        IdentifierStr = LastChar;
        // 这里是把连续的字符都提取出来
        while (isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;
        
        // 这里每次检查完以后都会来看这个地方是什么keyword
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
    
    // 看懂了，如果上一个字符不是 空格的话，他是不会读取下一个字符的，所以这里提前读了一个字符，真的逆天的不定时
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;

    
}

