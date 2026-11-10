CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O0 -g3 -fno-omit-frame-pointer
LLVM_CONFIG ?= llvm-config
LLVM_INCLUDEDIR := $(shell $(LLVM_CONFIG) --includedir 2>/dev/null)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags --libs core --system-libs 2>/dev/null)
CPPFLAGS += -I. -isystem $(LLVM_INCLUDEDIR)
LDLIBS += $(LLVM_LDFLAGS)
# DEBUG=1 定义预处理宏；DEBUG=0 不定义，适用于 #ifdef DEBUG。
DEBUG ?= 1
ifeq ($(DEBUG),1)
CPPFLAGS += -DDEBUG
endif
DEBUG_CONFIG = build/.debug-config
GDB ?= gdb
INPUT ?= examples/demo.k
TARGET = build/kaleidoscope
SOURCES = main.cpp token.cpp ExprAST.cpp Parser.cpp ASTPrinter.cpp Codegen.cpp
OBJECTS = $(SOURCES:%.cpp=build/%.o)
DEPS = $(OBJECTS:.o=.d)
TEST_TARGET = build/parser_test
TEST_OBJECT = build/tests/parser_test.o
DEPS += $(TEST_OBJECT:.o=.d)

.PHONY: check-llvm ir all debug gdb ast run demo test clean FORCE

all: $(TARGET)

# 默认构建也保留完整调试信息，不另建容易与源码不同步的调试程序。
debug: $(TARGET)

check-llvm:
	@command -v "$(LLVM_CONFIG)" >/dev/null 2>&1 || { echo "缺少 llvm-config：请安装 LLVM 开发包，或指定 LLVM_CONFIG=llvm-config-18" >&2; exit 1; }

$(OBJECTS) $(TEST_OBJECT): | check-llvm

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

build/%.o: %.cpp Makefile $(DEBUG_CONFIG)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

# 切换 DEBUG 或 LLVM 工具链时重建对象，避免复用旧配置。
$(DEBUG_CONFIG): FORCE
	@mkdir -p $(dir $@)
	@printf '%s\n' '$(DEBUG)|$(LLVM_CONFIG)|$(LLVM_INCLUDEDIR)|$(LLVM_LDFLAGS)' | cmp -s - $@ || printf '%s\n' '$(DEBUG)|$(LLVM_CONFIG)|$(LLVM_INCLUDEDIR)|$(LLVM_LDFLAGS)' > $@

FORCE:

$(TEST_TARGET): $(TEST_OBJECT) build/token.o build/ExprAST.o build/Parser.o build/Codegen.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

-include $(DEPS)

run: $(TARGET)
	./$(TARGET)

demo: $(TARGET)
	./$(TARGET) < examples/demo.k

# 查看每条语句的完整 AST：make ast INPUT=examples/demo.k
ast: $(TARGET)
	./$(TARGET) --dump-ast < "$(INPUT)"

# 在预设断点停下，逐步观察解析与所有权转移。
gdb: debug
	@command -v "$(GDB)" >/dev/null 2>&1 || { echo "未找到 GDB；Ubuntu/Debian 请先执行 sudo apt install gdb" >&2; exit 1; }
	$(GDB) -q -x debug/ast.gdb -ex 'run --dump-ast < "$(INPUT)"' ./$(TARGET)

test: $(TARGET) $(TEST_TARGET)
	./$(TEST_TARGET) < tests/parser_cases.k
	python3 tests/cli_test.py ./$(TARGET)
	python3 tests/codegen_test.py ./$(TARGET) "$(shell $(LLVM_CONFIG) --bindir)"

clean:
	rm -rf build

# 输出可交给 llvm-as / clang 的完整模块。
ir: $(TARGET)
	./$(TARGET) --emit-ir < "$(INPUT)"
