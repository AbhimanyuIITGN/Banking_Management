# Banking Management System Makefile

CXX = /usr/local/opt/llvm/bin/clang++
CXXFLAGS = -std=c++17 -stdlib=libc++ -Wall -Wextra -O2 -I include -DUSE_SQLITE
LDFLAGS = -pthread -L/usr/local/lib -lsqlite3 -stdlib=libc++

# Directories
SRCDIR = src
INCDIR = include
BUILDDIR = build
BINDIR = bin

# Source files
COMMON_SOURCES = $(SRCDIR)/User.cpp $(SRCDIR)/Account.cpp $(SRCDIR)/Transaction.cpp \
                 $(SRCDIR)/DatabaseHandler.cpp $(SRCDIR)/BankSystem.cpp $(SRCDIR)/Security.cpp \
                 $(SRCDIR)/DeadlockPrevention.cpp $(SRCDIR)/Encryption.cpp $(SRCDIR)/NetworkProtocol.cpp \
                 $(SRCDIR)/JsonHandler.cpp

MAIN_SOURCES = $(COMMON_SOURCES) $(SRCDIR)/main.cpp
SERVER_SOURCES = $(COMMON_SOURCES) $(SRCDIR)/BankServer.cpp $(SRCDIR)/bank_server_main.cpp

MAIN_OBJECTS = $(MAIN_SOURCES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)
SERVER_OBJECTS = $(SERVER_SOURCES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

# Targets
MAIN_TARGET = $(BINDIR)/banking_system
SERVER_TARGET = $(BINDIR)/bank_server

# Default target
all: directories $(MAIN_TARGET) $(SERVER_TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BUILDDIR) $(BINDIR)

# Build targets
$(MAIN_TARGET): $(MAIN_OBJECTS)
	$(CXX) $(MAIN_OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build successful! Main executable: $(MAIN_TARGET)"

$(SERVER_TARGET): $(SERVER_OBJECTS)
	$(CXX) $(SERVER_OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build successful! Server executable: $(SERVER_TARGET)"



# Compile source files
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -DUSE_SQLITE -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILDDIR) $(BINDIR)
	@echo "Clean complete"

# Install dependencies (SQLite)
install-deps:
	brew install sqlite3

# Run programs
run: $(MAIN_TARGET)
	./$(MAIN_TARGET)

run-server: $(SERVER_TARGET)
	./$(SERVER_TARGET)



# Debug builds
debug: CXXFLAGS += -g -DDEBUG
debug: all

# Test build (compile only)
test-compile: directories $(MAIN_OBJECTS) $(SERVER_OBJECTS)
	@echo "Compilation test successful"

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build banking system and server (default)"
	@echo "  clean        - Clean build files"
	@echo "  run          - Build and run main banking system"
	@echo "  run-server   - Build and run bank server"
	@echo "  debug        - Build with debug symbols"
	@echo "  test-compile - Test compilation only"
	@echo "  install-deps - Install required dependencies"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Note: ATM client is now in ATM_Machine/ folder"
	@echo "      cd ATM_Machine && make run"

.PHONY: all clean run run-server debug test-compile install-deps help directories
