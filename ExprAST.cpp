#include "ExprAST.h"
#include <utility>

NumberExprAST::NumberExprAST(double val) : m_val(val) {}

VariableExprAST::VariableExprAST(const std::string &name) : m_name(name) {}

BinaryExprAST::BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
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

// 这里的话，ContantFP 是表示数字常量， 它内部用 APFloat 保存数值
Value* NumberExprAST::codegen() {
    // 创建一个 ConstantFP的类型
    return ConstantFP::get(*TheContext, APFloat(m_val));
}

Value *VariableExprAST::codegen() {
    // 这边创建一个？
    // 假定映射已经存在
    Value* V = NamedValues[m_name];
    if(!V)
        LogErrorV("Unknown variable name");
    return V;
}

// 一个后序便利，在根据符号来添加实际的语句
Value *BinaryExprAST::codegen() {
    Value *L = m_LHS->codegen();
    Value *R = m_RHS->codegen();
    if (!L || !R)
        return nullptr;

    switch (m_op) {
        case '+':
        return Builder->CreateFAdd(L, R, "addtmp");
        case '-':
        return Builder->CreateFSub(L, R, "subtmp");
        case '*':
        return Builder->CreateFMul(L, R, "multmp");
        case '<':
        L = Builder->CreateFCmpULT(L, R, "cmptmp");
        // Convert bool 0/1 to double 0.0 or 1.0
        return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext),
                                        "booltmp");
        default:
        return LogErrorV("invalid binary operator");
    }
}

Value *CallExprAST::codegen() {
    // Look up the name in the global module table.
    // 这个 TheModule 是有全局符号表的是吗？
    Function *CalleeF = TheModule->getFunction(m_callee);
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    // If argument mismatch error
    //.
    if (CalleeF->arg_size() != m_args.size())
        return LogErrorV("Incorrect # arguments passed");

    // 后面把参数变量也给推进来！
    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = m_args.size(); i != e; ++i) {
        ArgsV.push_back(m_args[i]->codegen());
    if (!ArgsV.back())
        return nullptr;
    }
    // 最后用 Builder 构造类型
    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Function *PrototypeAST::codegen() {

    std::vector<Type*> Doubles(m_args.size(),
                                Type::getDoubleTy(*TheContext));
    
    // 为指定原型 创建 functiontype，创建类型
    // false 表示不是可变参数函数
    FunctionType *FT =
        FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);
    
    // 这个是 真正创建了一个 IR function
    // 函数类型、链接属性、名称、要插入的模块？
    // 意味着函数可以定义到当前模块之外
    // 传入的名称最终会被注册到 TheModule 的符号表中
    Function *F = 
        Function::Create(FT, Function::ExternalLinkage, m_name, TheModule.get());

    unsigned Idx = 0;
    for (auto &Arg : F -> args())
        Arg.setName(m_args[Idx ++]);

    return F;
}

// BUG：修复点，后面可以参考
Function *FunctionAST::codegen() {
    // 首先需要解析是不是已经存在函数的声明了
    Function *TheFunction = TheModule -> getFunction(m_proto -> getName());

    if(!TheFunction)
        TheFunction = m_proto -> codegen();
    
    if(!TheFunction)
        return nullptr;
    
    if (!TheFunction->empty())
        return (Function*)LogErrorV("Function cannot be redefined.");

    BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder -> SetInsertPoint(BB);

    NamedValues.clear();

    for (auto &Arg : TheFunction->args())
        NamedValues[std::string(Arg.getName())] = &Arg;

    // 这里是说如果有函数体，就调用它的 codegen 方法
    if (Value *RetVal = m_body->codegen()) {
        // 这里是返回参数
        Builder -> CreateRet(RetVal);

        // 确定
        verifyFunction(*TheFunction);
        
        return TheFunction;
    }
    // 如果 没有 函数body，就直接删除掉函数
    TheFunction -> eraseFromParent();
    return nullptr;
} 
