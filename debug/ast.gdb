# make gdb：按示例文件输入启动，在构造过程的关键位置停下。
# n：下一行；s：进入函数；finish：运行至当前函数返回；c：下一断点。
# bt：调用栈；info locals：局部变量；p CurTok：当前 token。
set pagination off
set print pretty on
set print object on
set print elements 0

break ParseDefinition
break ParseBinOpRHS
break BinaryExprAST::BinaryExprAST
break FunctionAST::FunctionAST

# 构造函数入口处成员可能尚未初始化，可观察参数，再 n 到函数返回。
# std::move 后原 unique_ptr 不再拥有子树，子树由父节点持有。
