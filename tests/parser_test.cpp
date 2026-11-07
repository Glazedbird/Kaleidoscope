#include "Parser.h"
#include "token.h"

#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void Check(bool Condition, const std::string &Message) {
    if (!Condition)
        throw std::runtime_error(Message);
}

// 比较树的结构，避免仅凭“解析成功”漏掉优先级或结合性错误。
std::string Describe(const ExprAST &Expr) {
    if (auto Number = dynamic_cast<const NumberExprAST *>(&Expr)) {
        std::ostringstream Stream;
        Stream << Number->getValue();
        return Stream.str();
    }
    if (auto Variable = dynamic_cast<const VariableExprAST *>(&Expr))
        return Variable->getName();
    if (auto Binary = dynamic_cast<const BinaryExprAST *>(&Expr))
        return std::string("(") + Binary->getOp() + " " +
               Describe(Binary->getLHS()) + " " + Describe(Binary->getRHS()) + ")";
    if (auto Call = dynamic_cast<const CallExprAST *>(&Expr)) {
        std::string Result = "(call " + Call->getCallee();
        for (const auto &Arg : Call->getArgs())
            Result += " " + Describe(*Arg);
        return Result + ")";
    }
    throw std::runtime_error("unknown AST node");
}

void NextStatement() {
    Check(CurTok == ';', "parser must stop before the statement delimiter");
    getNextToken();
}

} // namespace

int main() {
    try {
        getNextToken();
        const char *Expected[] = {
            "(+ 1 (* 2 3))", "(* (+ 1 2) 3)", "(- (- 10 3) 2)",
            "(< (+ 1 2) (* 3 4))", "(call foo 1 (call bar (+ 2 3)) x)",
            "(call empty)", "x", "12.5",
        };
        for (const char *Tree : Expected) {
            auto Function = ParseTopLevelExpr();
            Check(Function != nullptr, "expected a top-level expression");
            Check(Function->getPrototype().getName() == "__anon_expr",
                  "incorrect anonymous function name");
            Check(Function->getPrototype().getArgs().empty(),
                  "anonymous function must have no parameters");
            Check(Describe(Function->getBody()) == Tree,
                  std::string("unexpected AST; expected ") + Tree);
            NextStatement();
        }

        Check(CurTok == tok_def, "expected def");
        auto Function = ParseDefinition();
        Check(Function != nullptr, "expected a function definition");
        Check(Function->getPrototype().getName() == "combine", "wrong function name");
        Check(Function->getPrototype().getArgs() == std::vector<std::string>({"x", "y"}),
              "wrong function parameters");
        Check(Describe(Function->getBody()) == "(+ x (* y 2))", "wrong function body");
        NextStatement();

        Check(CurTok == tok_extern, "expected extern");
        auto EmptyProto = ParseExtern();
        Check(EmptyProto && EmptyProto->getName() == "clock" &&
              EmptyProto->getArgs().empty(), "wrong zero-argument prototype");
        NextStatement();

        Check(CurTok == tok_extern, "expected extern");
        auto Proto = ParseExtern();
        Check(Proto && Proto->getName() == "foo" &&
              Proto->getArgs() == std::vector<std::string>({"a", "b"}),
              "wrong multi-argument prototype");
        NextStatement();
        Check(CurTok == tok_eof, "expected end of input");
        std::puts("Parser AST tests passed (11 statements).");
    } catch (const std::exception &Error) {
        std::fprintf(stderr, "Test failed: %s\n", Error.what());
        return 1;
    }
}
