#include "Codegen.h"
#include "ExprAST.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <set>

using namespace llvm;
namespace {
// Destruction order: builder, module, context. No state is defined in headers.
std::unique_ptr<LLVMContext> Context;
std::unique_ptr<Module> TheModule;
std::unique_ptr<IRBuilder<>> Builder;
std::map<std::string, Value *> NamedValues;
Value *LogErrorV(const char *message) {
    errs() << "Error: " << message << '\n';
    return nullptr;
}
}

void InitializeModule() {
    Builder.reset();
    TheModule.reset();
    Context = std::make_unique<LLVMContext>();
    TheModule = std::make_unique<Module>("Kaleidoscope", *Context);
    Builder = std::make_unique<IRBuilder<>>(*Context);
    NamedValues.clear();
}

bool EmitModule() {
    if (verifyModule(*TheModule, &errs()))
        return false;
    TheModule->print(outs(), nullptr);
    return true;
}

Value *NumberExprAST::codegen() {
    return ConstantFP::get(*Context, APFloat(m_val));
}
Value *VariableExprAST::codegen() {
    auto it = NamedValues.find(m_name);
    return it == NamedValues.end() ? LogErrorV("Unknown variable name") : it->second;
}
Value *BinaryExprAST::codegen() {
    Value *L = m_LHS->codegen();
    if (!L) return nullptr;
    Value *R = m_RHS->codegen();
    if (!R) return nullptr;
    switch (m_op) {
    case '+': return Builder->CreateFAdd(L, R, "addtmp");
    case '-': return Builder->CreateFSub(L, R, "subtmp");
    case '*': return Builder->CreateFMul(L, R, "multmp");
    case '<':
        return Builder->CreateUIToFP(Builder->CreateFCmpULT(L, R, "cmptmp"),
                                    Type::getDoubleTy(*Context), "booltmp");
    default: return LogErrorV("invalid binary operator");
    }
}
Value *CallExprAST::codegen() {
    Function *callee = TheModule->getFunction(m_callee);
    if (!callee) return LogErrorV("Unknown function referenced");
    if (callee->arg_size() != m_args.size())
        return LogErrorV("Incorrect # arguments passed");
    std::vector<Value *> args;
    for (auto &arg : m_args) {
        Value *value = arg->codegen();
        if (!value) return nullptr;
        args.push_back(value);
    }
    return Builder->CreateCall(callee, args, "calltmp");
}
Function *PrototypeAST::codegen() {
    std::set<std::string> names(m_args.begin(), m_args.end());
    if (names.size() != m_args.size()) {
        LogErrorV("Duplicate argument name");
        return nullptr;
    }
    if (Function *existing = TheModule->getFunction(m_name)) {
        if (existing->arg_size() != m_args.size()) {
            LogErrorV("Conflicting function declaration");
            return nullptr;
        }
        return existing;
    }
    std::vector<Type *> doubles(m_args.size(), Type::getDoubleTy(*Context));
    auto *type = FunctionType::get(Type::getDoubleTy(*Context), doubles, false);
    auto *function = Function::Create(type, Function::ExternalLinkage, m_name, *TheModule);
    unsigned i = 0;
    for (auto &arg : function->args()) arg.setName(m_args[i++]);
    return function;
}
Function *FunctionAST::codegen() {
    const bool existed = TheModule->getFunction(m_proto->getName()) != nullptr;
    Function *function = m_proto->codegen();
    if (!function) return nullptr;
    if (!function->empty()) {
        LogErrorV("Function cannot be redefined.");
        return nullptr;
    }
    Builder->SetInsertPoint(BasicBlock::Create(*Context, "entry", function));
    NamedValues.clear();
    // Definition parameter names may differ from an earlier extern declaration.
    for (auto &arg : function->args()) arg.setName("");
    unsigned i = 0;
    for (auto &arg : function->args()) {
        const auto &name = m_proto->getArgs()[i++];
        arg.setName(name);
        NamedValues[name] = &arg;
    }
    Value *value = m_body->codegen();
    bool valid = false;
    if (value) {
        Builder->CreateRet(value);
        valid = !verifyFunction(*function, &errs());
    }
    Builder->ClearInsertionPoint();
    NamedValues.clear();
    if (valid) return function;
    // Keep a pre-existing declaration (and its callers) valid after failure.
    if (existed) function->deleteBody();
    else function->eraseFromParent();
    return nullptr;
}
