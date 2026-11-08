CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O0 -g3 -fno-omit-frame-pointer
CPPFLAGS += -I.
# DEBUG=1 定义预处理宏；DEBUG=0 不定义，适用于 #ifdef DEBUG。
DEBUG ?= 1
ifeq ($(DEBUG),1)
CPPFLAGS += -DDEBUG
endif
DEBUG_CONFIG = build/.debug-config
GDB ?= gdb
INPUT ?= examples/demo.k
TARGET = build/kaleidoscope
SOURCES = main.cpp token.cpp ExprAST.cpp Parser.cpp ASTPrinter.cpp
OBJECTS = $(SOURCES:%.cpp=build/%.o)
DEPS = $(OBJECTS:.o=.d)
TEST_TARGET = build/parser_test
TEST_OBJECT = build/tests/parser_test.o
DEPS += $(TEST_OBJECT:.o=.d)

.PHONY: all debug gdb ast run demo test clean FORCE

all: $(TARGET)

# 默认构建也保留完整调试信息，不另建容易与源码不同步的调试程序。
debug: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

build/%.o: %.cpp Makefile $(DEBUG_CONFIG)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

# 切换宏时更新时间戳，避免 make 继续使用上次配置生成的对象文件。
$(DEBUG_CONFIG): FORCE
	@mkdir -p $(dir $@)
	@printf '%s\n' '$(DEBUG)' | cmp -s - $@ || printf '%s\n' '$(DEBUG)' > $@

FORCE:

$(TEST_TARGET): $(TEST_OBJECT) build/token.o build/ExprAST.o build/Parser.o
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

clean:
	rm -rf build
