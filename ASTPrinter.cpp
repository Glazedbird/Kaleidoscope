#include "ASTPrinter.h"

#include <ostream>
#include <stdexcept>
#include <string>

void DumpAST(const ExprAST &Expr, std::ostream &Out, int Indent) {
    Out << std::string(Indent, ' ');
    if (auto Number = dynamic_cast<const NumberExprAST *>(&Expr)) {
        Out << "Number " << Number->getValue() << '\n';
    } else if (auto Variable = dynamic_cast<const VariableExprAST *>(&Expr)) {
        Out << "Variable " << Variable->getName() << '\n';
    } else if (auto Unary = dynamic_cast<const UnaryAST *>(&Expr)) {
        Out << "Unary '" << Unary->getOp() << "'\n";
        DumpAST(Unary->getExpr(), Out, Indent + 2);
    } else if (auto Binary = dynamic_cast<const BinaryExprAST *>(&Expr)) {
        Out << "Binary '" << Binary->getOp() << "'\n";
        DumpAST(Binary->getLHS(), Out, Indent + 2);
        DumpAST(Binary->getRHS(), Out, Indent + 2);
    } else if (auto Call = dynamic_cast<const CallExprAST *>(&Expr)) {
        Out << "Call " << Call->getCallee() << '\n';
        for (const auto &Arg : Call->getArgs())
            DumpAST(*Arg, Out, Indent + 2);
    } else {
        throw std::logic_error("AST printer does not support this expression type");
    }
}

void DumpAST(const PrototypeAST &Proto, std::ostream &Out, int Indent) {
    Out << std::string(Indent, ' ') << "Prototype " << Proto.getName() << '(';
    const auto &Args = Proto.getArgs();
    for (std::size_t Index = 0; Index < Args.size(); ++Index) {
        if (Index != 0)
            Out << ", ";
        Out << Args[Index];
    }
    Out << ")\n";
}

void DumpAST(const FunctionAST &Function, std::ostream &Out, int Indent) {
    Out << std::string(Indent, ' ') << "Function\n";
    DumpAST(Function.getPrototype(), Out, Indent + 2);
    DumpAST(Function.getBody(), Out, Indent + 2);
}
