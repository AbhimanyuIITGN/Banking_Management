# ATM Machine Makefile

CXX = /usr/local/opt/llvm/bin/clang++
CXXFLAGS = -std=c++17 -stdlib=libc++ -Wall -Wextra -O2 -I include
LDFLAGS = -pthread -stdlib=libc++

# Directories
SRCDIR = src
INCDIR = include
BUILDDIR = build
BINDIR = bin

# Source files for ATM
ATM_SOURCES = $(SRCDIR)/NetworkProtocol.cpp $(SRCDIR)/JsonHandler.cpp $(SRCDIR)/Encryption.cpp \
              $(SRCDIR)/ATMClient.cpp $(SRCDIR)/atm_main.cpp

ATM_OBJECTS = $(ATM_SOURCES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

# Target
ATM_TARGET = $(BINDIR)/atm_client

# Default target
all: directories $(ATM_TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BUILDDIR) $(BINDIR)

# Build ATM client
$(ATM_TARGET): $(ATM_OBJECTS)
	$(CXX) $(ATM_OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build successful! ATM executable: $(ATM_TARGET)"

# Compile source files
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILDDIR) $(BINDIR)
	@echo "Clean complete"

# Run the ATM client
run: $(ATM_TARGET)
	./$(ATM_TARGET)

# Run ATM client with custom server
run-server: $(ATM_TARGET)
	@echo "Usage: make run-server HOST=<server_ip> PORT=<server_port>"
	@echo "Example: make run-server HOST=192.168.1.100 PORT=8080"
	./$(ATM_TARGET) $(HOST) $(PORT)

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: all

# Test build (compile only)
test-compile: directories $(ATM_OBJECTS)
	@echo "ATM compilation test successful"

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build ATM client (default)"
	@echo "  clean        - Clean build files"
	@echo "  run          - Build and run ATM client (connects to localhost:8080)"
	@echo "  run-server   - Run ATM with custom server (use HOST= and PORT=)"
	@echo "  debug        - Build with debug symbols"
	@echo "  test-compile - Test compilation only"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make run                              # Connect to localhost:8080"
	@echo "  make run-server HOST=192.168.1.100   # Connect to remote server"

.PHONY: all clean run run-server debug test-compile help directories
