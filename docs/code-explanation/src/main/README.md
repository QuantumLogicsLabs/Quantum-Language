# main.cpp - Entry Point and CLI Interface

## Overview
`main.cpp` serves as the entry point for the Quantum Language interpreter. It handles command-line argument parsing, provides both REPL (Read-Eval-Print Loop) and file execution modes, and includes comprehensive error handling and user interface elements.

## Line-by-Line Analysis

### Includes and Dependencies (Lines 1-15)
```cpp
#include "../include/Lexer.h"
#include "../include/Parser.h"
#include "../include/Interpreter.h"
#include "../include/Error.h"
#include "../include/Value.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif
```
- **Lines 1-5**: Core compiler components - Lexer, Parser, Interpreter, Error handling, and Value system
- **Lines 6-11**: Standard C++ libraries for I/O operations, file handling, string manipulation, and algorithms
- **Line 12**: Filesystem library for enhanced file operations (C++17 feature)
- **Lines 13-15**: Windows-specific header for console UTF-8 support

### Global Configuration (Lines 17-23)
```cpp
namespace fs = std::filesystem;

// ─── Globals ──────────────────────────────────────────────────────────────────

// Set to true during --test runs so input() returns "" instantly instead of
// blocking on stdin.
bool g_testMode = false;
```
- **Line 17**: Namespace alias for filesystem operations
- **Lines 19-23**: Global test mode flag for automated testing without blocking input

### Banner Display Function (Lines 27-40)
```cpp
static void printBanner()
{
    std::cout << Colors::CYAN << Colors::BOLD
              << "\n"
              << "  ██████╗ ██╗   ██╗ █████╗ ███╗   ██╗████████╗██╗   ██╗███╗   ███╗\n"
              // ... ASCII art continues ...
              << Colors::RESET
              << Colors::YELLOW << "  Quantum Language v1.0.0 | The Cybersecurity-Ready Scripting Language\n"
              << Colors::RESET << "\n";
}
```
- **Purpose**: Displays the Quantum Language logo and version information
- **Design**: Uses ANSI color codes for visual appeal
- **Static function**: Internal helper, not exported outside this translation unit

### Achievement Display Function (Lines 42-77)
```cpp
static void printAura()
{
    std::cout << Colors::CYAN << Colors::BOLD
              << "\n╔══════════════════════════════════════════════════════════════════╗\n"
              // ... Achievement display continues ...
              << Colors::RESET;
}
```
- **Purpose**: Shows project achievements and statistics in a formatted table
- **Features**: Lists completed features, project statistics, and cybersecurity capabilities
- **UI Design**: Uses Unicode box-drawing characters for professional appearance

### REPL Implementation (Lines 81-120)
```cpp
static void runREPL()
{
    printBanner();
    std::cout << Colors::GREEN << "  REPL Mode — type 'exit' or Ctrl+D to quit\n"
              << Colors::RESET << "\n";

    Interpreter interp;
    std::string line;
    int lineNum = 1;

    while (true)
    {
        std::cout << Colors::CYAN << "quantum[" << lineNum++ << "]> " << Colors::RESET;
        if (!std::getline(std::cin, line))
            break;
        if (line == "exit" || line == "quit")
            break;
        if (line.empty())
            continue;

        try
        {
            Lexer lexer(line);
            auto tokens = lexer.tokenize();
            Parser parser(std::move(tokens));
            auto ast = parser.parse();
            interp.execute(*ast);
        }
        catch (const ParseError &e)
        {
            std::cerr << Colors::RED << "[ParseError] " << Colors::RESET << e.what() << " (line " << e.line << ")\n";
        }
        catch (const QuantumError &e)
        {
            std::cerr << Colors::RED << "[" << e.kind << "] " << Colors::RESET << e.what();
            if (e.line > 0)
                std::cerr << " (line " << e.line << ")";
            std::cerr << "\n";
        }
        catch (const std::exception &e)
        {
            std::cerr << Colors::RED << "[Error] " << Colors::RESET << e.what() << "\n";
        }
    }

    std::cout << Colors::YELLOW << "\n  Goodbye! 👋\n"
              << Colors::RESET;
}
```

**REPL Analysis:**
- **Line 87**: Creates a single interpreter instance for the entire session, maintaining state across commands
- **Line 88**: Stores user input line by line
- **Line 89**: Tracks line numbers for error reporting and prompt display
- **Lines 93-99**: Prompt display with line numbering and input handling
- **Lines 101-107**: Core compilation pipeline - Lexer → Parser → Interpreter
- **Lines 109-119**: Comprehensive exception handling for different error types

### File Execution Function (Lines 122-180)
```cpp
static void runFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << Colors::RED << "[Error] " << Colors::RESET
                  << "Cannot open file: " << path << "\n";
        std::exit(1);
    }

    // Check extension
    if (path.size() < 3 || path.substr(path.size() - 3) != ".sa")
    {
        std::cerr << Colors::YELLOW << "[Warning] " << Colors::RESET
                  << "File does not have .sa extension\n";
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    try
    {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(std::move(tokens));
        auto ast = parser.parse();

        Interpreter interp;
        interp.execute(*ast);
    }
    // ... Exception handling ...
}
```

**File Execution Analysis:**
- **Lines 124-130**: File existence validation with graceful error handling
- **Lines 133-137**: Extension validation (.sa files) with warning instead of error
- **Lines 139-141**: Efficient file reading using string stream buffer
- **Lines 145-152**: Same compilation pipeline as REPL but with complete file content
- **Lines 154-179**: Enhanced error reporting with file context and line numbers

### Test Mode Execution (Lines 182-210)
```cpp
static void runTestMode(const std::string &path)
{
    g_testMode = true;  // Enable test mode
    
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << Colors::RED << "[Error] " << Colors::RESET
                  << "Cannot open test file: " << path << "\n";
        std::exit(1);
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    try
    {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(std::move(tokens));
        auto ast = parser.parse();

        Interpreter interp;
        interp.execute(*ast);
        
        std::cout << Colors::GREEN << "Test completed successfully\n" << Colors::RESET;
    }
    catch (const std::exception &e)
    {
        std::cout << Colors::RED << "Test failed: " << e.what() << "\n" << Colors::RESET;
        std::exit(1);
    }
}
```

**Test Mode Features:**
- **Non-blocking Input**: `g_testMode` flag makes `input()` return empty string
- **Automated Testing**: Designed for CI/CD and automated test suites
- **Clear Output**: Success/failure indication with color coding

### Help Display Function (Lines 212-240)
```cpp
static void printHelp(const char *prog)
{
    std::cout << Colors::BOLD << "Usage:\n"
              << Colors::RESET
              << "  " << prog << " <file.sa>          Run a Quantum script\n"
              << "  " << prog << "                     Start interactive REPL\n"
              << "  " << prog << " --help              Show this help\n"
              << "  " << prog << " --version           Show version info\n"
              << "  " << prog << " --test <file.sa>     Run in test mode (non-blocking input)\n"
              << "  " << prog << " --aura              Show achievement display\n\n"
              << Colors::BOLD << "File extension:\n"
              << Colors::RESET
              << "  Quantum scripts use the .sa extension\n\n"
              << Colors::BOLD << "Examples:\n"
              << Colors::RESET
              << "  quantum hello.sa\n"
              << "  quantum --test script.sa\n"
              << "  quantum script.sa\n";
}
```

### Main Function (Lines 242-320)
```cpp
int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    if (argc == 1)
    {
        runREPL();
        return 0;
    }

    std::string arg = argv[1];

    if (arg == "--help" || arg == "-h")
    {
        printBanner();
        printHelp(argv[0]);
        return 0;
    }

    if (arg == "--aura")
    {
        printBanner();
        printAura();
        return 0;
    }

    if (arg == "--version" || arg == "-v")
    {
        std::cout << "Quantum Language v1.0.0\n"
                  << "Runtime: Tree-walk interpreter\n"
                  << "Built By Muhammad Saad Amin\n";
        return 0;
    }

    if (arg == "--test" && argc >= 3)
    {
        runTestMode(argv[2]);
        return 0;
    }
    runFile(arg);
    return 0;
}
```

**Main Function Analysis:**
- **Lines 244-247**: Windows UTF-8 console setup for proper Unicode display
- **Lines 249-254**: Default behavior - start REPL when no arguments provided
- **Lines 256-262**: Help flag handling with both short and long forms
- **Lines 264-269**: Special `--aura` flag for achievement display
- **Lines 271-278**: Version information display
- **Lines 280-285**: Test mode with argument validation
- **Line 287**: Default case - treat argument as filename to execute

## New Features in Current Version

### Test Mode Support
- **`--test` Flag**: Enables non-blocking input for automated testing
- **Global Test Flag**: `g_testMode` affects interpreter behavior
- **CI/CD Ready**: Designed for continuous integration pipelines

### Enhanced File Handling
- **Filesystem Library**: Uses `std::filesystem` for modern file operations
- **Better Error Messages**: More informative file-related error reporting
- **Extension Validation**: Checks for `.sa` extension with warnings

### Improved CLI
- **Extended Help**: Updated help text with new options
- **Better Organization**: Cleaner command-line argument handling
- **Professional Output**: Enhanced visual presentation

## Design Patterns and Architecture

### Command Pattern
The main function implements a command pattern where different arguments trigger different execution modes:
- No arguments → REPL mode
- `--help` → Help display
- `--version` → Version info
- `--test` → Test mode execution
- Filename → File execution

### Error Handling Strategy
- **Layered exception handling**: ParseError, QuantumError, and std::exception
- **User-friendly error messages**: Colored output with context information
- **Graceful degradation**: Warnings for non-critical issues (wrong file extension)

### Resource Management
- **RAII**: File streams automatically closed when leaving scope
- **Smart pointers**: Used throughout the interpreter components
- **Efficient I/O**: String stream buffering for file reading

## Why This Design Works

### Separation of Concerns
- **CLI handling** separated from **core interpreter logic**
- **Error display** separated from **error generation**
- **File I/O** separated from **language processing**

### User Experience Focus
- **Informative prompts** with line numbers in REPL
- **Colored output** for better readability
- **Comprehensive help** system with multiple options
- **Test mode** for automated workflows

### Robustness
- **Comprehensive error handling** for all failure modes
- **Input validation** for file existence and format
- **Graceful EOF handling** in REPL
- **Cross-platform considerations** with Windows-specific UTF-8 setup

This main.cpp design provides a professional, user-friendly interface while maintaining clean separation between the CLI layer and the core language implementation, with enhanced features for testing and automation.
