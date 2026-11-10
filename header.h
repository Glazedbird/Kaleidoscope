#ifndef HEADER_H
#define HEADER_H

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

// 持有 llvm 的 核心数据结构
static std::unique_ptr<LLVMContext> TheContext;
// 帮助生成 LLVM 指令的辅助对象，
static std::unique_ptr<IRBuilder<>> Builder;
// 用于包含 函数 与 全局变量
static std::unique_ptr<Module> TheModule;
// 映射当前作用域中的 变量和 llvm中的值
static std::map<std::string, Value *> NamedValues;

Value *LogErrorV(const char *Str);

#endif