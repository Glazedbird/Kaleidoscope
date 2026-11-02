CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -g
TARGET = build/token
SOURCES = main.cpp token.cpp ExprAST.cpp Parser.cpp
OBJECTS = $(SOURCES:%.cpp=build/%.o)
DEPS = $(OBJECTS:.o=.d)

.PHONY: all run demo clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@

build/%.o: %.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

run: $(TARGET)
	./$(TARGET)

demo: $(TARGET)
	./$(TARGET) < examples/demo.k

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPS)
