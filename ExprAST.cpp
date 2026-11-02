#include <string>
#include <memory>
#include <vector>

class ExprAST {
    public:
        virtual ~ExprAST() = default;
};

class NumberExprAST : public ExprAST {
    double m_val;

    public:
        NumberExprAST(double val)
        : m_val(val)
        {}
};

class VariableExprAst : public ExprAST {
    std::string m_name;
    public:
        VariableExprAst(const std::string &name)
        : m_name(name)
        {}

};


class BinaryExprAst : public ExprAST {
    char m_op;
    std::unique_ptr<ExprAST> m_LHS, m_RHS;

    public:
        BinaryExprAst(char op, std::unique_ptr<ExprAST> LHS,
                      std::unique_ptr<ExprAST> RHS)
                    : m_op(op), m_LHS(std::move(LHS)), m_RHS(std::move(RHS))
                    {}
};
// 描述函数调用
// 每一个参数都是 一个 ExprAST
class CallExprAST : public ExprAST {
    std::string m_callee;
    std::vector<std::unique_ptr<ExprAST>> m_args;

    public:
        CallExprAST(const std::string & callee,
                    std::vector<std::unique_ptr<ExprAST>> args)
        : m_callee(callee), m_args(std::move(args))
        {}
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
  std::string m_name;
  std::vector<std::string> m_args;

public:
  PrototypeAST(const std::string &name, std::vector<std::string> args)
    : m_name(name), m_args(std::move(args)) {}

  const std::string &getName() const { return m_name; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  std::unique_ptr<PrototypeAST> m_proto;
  std::unique_ptr<ExprAST> m_body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> proto,
              std::unique_ptr<ExprAST> body)
    : m_proto(std::move(proto)), m_body(std::move(body)) {}
};

// Parser 是将 内容从lexer中解析成为 AST的

// CurTok 是当前进行解析的 Tok
static int CurTok;
static int getNextToken() {
    return CurTok = gettok();
}

std::unique_ptr<ExprAST> LogError(const char *str) {
  fprintf(stderr, "Error: %s\n", str);
  return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *str) {
  LogError(str);
  return nullptr;
}

static std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto Result = std::make_unique<NumberExprAST>(NumVal);
  getNextToken(); // consume the number
  return std::move(Result);
}