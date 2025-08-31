cmake_minimum_required(VERSION 3.16)
project(BankingManagementSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# For now, use SQLite for easier setup
message(STATUS "Using SQLite database")
set(USE_SQLITE ON)

# Find SQLite3
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Include directories
include_directories(include)
include_directories(${SQLITE3_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/main.cpp
    src/User.cpp
    src/Account.cpp
    src/Transaction.cpp
    src/BankSystem.cpp
    src/DatabaseHandler.cpp
    src/Security.cpp
    src/DeadlockPrevention.cpp
)

# Create executable
add_executable(banking_system ${SOURCES})

# Link libraries
target_link_libraries(banking_system Threads::Threads)
target_link_libraries(banking_system ${SQLITE3_LIBRARIES})
target_compile_definitions(banking_system PRIVATE USE_SQLITE)

# Add compiler flags
target_compile_options(banking_system PRIVATE ${SQLITE3_CFLAGS_OTHER})

# Compiler flags
target_compile_options(banking_system PRIVATE -Wall -Wextra -O2)

# Create directories for build
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(banking_system PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
