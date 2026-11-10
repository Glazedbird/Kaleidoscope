#include "Parser.h"
#include "token.h"
#include "header.h"

#include <cstdio>
#include <map>
#include <utility>

int CurTok = tok_eof;

int getNextToken() {
    return CurTok = gettok();
}

namespace {

// 优先级越大，结合越紧；未列出的 token 结束当前表达式。
const std::map<char, int> BinopPrecedence = {
    {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40},
};

std::unique_ptr<ExprAST> LogError(const char *message) {
    std::fprintf(stderr, "Error: %s\n", message);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *message) {
    LogError(message);
    return nullptr;
}

Value *LogErrorV(const char *Str) {
  LogError(Str);
  return nullptr;
}

int GetTokPrecedence() {
    if (CurTok < 0 || CurTok > 127)
        return -1;
    auto It = BinopPrecedence.find(static_cast<char>(CurTok));
    return It == BinopPrecedence.end() ? -1 : It->second;
}

// numberexpr ::= number
std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    // 它这种地方会完成 Parse
    getNextToken(); // 消费数字。
    return Result;
}

// parenexpr ::= '(' expression ')'
std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken(); // 消费 '('。
    auto Value = ParseExpression();
    if (!Value)
        return nullptr;
    if (CurTok != ')')
        return LogError("expected ')'");
    getNextToken(); // 消费 ')'。
    return Value;
}

// identifierexpr ::= identifier | identifier '(' (expression (',' expression)*)? ')'
std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string Name = IdentifierStr;
    getNextToken(); // 消费标识符。

    if (CurTok != '(')
        return std::make_unique<VariableExprAST>(Name);

    getNextToken(); // 消费 '('。
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok != ')') {
        while (true) {
            auto Arg = ParseExpression();
            if (!Arg)
                return nullptr;
            Args.push_back(std::move(Arg));
            if (CurTok == ')')
                break;
            if (CurTok != ',')
                return LogError("expected ')' or ',' in argument list");
            getNextToken(); // 消费 ','。
        }
    }
    getNextToken(); // 消费 ')'。
    return std::make_unique<CallExprAST>(Name, std::move(Args));
}

// primary ::= identifierexpr | numberexpr | parenexpr
std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_number:
        return ParseNumberExpr();
    case '(':
        return ParseParenExpr();
    default:
        return LogError("unknown token when expecting an expression");
    }
}

// 消费优先级不低于 ExprPrec 的运算符。右侧遇到更高优先级时，
// 先递归合并右侧子树；相同优先级继续循环，从而保持左结合。
std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                       std::unique_ptr<ExprAST> LHS) {
    // 这个大循环 while true 其实是用来不断循环构造左子树的
    while (true) {
        int TokPrec = GetTokPrecedence();
        if (TokPrec < ExprPrec)
            return LHS;

        char BinOp = static_cast<char>(CurTok);
        getNextToken(); // 消费运算符。
        auto RHS = ParsePrimary();
        if (!RHS)
            return nullptr;

        if (TokPrec < GetTokPrecedence()) {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }
        LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS),
                                              std::move(RHS));
    }
}

// prototype ::= identifier '(' identifier* ')'
// 与调用实参不同，原型的形参以空白分隔，沿用教程语法。
std::unique_ptr<PrototypeAST> ParsePrototype() {
    // 这里的这些内容是循环不定式
    if (CurTok != tok_identifier)
        return LogErrorP("expected function name in prototype");
    std::string Name = IdentifierStr;
    getNextToken();
    if (CurTok != '(')
        return LogErrorP("expected '(' in prototype");

    std::vector<std::string> Args;
    while (getNextToken() == tok_identifier)
        Args.push_back(IdentifierStr);
    if (CurTok != ')')
        return LogErrorP("expected ')' in prototype");
    // 这里其实是把上一个字符给吃掉了
    getNextToken();
    return std::make_unique<PrototypeAST>(Name, std::move(Args));
}

} // namespace

// expression ::= primary binoprhs
std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;
    return ParseBinOpRHS(0, std::move(LHS));
}

// definition ::= 'def' prototype expression
// 这里是 前端看到了一个 def 的关键字，然后就 来了
std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken(); // 消费 'def'。
    auto Proto = ParsePrototype();
    if (!Proto)
        return nullptr;
    // 这里的 Body 是一个 Parse
    if (auto Body = ParseExpression())
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
    return nullptr;
}

// external ::= 'extern' prototype
std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken(); // 消费 'extern'。
    return ParsePrototype();
}

// 独立表达式包装成无参匿名函数，便于后续统一生成 LLVM IR。
std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto Body = ParseExpression()) {
        auto Proto = std::make_unique<PrototypeAST>(
            "__anon_expr", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
    }
    return nullptr;
}
