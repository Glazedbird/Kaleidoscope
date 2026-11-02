#ifndef KALEIDOSCOPE_EXPR_AST_H
#define KALEIDOSCOPE_EXPR_AST_H

#include <memory>
#include <string>
#include <vector>

class ExprAST {
public:
    virtual ~ExprAST() = default;
};

class NumberExprAST : public ExprAST {
    double m_val;

public:
    NumberExprAST(double val);
};

class VariableExprAst : public ExprAST {
    std::string m_name;

public:
    VariableExprAst(const std::string &name);
};

class BinaryExprAst : public ExprAST {
    char m_op;
    std::unique_ptr<ExprAST> m_LHS, m_RHS;

public:
    BinaryExprAst(char op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS);
};

// 函数调用：保存函数名和实参表达式。
class CallExprAST : public ExprAST {
    std::string m_callee;
    std::vector<std::unique_ptr<ExprAST>> m_args;

public:
    CallExprAST(const std::string &callee,
                std::vector<std::unique_ptr<ExprAST>> args);
};

// 函数原型：保存函数名和形参名。
class PrototypeAST {
    std::string m_name;
    std::vector<std::string> m_args;

public:
    PrototypeAST(const std::string &name, std::vector<std::string> args);
    const std::string &getName() const;
};

// 函数定义：包含函数原型和函数体。
class FunctionAST {
    std::unique_ptr<PrototypeAST> m_proto;
    std::unique_ptr<ExprAST> m_body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto,
                std::unique_ptr<ExprAST> body);
};

#endif
