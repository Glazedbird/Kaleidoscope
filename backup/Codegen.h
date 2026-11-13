#pragma once

// Initialize the module and LLVM optimization pipeline before AST::codegen().
void InitializeModuleAndManagers();
// Compatibility entry point used by the driver.
void InitializeModule();
// Verify the complete module and write LLVM IR to stdout on success.
bool EmitModule();
