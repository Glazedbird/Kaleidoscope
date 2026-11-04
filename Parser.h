#ifndef KALEIDOSCOPE_PARSER_H
#define KALEIDOSCOPE_PARSER_H

#include "ExprAST.h"
#include <map>

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

// parenexpr ::= '(' expression ')'
// 括号表达式
static std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken();
    auto V = ParseExpression();
    if(!V)
        return nullptr;
    // 发生错误不反悔
    if(CurTok != ')')
        return LogError("expected ')'");
    getNextToken();
    return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
//  这里的 identifier 到底是定义的 什么
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;

    getNextToken();  // eat identifier.

    if (CurTok != '(') // Simple variable ref.
        return std::make_unique<VariableExprAST>(IdName);

    // Call.
    // 处理第二种形式，identifier (expression)
    getNextToken();  // eat (
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok != ')') {
        while (true) {
            if (auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            if (CurTok == ')')
                break;

            // 处理arglists
            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }

  // Eat the ')'.
    getNextToken();

    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
    default:
        return LogError("unknown token when expecting an expression");
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_number:
        return ParseNumberExpr();
    case '(':
        return ParseParenExpr();
    }
}

static std::map<char, int> BinopPrecedence;

// 从这里拿到 算子优先级
static int GetTokPrecedence() {
    if(!isascii(CurTok))
        return -1;
    int TokPrec = BinopPrecedence[CurTok];
    if(TokPrec <= 0) return -1;
    return TokPrec;
}

static std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if(!LHS)
        return nullptr;
    return ParseBinOpRHS(0, std::move(LHS));
}

/// binoprhs
/// ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS){
    while(true) {
        
        int TokPrec = GetTokPrecedence();
    
        if(TokPrec < ExprPrec)
            return LHS;
        
        int BIinOp = CurTok;
        getNextToken();

        auto RHS = ParsePrimary();
        if(!RHS)
            return nullptr;



    }

}

int main() {
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;
}

#endif
