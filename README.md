# Kaleidoscope 词法分析测试

需要 `g++` 和 `make`。

编译并运行示例：

```bash
make demo
```

程序读取 `examples/demo.k`，逐行打印识别出的 token。比如 `def` 输出
`tok_def`，`add` 输出 `tok_identifier: add`，`3.5` 输出
`tok_number: 3.5`。括号、运算符等输出 `character`，注释会被跳过，
输入结束后输出 `tok_eof`。

手动输入代码：

```bash
make run
```

输入一行代码并按回车查看结果。在 Linux/macOS 终端中，在空行按
`Ctrl+D` 结束输入；`Ctrl+C` 可以直接退出。

也可以编译后通过管道或文件输入：

```bash
make
printf 'def foo(x) x + 1; # comment\n' | ./build/token
./build/token < examples/demo.k
```

当前程序只测试词法分析，不会计算表达式或执行函数。

## 代码组织

| 文件 | 职责 |
| --- | --- |
| `token.h` / `token.cpp` | token 枚举、`gettok()` 和词法分析结果 |
| `ExprAST.h` / `ExprAST.cpp` | AST 类定义及构造函数实现 |
| `Parser.h` / `Parser.cpp` | Parser 接口和实现，已迁入数字表达式解析代码 |
| `main.cpp` | 当前的 token 打印测试入口 |

后续解析函数可以继续写在 `Parser.cpp` 中。该文件已经通过头文件引用
词法分析接口和 AST 类型，无需 `#include` 任何 `.cpp` 文件。
`make` 会编译并链接这些源文件，修改头文件后也会重新编译依赖它的文件。

目前 Parser 只包含已有的数字表达式解析函数。以后要在入口调用它时，
包含 `Parser.h` 和 `token.h`，先调用 `getNextToken()` 初始化 `CurTok`，
再根据 `CurTok` 选择解析函数。例如输入以数字开头时：

```cpp
getNextToken();
if (CurTok == tok_number) {
    auto Expr = ParseNumberExpr();
}
```

`gettok()` 读取一个 token，并更新相应的 `IdentifierStr` 或 `NumVal`；
`getNextToken()` 在此基础上还会更新 Parser 的 `CurTok`。
解析函数应使用 `getNextToken()` 推进输入。
