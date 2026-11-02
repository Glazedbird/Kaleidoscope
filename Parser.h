#ifndef KALEIDOSCOPE_PARSER_H
#define KALEIDOSCOPE_PARSER_H

#include "ExprAST.h"

// 当前等待解析的 token，调用 getNextToken() 后更新。
extern int CurTok;
int getNextToken();

std::unique_ptr<ExprAST> LogError(const char *str);
std::unique_ptr<PrototypeAST> LogErrorP(const char *str);

// 调用前 CurTok 应为 tok_number；成功后前进到下一个 token。
static std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    // 这里需要保证 不定式
    getNextToken(); // consume the number
    return std::move(Result);
};

static std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken();
    auto V = ParseExpression();
    if(!V)
        return nullptr;
    if(CurTok != ')')
        return LogError("expected ')'");
    getNextToken();
    return V;
}

#endif
