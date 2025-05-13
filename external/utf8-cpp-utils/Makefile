# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra --pedantic

# Directories
TEST_DIR := ./test

# Test files
TEST_SRC := $(TEST_DIR)/test.cpp
TEST_BIN := $(TEST_DIR)/test

# Build and run tests
.PHONY: all test clean

all: test

test: $(TEST_BIN)
	@echo "Running tests..."
	@./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TEST_BIN)
