# Kaleidoscope 词法分析与语法解析

当前实现支持读取 Kaleidoscope 源码并构建 AST，覆盖数字、变量、括号、
二元运算、函数调用、函数定义和 `extern` 声明。尚未生成 LLVM IR，也不会
计算表达式或执行函数。

需要 `g++`（支持 C++17）和 `make`；运行测试还需要 Python 3。

## 编译和运行

```bash
make demo
```

编译生成 `build/kaleidoscope`，读取 `examples/demo.k`，输出：

```text
Parsed a function definition: add
Parsed an extern: sin
Parsed a top-level expression
Parsed a top-level expression
```

手动输入代码：

```bash
make run
```

交互终端会显示 `ready>`。可以输入 `1 + 2;` 或 `def add(x y) x + y;`。
在 Linux/macOS 终端中，在空行按 `Ctrl+D` 结束输入；`Ctrl+C` 直接退出。
词法分析器需要看到下一个 token 才能确认当前表达式结束，交互时建议使用
分号结束语句。

也可以通过管道或文件输入，非交互模式不显示提示符：

```bash
printf 'def add(x y) x + y; add(12, 3.5);\n' | ./build/kaleidoscope
./build/kaleidoscope < examples/demo.k
```

保留原有的 token 打印模式：

```bash
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
目前只做语法解析，不检查变量是否定义、函数是否存在或调用实参数量。

## 代码组织

| 文件 | 职责 |
| --- | --- |
| `token.h` / `token.cpp` | token 枚举、`gettok()` 和词法分析结果 |
| `ExprAST.h` / `ExprAST.cpp` | AST 类型、构造函数和只读访问接口 |
| `Parser.h` | Parser 的对外声明，不定义函数实现或程序入口 |
| `Parser.cpp` | 表达式、原型、函数定义及声明的解析；内部辅助函数放入匿名命名空间 |
| `main.cpp` | 唯一程序入口、交互主循环、错误恢复和 `--tokens` 模式 |
| `tests/` | AST 结构校验，以及命令行输入、错误恢复和退出码测试 |

`gettok()` 读取一个 token，并更新对应的 `IdentifierStr` 或 `NumVal`；
`getNextToken()` 还会更新 Parser 的 `CurTok`。调用解析接口前先调用
`getNextToken()` 初始化。解析成功后，`CurTok` 停在已解析结构之后的第一个
未消费 token；失败返回 `nullptr` 并输出诊断，由主循环负责恢复。

## 验证

```bash
make test
```

AST 测试检查优先级、括号、左结合、嵌套调用、形参和匿名函数结构。
命令行测试检查正常输入、空输入、注释、语法错误、错误恢复、EOF 和
`--tokens` 模式，并通过超时检查避免主循环卡死。

```bash
make clean  # 删除生成的 build 目录
```
