#ifndef KALEIDOSCOPE_PARSER_H
#define KALEIDOSCOPE_PARSER_H

#include "ExprAST.h"

// CurTok 是当前尚未消费的 token；解析前先调用 getNextToken()。
extern int CurTok;
int getNextToken();

// 成功时返回 AST，并停在该语法结构之后的第一个 token。
// 失败时输出诊断并返回 nullptr，由调用方决定如何恢复。
std::unique_ptr<ExprAST> ParseExpression();
std::unique_ptr<FunctionAST> ParseDefinition();
std::unique_ptr<PrototypeAST> ParseExtern();
std::unique_ptr<FunctionAST> ParseTopLevelExpr();
std::unique_ptr<ExprAST> ParseUnaryOp();

#endif
