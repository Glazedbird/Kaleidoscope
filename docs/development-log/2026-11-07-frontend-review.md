# 前端开发经历复盘

记录日期：2026-11-07。资料来源：本次对话、当前源码与 Git 历史。核对基线：`ff1210d`。

本记录区分历史验证、当前实现和待验证问题。后续提交体现的改进按 Git 证据记录，不将所有改动归为早期对话中一次完成。

## 目标与完成情况

最初目标是理解 Lexer 的注释处理，建立简单可运行的测试入口，再理解 AST 并整理代码，使后续 Parser 能访问需要的函数、变量和类型。

| 阶段 | 实际工作与证据 |
| --- | --- |
| 早期运行环境 | 在 Lexer 中加入测试入口，提供 Makefile、示例和说明；当时验证了 token 输出、注释后继续读取、注释至 EOF、空输入 |
| AST 与模块整理 | 明确函数调用、原型、定义的不同职责；拆出头文件、实现文件与 `main.cpp`；当时验证跨文件数字解析及 AST 构造 |
| Parser 草稿 | `d3ad9fe`、`63d2035`、`505c13d` 记录了分阶段推进；制作 11 月 5 日 PPT 时，优先级解析和整体联调尚未完成 |
| 当前可运行前端 | `ff1210d` 整理 Parser 头文件和实现，修复 AST 类名，完成解析主循环与错误恢复，并加入 AST 和 CLI 测试 |

当前支持表达式、函数调用、函数定义、`extern` 和顶层匿名函数包装。主程序只构建 AST 并输出解析结果，不生成 LLVM IR、不执行表达式。

## 问题、尝试与结论

### 跨文件接口不可见

最初 Lexer 接口和结果变量使用文件级 `static`，AST 类型没有独立头文件。Parser 放到其他翻译单元后，不能直接使用这些内部名字。

最终采用声明与定义分离：`token.h` 声明公共接口，`token.cpp` 单一定义；AST 类型放入 `ExprAST.h`，Parser 实现独立编译。设计依据见 [ADR-0001](../decisions/0001-frontend-modules-and-ownership.md)。

### AST 参数所有权

对话中曾发现 `CallExprAST` 用 `m_args(args)` 初始化包含 `unique_ptr` 的列表，需改为 `m_args(std::move(args))`。复制独占指针在类型语义上不可行；本记录没有相应的编译器报错实录，不把它写成一次已执行的失败测试。当前构造函数已经使用移动。

### 完整构建失败与模块隔离

11 月 5 日准备汇报时，`make demo` 实际在 `ExprAST.cpp` 编译失败：头文件使用 `VariableExprAST`、`BinaryExprAST`，实现仍使用 `VariableExprAst`、`BinaryExprAst`。

当时单独编译 `main.cpp + token.cpp` 并运行示例成功，确认 Lexer 可用，未因此宣称 Parser 已跑通。该阶段的 `main.cpp` 仍是词法测试入口，历史编译命令不适用于当前已经依赖 Parser 的入口。

最新 `ff1210d` 的 Diff 同步了类名。当前完整重编译通过，因此类名问题属于已修复历史，不再列入待办。

### Parser 草稿与当前实现的差异

历史版本在 `Parser.h` 中混放函数实现及 `main()`，并存在声明顺序、token 依赖和未完成的二元解析问题。当前头文件仅保留公共声明，内部函数移入 `Parser.cpp`，生产入口集中在 `main.cpp`。

`ParseBinOpRHS()` 当前已经实现右侧更高优先级的递归与 `BinaryExprAST` 构造。解析函数约定与主循环恢复策略见 [ADR-0002](../decisions/0002-parser-contract-and-precedence.md)。

### 分词通过不代表语法正确

早期示例中的 `add(12 3.5)` 能被 Lexer 分词，但调用语法要求逗号。当前 [examples/demo.k](../../examples/demo.k) 改为 `add(12, 3.5)`；形参仍使用 `def add(x y)` 的空白分隔规则。CLI 测试覆盖了缺少实参逗号的错误输入。

## 已执行的验证

以下结果来自 2026-11-07 上一轮复盘中的实际运行。本次写文档前重新确认相关源码与 `ff1210d` 一致；本次文档维护没有重复运行测试，也没有修改测试代码。

| 命令 | 实际结果 | 证明范围 |
| --- | --- | --- |
| `make -B test` | 完整重编译与链接成功；输出 `Parser AST tests passed (11 statements).`；Python 输出 `Ran 7 tests` 和 `OK` | 当前主构建、AST 结构、token 边界、CLI 正常输入、错误恢复、EOF、退出码和 token 模式 |
| `make demo` | 输出一条函数定义、一条 extern 和两条顶层表达式的成功提示，退出成功 | 当前示例可由主程序解析，不代表表达式已求值 |

11 条语句不等于 11 个独立测试程序；7 个 CLI 测试方法内还包含子用例。本记录没有性能指标、真实终端交互测试或 `toy.cpp` 构建结果。

## Git 追溯

- `ff1210d`：Parser 实现、主循环、AST 类名修复、访问接口、示例、Makefile 与测试的主要整合。
- `505c13d`：Parser 开发中的头文件草稿，不代表当时已具备当前版本的完整验证结果。
- 可执行 `git diff d3ad9fe..ff1210d -- Parser.h Parser.cpp ExprAST.cpp main.cpp Makefile examples/demo.k` 核对阶段差异。
- 可执行 `git show --stat ff1210d` 核对测试和文档变更范围。

## 后续工作与待确认项

1. 确定 `.`、`1.2.3` 等数字输入的合法性和诊断规则，再实现测试与必要修复。当前风险来自 `gettok()` 扫描规则与未检查转换终点，本次没有运行这些用例。
2. 补充精确的注释终止 token 测试和伪终端交互测试，避免以管道测试替代交互验证。具体方案见[知识整理](../知识整理.md)。
3. 主构建尚未接入 LLVM。`toy.cpp` 含 `codegen()` 代码，但未纳入 Makefile，本次未验证；其角色及集成计划需人工确认。
4. 本次写入前，IDE 显示的 `docs/architecture.md` 与 `docs/知识整理.md` 在磁盘上不存在，因此新建了同名文件；若编辑器仍有未保存内容，应人工比对合并。
5. [11 月 5 日汇报材料](../../reports/Kaleidoscope_组会进展汇报_2页.pptx) 保留当时进度，不自动改写为最新状态。若再次使用，应先决定是否更新。
