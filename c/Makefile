# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS =

# Directories
SRC_DIR = src
ANALYZER_DIR = $(SRC_DIR)/analyzer
TEST_DIR = test
TEST_BUILD_DIR = $(TEST_DIR)/build
BUILD_DIR = build

# Target executable
TARGET = ast-analyzer
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
endif

# Source files
SOURCES = $(SRC_DIR)/main.c $(ANALYZER_DIR)/analyzer.c
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/analyzer.o

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Create test build directory
$(TEST_BUILD_DIR):
	mkdir -p $(TEST_BUILD_DIR)

# Build main executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile main.c
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(ANALYZER_DIR)/analyzer.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile analyzer.c
$(BUILD_DIR)/analyzer.o: $(ANALYZER_DIR)/analyzer.c $(ANALYZER_DIR)/analyzer.h
	$(CC) $(CFLAGS) -c $< -o $@

# Run tests
test: $(TARGET) $(TEST_BUILD_DIR)
	@echo "Stripping types from example.ts..."
	./$(TARGET) -f $(TEST_DIR)/example.ts -o $(TEST_BUILD_DIR)/example.js
	@echo "JavaScript output saved to test/build/example.js"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TEST_BUILD_DIR)
	rm -f $(TARGET)
	@echo "Clean complete"

# Install (optional - copies to /usr/local/bin on Unix-like systems)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Phony targets
.PHONY: all clean test install uninstall

# Help
help:
	@echo "Available targets:"
	@echo "  all      - Build the project (default)"
	@echo "  test     - Run the type stripper on example.ts"
	@echo "  clean    - Remove build artifacts"
	@echo "  install  - Install to /usr/local/bin (Unix-like systems)"
	@echo "  help     - Show this help message"
