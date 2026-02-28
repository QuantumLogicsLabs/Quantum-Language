# Quantum Language Code Documentation

This directory contains detailed line-by-line explanations of all source and header files in the Quantum Language compiler/interpreter project.

## Project Overview

Quantum Language is a cybersecurity-ready scripting language built with C++17. It features:
- Tree-walk interpreter architecture
- Complete lexical analysis and parsing pipeline
- Dynamic type system with object-oriented programming support
- Cross-platform compatibility
- Extensive built-in library for cybersecurity operations

## File Structure

### Source Files (`src/`)
- [`main.cpp`](./main.md) - Entry point, CLI interface, and REPL implementation
- [`Token.cpp`](./Token.md) - Token string representation utilities
- [`Value.cpp`](./Value.md) - Value type system, environment, and class/instance management
- [`Lexer.cpp`](./Lexer.md) - Lexical analysis and tokenization
- [`Parser.cpp`](./Parser.md) - Recursive descent parser with Pratt-style expression parsing
- [`Interpreter.cpp`](./Interpreter.md) - Tree-walk interpreter with native function registration

### Header Files (`include/`)
- [`Token.h`](./Token_h.md) - Token type definitions and Token struct
- [`Value.h`](./Value_h.md) - Value system, environment, and OOP structures
- [`Lexer.h`](./Lexer_h.md) - Lexer class interface and method declarations
- [`Parser.h`](./Parser_h.md) - Parser class interface and parsing methods
- [`Interpreter.h`](./Interpreter_h.md) - Interpreter class interface and execution methods
- [`AST.h`](./AST_h.md) - Abstract Syntax Tree node definitions
- [`Error.h`](./Error_h.md) - Exception hierarchy and console color definitions

## Architecture Flow

```
Source Code → Lexer → Tokens → Parser → AST → Interpreter → Results
```

1. **Lexical Analysis**: Source code is tokenized into meaningful units
2. **Parsing**: Tokens are organized into an Abstract Syntax Tree
3. **Interpretation**: AST is traversed and executed using the tree-walk pattern

## Key Design Patterns

- **Visitor Pattern**: Used in the interpreter for AST traversal
- **Variant-based AST**: Modern C++ std::variant for type-safe AST nodes
- **Environment Chain**: Lexical scoping with parent-child environment relationships
- **Function Objects**: Native functions implemented as std::function objects
- **RAII**: Smart pointers used throughout for memory management

## Language Features Supported

- Variables (let/const) with type hints
- Functions and lambdas with closures
- Classes with inheritance and methods
- Arrays and dictionaries with comprehensions
- Control flow (if/elif/else, while, for, break/continue)
- Exception handling (try/except/finally)
- Template literals and string interpolation
- Import system
- Cybersecurity-specific built-in functions

## Compilation

The project uses CMake for cross-platform building:
```bash
mkdir build && cd build
cmake ..
make  # or MSBuild on Windows
```
