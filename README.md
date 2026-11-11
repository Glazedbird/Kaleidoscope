# Kaleidoscope：解析、AST 与 LLVM IR 生成

当前实现支持读取 Kaleidoscope 源码并构建 AST，覆盖数字、变量、括号、
二元运算、函数调用、函数定义和 `extern` 声明。默认像教程一样逐条输出
LLVM IR；`--emit-ir` 则导出经过验证的完整 IR 模块。程序本身尚未集成 JIT，不会直接执行表达式。

需要支持 C++17 的 `g++`、`make` 和 LLVM 开发包；测试还需要 Python 3、
同版本的 `llvm-as` 与 `clang`。本次在 LLVM 18.1.3 上验证。

Ubuntu/Debian 安装依赖的示例（具体包名取决于发行版）：

```bash
sudo apt install g++ make llvm-18-dev llvm-18 clang-18 python3
make LLVM_CONFIG=llvm-config-18
```

默认使用 PATH 中的 `llvm-config`；可通过 `LLVM_CONFIG` 指定版本。
头文件目录、链接库和测试工具目录均从该工具查询。

## 编译和运行

```bash
make demo
```

编译生成 `build/kaleidoscope`，读取 `examples/demo.k`，逐条输出函数定义、
外部声明及顶层表达式对应的 IR。

手动输入代码：

```bash
make run
# 也可以直接执行 ./build/kaleidoscope
```

默认进入教程式交互，每输入一条完整代码就立即显示 IR，例如：

```text
ready> def test(x) 1+2+x;
Read function definition:
define double @test(double %x) {
entry:
  %addtmp = fadd double 3.000000e+00, %x
  ret double %addtmp
}

ready>
```

默认模式也可显式指定为 `--repl`。IR 和诊断按教程输出到标准错误，
`ready>` 只在交互终端出现。函数定义和外部声明保留在当前会话中，后续代码
可以调用它们；顶层表达式生成临时 `__anon_expr`，打印后清理。

在 Linux/macOS 终端中，在空行按 `Ctrl+D` 结束输入；`Ctrl+C` 直接退出。
词法分析器需要看到下一个 token 才能确认当前表达式结束，交互时请使用
分号结束代码并回车，这样会立即输出 IR，无需等待 EOF。换行本身不是语句分隔符。

也可以通过管道或文件输入，非交互模式不显示提示符：

```bash
printf 'def add(x y) x + y; add(12, 3.5);\n' | ./build/kaleidoscope
./build/kaleidoscope < examples/demo.k
```

保留仅解析、AST 和 token 调试模式：

```bash
./build/kaleidoscope --parse < examples/demo.k     # 原来的 Parsed ... 提示
./build/kaleidoscope --dump-ast < examples/demo.k
./build/kaleidoscope --tokens < examples/demo.k
```

## 支持的语法

```text
extern sin(x);
def add(x y) x + y;
add(12, sin(3.5));
(1 + 2) * 3 < 10;
```

- 定义/声明的形参以空白分隔，例如 `def add(x y)`。
- 调用实参以逗号分隔，例如 `add(1, 2)`；支持无参函数和嵌套调用。
- 运算符从低到高为 `<`、`+` / `-`、`*`，同级运算符左结合。
- `#` 开始行注释；顶层分号是语句分隔符，不产生 AST。
- 独立表达式包装成名为 `__anon_expr` 的无参函数。

语法错误输出到标准错误。程序跳到分号或下一条 `def` / `extern` 声明
继续解析；只要遇到过语法错误，最终退出码就是 `1`，全部成功则为 `0`。
`--parse` 和 AST 模式只检查语法。默认交互和 `--emit-ir` 模式还检查变量、
被调用函数、参数数量、重复参数和冲突定义。交互模式出错后继续接受输入，
已成功的 IR 保留在输出中；`--emit-ir` 任意错误均返回 `1`，且不输出部分模块。

## 生成 LLVM IR

```bash
make ir                              # 输出 examples/demo.k 的 LLVM IR
make ir INPUT=examples/demo.k         # 可指定其他输入文件
./build/kaleidoscope --emit-ir < examples/demo.k > build/demo.ll
llvm-as build/demo.ll -o build/demo.bc # 工具版本应与构建时一致
```

重定向 IR 时请直接调用程序，或使用 `make -s ir`，避免 make 的命令回显混入文件。
`--emit-ir` 模式在 EOF 时输出完整模块；交互终端用 Ctrl+D 结束输入。
连续顶层表达式保存为 `__anon_expr.0`、`__anon_expr.1` 等独立函数。
函数必须先声明或定义才能被调用，函数体内可以递归调用自身。

输出 IR 也能与 C 程序链接执行。例如生成一个平均值函数：

```bash
printf 'def average(x y) (x+y)*0.5;\n' | ./build/kaleidoscope --emit-ir > build/average.ll
printf 'extern double average(double,double); int main(void) { return average(3,4) == 3.5 ? 0 : 1; }\n' > build/check.c
clang build/average.ll build/check.c -o build/check
./build/check
```

## 代码组织

| 文件 | 职责 |
| --- | --- |
| `token.h` / `token.cpp` | token 枚举、`gettok()` 和词法分析结果 |
| `ExprAST.h` / `ExprAST.cpp` | AST 类型、构造函数和只读访问接口；头文件仅前向声明 LLVM 类型 |
| `Parser.h` / `Parser.cpp` | 表达式、原型、函数定义及声明的解析 |
| `Codegen.h` / `Codegen.cpp` | LLVM 状态的唯一存储位置、初始化、各 AST 的 `codegen()`、函数与模块验证 |
| `ASTPrinter.h` / `ASTPrinter.cpp` | AST 文本打印，供 `--dump-ast` 和调试使用 |
| `main.cpp` | 唯一生产入口、交互循环、错误恢复，以及解析/token/AST/IR 模式分派 |
| `header.h` | 旧头文件的兼容入口；只转发 `Codegen.h`，不定义 LLVM 全局状态 |
| `reference/toy-chapter3.cpp` | 保留的单文件学习参考，不参与主构建，未作为受测实现维护 |
| `tests/` | AST、CLI、IR 验证、语义错误与链接执行测试 |
| `html/` | 第 3–10 章中文教程及离线图片，入口为 [章节目录](html/index.html) |

`gettok()` 读取一个 token，并更新对应的 `IdentifierStr` 或 `NumVal`；
`getNextToken()` 还会更新 Parser 的 `CurTok`。调用解析接口前先调用
`getNextToken()` 初始化。解析成功后，`CurTok` 停在已解析结构之后的第一个
未消费 token；失败返回 `nullptr` 并输出诊断，由主循环负责恢复。

代码生成模式先调用 `InitializeModule()`，随后对解析成功的节点调用
`codegen()`。定义失败时撤销其函数体；此前存在的声明及调用引用予以保留。
默认交互模式逐条打印函数 IR；`--emit-ir` 在全部输入成功后，由
`EmitModule()` 验证并输出模块。当前实现为单进程、
单输入流的学习编译器，不提供并发或多会话接口。

## 验证和调试

```bash
make test
make ast INPUT=examples/demo.k
make gdb INPUT=examples/demo.k
make clean
```

测试覆盖优先级、括号、左结合、嵌套调用、函数结构、错误恢复、EOF、token
和 AST 模式。代码生成测试进一步用 `llvm-as` 验证 IR，用 `clang` 与 C 驱动
链接并执行，检查比较与算术结果、重复声明、声明/定义参数改名、连续顶层
表达式、语义错误与失败定义的恢复。伪终端测试检查未发送 EOF 时即时输出
IR、连续输入、跨输入函数引用、出错恢复与交互提示。

中文教程正文按官方第 4–10 章翻译，源码、命令和输出保留原文。
后续章节包含 JIT、控制流、可变变量、目标文件和调试信息等学习内容；
它们尚未全部集成进本项目主程序。教程的 LLVM API 应与所安装版本核对。
`docs/` 中现有架构与复盘记录仍描述此前的前端阶段，后续更新按文档整理方案进行。
