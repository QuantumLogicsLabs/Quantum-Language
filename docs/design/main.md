# main.cpp - Entry Point and CLI Interface

## Overview
`main.cpp` serves as the entry point for the Quantum Language interpreter. It handles command-line argument parsing, provides both REPL (Read-Eval-Print Loop) and file execution modes, and includes comprehensive error handling and user interface elements.

## Line-by-Line Analysis

### Includes and Dependencies (Lines 1-11)
```cpp
#include "../include/Lexer.h"
#include "../include/Parser.h"
#include "../include/Interpreter.h"
#include "../include/Error.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif
```
- **Lines 1-4**: Core compiler components - Lexer (tokenization), Parser (AST building), Interpreter (execution), and Error handling
- **Lines 5-8**: Standard C++ libraries for I/O operations, file handling, and string manipulation
- **Lines 9-11**: Windows-specific header for console UTF-8 support, ensuring proper character encoding display

### Banner Display Function (Lines 13-26)
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

### Achievement Display Function (Lines 28-63)
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

### REPL Implementation (Lines 65-112)
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
- **Line 71**: Creates a single interpreter instance for the entire session, maintaining state across commands
- **Line 72**: Stores user input line by line
- **Line 73**: Tracks line numbers for error reporting and prompt display
- **Lines 77-83**: Prompt display with line numbering and input handling
- **Lines 85-91**: Core compilation pipeline - Lexer → Parser → Interpreter
- **Lines 93-107**: Comprehensive exception handling for different error types
- **Line 108**: Handles EOF (Ctrl+D) gracefully

### File Execution Function (Lines 114-168)
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
- **Lines 116-122**: File existence validation with graceful error handling
- **Lines 125-129**: Extension validation (.sa files) with warning instead of error
- **Lines 131-133**: Efficient file reading using string stream buffer
- **Lines 137-144**: Same compilation pipeline as REPL but with complete file content
- **Lines 146-167**: Enhanced error reporting with file context and line numbers

### Syntax Checking Function (Lines 170-200)
```cpp
static int checkFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << path << ":1:1: error: Cannot open file\n";
        return 1;
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
        (void)ast;
        return 0;
    }
    // ... Error handling ...
}
```

**Syntax Checker Analysis:**
- **Purpose**: Validates syntax without executing code (like `python -m py_compile`)
- **Line 187**: `(void)ast;` suppresses unused variable warning while maintaining AST creation
- **Return values**: 0 for success, 1 for any error (following Unix conventions)

### Help Display Function (Lines 202-217)
```cpp
static void printHelp(const char *prog)
{
    std::cout << Colors::BOLD << "Usage:\n"
              << Colors::RESET
              << "  " << prog << " <file.sa>          Run a Quantum script\n"
              << "  " << prog << "                     Start interactive REPL\n"
              << "  " << prog << " --help              Show this help\n"
              << "  " << prog << " --version           Show version info\n\n"
              << Colors::BOLD << "File extension:\n"
              << Colors::RESET
              << "  Quantum scripts use the .sa extension\n\n"
              << Colors::BOLD << "Examples:\n"
              << Colors::RESET
              << "  quantum hello.sa\n"
              << "  quantum script.sa\n";
}
```

### Main Function (Lines 219-262)
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

    if (arg == "--check" && argc >= 3)
    {
        return checkFile(argv[2]);
    }
    runFile(arg);
    return 0;
}
```

**Main Function Analysis:**
- **Lines 221-224**: Windows UTF-8 console setup for proper Unicode display
- **Lines 225-229**: Default behavior - start REPL when no arguments provided
- **Lines 233-238**: Help flag handling with both short and long forms
- **Lines 240-245**: Special `--aura` flag for achievement display
- **Lines 247-253**: Version information display
- **Lines 255-258**: Syntax checking mode with argument validation
- **Line 259**: Default case - treat argument as filename to execute

## Design Patterns and Architecture

### Command Pattern
The main function implements a command pattern where different arguments trigger different execution modes:
- No arguments → REPL mode
- `--help` → Help display
- `--version` → Version info
- `--check` → Syntax validation
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
- **Comprehensive help** system
- **Multiple execution modes** for different use cases

### Robustness
- **Comprehensive error handling** for all failure modes
- **Input validation** for file existence and format
- **Graceful EOF handling** in REPL
- **Cross-platform considerations** with Windows-specific UTF-8 setup

This main.cpp design provides a professional, user-friendly interface while maintaining clean separation between the CLI layer and the core language implementation.
