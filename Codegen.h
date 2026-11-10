#pragma once

// One compilation session per process, initialized before AST::codegen().
void InitializeModule();
// Verify the complete module and write LLVM IR to stdout on success.
bool EmitModule();
