#ifndef KALEIDOSCOPE_EXPR_AST_H
#define KALEIDOSCOPE_EXPR_AST_H

#include <memory>
#include <string>
#include <vector>
#include "header.h"

class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual Value *codegen() = 0;
};

class NumberExprAST : public ExprAST {
    double m_val;

public:
    NumberExprAST(double val);
    double getValue() const { return m_val; }
    Value *codegen() override;
};

// 一个变量其实是一个名字
class VariableExprAST : public ExprAST {
    std::string m_name;

public:
    VariableExprAST(const std::string &name);
    const std::string &getName() const { return m_name; }
    Value *codegen() override;
};

class BinaryExprAST : public ExprAST {
    char m_op;
    std::unique_ptr<ExprAST> m_LHS, m_RHS;

public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS);
    char getOp() const { return m_op; }
    const ExprAST &getLHS() const { return *m_LHS; }
    const ExprAST &getRHS() const { return *m_RHS; }
    Value *codegen() override;
};

// 函数调用：保存函数名和实参表达式。
class CallExprAST : public ExprAST {
    std::string m_callee;
    std::vector<std::unique_ptr<ExprAST>> m_args;

public:
    CallExprAST(const std::string &callee,
                std::vector<std::unique_ptr<ExprAST>> args);
    const std::string &getCallee() const { return m_callee; }
    const std::vector<std::unique_ptr<ExprAST>> &getArgs() const { return m_args; }
    Value *codegen() override;
};

// 函数原型：保存函数名和形参名。
class PrototypeAST {
    std::string m_name;
    std::vector<std::string> m_args;

public:
    PrototypeAST(const std::string &name, std::vector<std::string> args);
    const std::string &getName() const;
    const std::vector<std::string> &getArgs() const { return m_args; }
    Function* codegen();

};

// 函数定义：包含函数原型和函数体。
class FunctionAST {
    std::unique_ptr<PrototypeAST> m_proto;
    std::unique_ptr<ExprAST> m_body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto,
                std::unique_ptr<ExprAST> body);
    const PrototypeAST &getPrototype() const { return *m_proto; }
    const ExprAST &getBody() const { return *m_body; }
    Function *codegen();
};

#endif
