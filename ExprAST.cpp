#include "ExprAST.h"

#include <utility>

NumberExprAST::NumberExprAST(double val) : m_val(val) {}

VariableExprAst::VariableExprAst(const std::string &name) : m_name(name) {}

BinaryExprAst::BinaryExprAst(char op, std::unique_ptr<ExprAST> LHS,
                           std::unique_ptr<ExprAST> RHS)
    : m_op(op), m_LHS(std::move(LHS)), m_RHS(std::move(RHS)) {}

CallExprAST::CallExprAST(const std::string &callee,
                       std::vector<std::unique_ptr<ExprAST>> args)
    : m_callee(callee), m_args(std::move(args)) {}

PrototypeAST::PrototypeAST(const std::string &name, std::vector<std::string> args)
    : m_name(name), m_args(std::move(args)) {}

const std::string &PrototypeAST::getName() const { return m_name; }

FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> proto,
                       std::unique_ptr<ExprAST> body)
    : m_proto(std::move(proto)), m_body(std::move(body)) {}
