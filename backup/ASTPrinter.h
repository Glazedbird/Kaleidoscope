#ifndef KALEIDOSCOPE_AST_PRINTER_H
#define KALEIDOSCOPE_AST_PRINTER_H

#include "ExprAST.h"

#include <iosfwd>

// 只读遍历已构造的树，不消费 token，也不改变节点所有权。
void DumpAST(const ExprAST &Expr, std::ostream &Out, int Indent = 0);
void DumpAST(const PrototypeAST &Proto, std::ostream &Out, int Indent = 0);
void DumpAST(const FunctionAST &Function, std::ostream &Out, int Indent = 0);

#endif
