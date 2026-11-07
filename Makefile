CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -g
TARGET = build/kaleidoscope
SOURCES = main.cpp token.cpp ExprAST.cpp Parser.cpp
OBJECTS = $(SOURCES:%.cpp=build/%.o)
DEPS = $(OBJECTS:.o=.d)
TEST_TARGET = build/parser_test
TEST_OBJECT = build/tests/parser_test.o
DEPS += $(TEST_OBJECT:.o=.d)

.PHONY: all run demo test clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@

build/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I. -MMD -MP -c $< -o $@

$(TEST_TARGET): $(TEST_OBJECT) build/token.o build/ExprAST.o build/Parser.o
	$(CXX) $(CXXFLAGS) $^ -o $@

-include $(DEPS)

run: $(TARGET)
	./$(TARGET)

demo: $(TARGET)
	./$(TARGET) < examples/demo.k

test: $(TARGET) $(TEST_TARGET)
	./$(TEST_TARGET) < tests/parser_cases.k
	python3 tests/cli_test.py ./$(TARGET)

clean:
	rm -rf build
