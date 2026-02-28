# Quantum Language Compiler Structure and Architecture

## Overview

This document explains the complete structure and architecture of the Quantum Language compiler, detailing how all components work together to transform source code into executable programs.

## Compilation Pipeline

```
Source Code (.sa files)
        ↓
    Lexer (Token.cpp/h)
        ↓
    Token Stream
        ↓
    Parser (Parser.cpp/h)
        ↓
  Abstract Syntax Tree (AST.h)
        ↓
  Interpreter (Interpreter.cpp/h)
        ↓
  Program Output/Results
```

## Component Architecture

### 1. Lexical Analysis Phase

**Files:** `Lexer.cpp`, `Lexer.h`, `Token.cpp`, `Token.h`

**Purpose:** Convert raw source code into a stream of tokens

**Key Responsibilities:**
- Character-by-character source processing
- Keyword vs identifier distinction
- Number, string, and operator recognition
- Template literal parsing with interpolation
- Position tracking for error reporting
- Comment and whitespace handling

**Design Highlights:**
- Single-pass O(n) tokenization
- Hash table O(1) keyword lookup
- Support for multi-language syntax (Python, JavaScript, C++)
- Future-proof cybersecurity keyword reservation

### 2. Parsing Phase

**Files:** `Parser.cpp`, `Parser.h`, `AST.h`

**Purpose:** Transform token stream into Abstract Syntax Tree

**Key Responsibilities:**
- Recursive descent parsing for statements
- Pratt-style parsing for expressions with operator precedence
- AST node construction with position information
- Error recovery and synchronization
- Support for complex language features (classes, lambdas, comprehensions)

**Design Highlights:**
- Hybrid parsing approach (recursive descent + Pratt)
- Comprehensive statement and expression coverage
- Type-safe variant-based AST nodes
- Extensible architecture for new language features

### 3. Interpretation Phase

**Files:** `Interpreter.cpp`, `Interpreter.h`, `Value.cpp`, `Value.h`

**Purpose:** Execute the AST to produce program results

**Key Responsibilities:**
- Tree-walk AST execution
- Dynamic type system with variant-based values
- Lexical scoping with environment chains
- Object-oriented programming support
- Native function registration and execution
- Comprehensive standard library

**Design Highlights:**
- Modern C++ variant-based value system
- Smart pointer memory management
- Exception-based control flow
- Extensive built-in function library
- Cybersecurity-focused function set

### 4. Error Handling System

**Files:** `Error.h`

**Purpose:** Provide comprehensive error reporting and user feedback

**Key Responsibilities:**
- Specialized exception hierarchy
- Position-aware error reporting
- Console color definitions for enhanced UX
- Consistent error message formatting

**Design Highlights:**
- Type-safe exception hierarchy
- Rich context information (line, column, error type)
- ANSI color support for terminal output
- Professional error message presentation

### 5. Entry Point and CLI

**Files:** `main.cpp`

**Purpose:** Provide command-line interface and program orchestration

**Key Responsibilities:**
- Command-line argument parsing
- REPL (Read-Eval-Print Loop) implementation
- File execution mode
- Error handling and reporting
- User interface elements (banners, help, version info)

**Design Highlights:**
- Multiple execution modes (REPL, file, syntax check)
- Comprehensive error handling
- User-friendly interface with colors
- Professional presentation

## Data Flow Architecture

### Token Stream
```cpp
// Lexer produces
std::vector<Token> tokens = {
    Token(TokenType::LET, "let", 1, 1),
    Token(TokenType::IDENTIFIER, "x", 1, 5),
    Token(TokenType::ASSIGN, "=", 1, 7),
    Token(TokenType::NUMBER, "42", 1, 9),
    Token(TokenType::EOF_TOKEN, "", 1, 11)
};
```

### AST Structure
```cpp
// Parser creates hierarchical tree
ASTNode (BlockStmt) {
    statements: [
        ASTNode (VarDecl) {
            isConst: false,
            name: "x",
            initializer: ASTNode (NumberLiteral) { value: 42.0 }
        }
    ]
}
```

### Value System
```cpp
// Interpreter uses variant-based values
QuantumValue value = QuantumValue(42.0);  // Number
QuantumValue str = QuantumValue("hello"); // String
QuantumValue arr = QuantumValue(std::make_shared<Array>()); // Array
```

## Memory Management Strategy

### Smart Pointer Usage
- **`std::unique_ptr`**: AST node ownership (parser → interpreter)
- **`std::shared_ptr`**: Environment chains, function closures, collections
- **Automatic Cleanup**: RAII ensures no memory leaks

### Object Lifetime
1. **Tokens**: Owned by lexer, moved to parser, discarded after parsing
2. **AST Nodes**: Owned by parser, transferred to interpreter, cleaned up after execution
3. **Environments**: Shared ownership for closures, automatic cleanup when references drop to zero
4. **Values**: Reference-counted for collections and functions

## Type System Architecture

### Dynamic Typing with Variant
```cpp
using Data = std::variant<
    QuantumNil, bool, double, std::string,
    std::shared_ptr<Array>, std::shared_ptr<Dict>,
    std::shared_ptr<QuantumFunction>, std::shared_ptr<QuantumNative>,
    std::shared_ptr<QuantumInstance>, std::shared_ptr<QuantumClass>
>;
```

**Benefits:**
- Type safety with compile-time checking
- Memory efficiency (only active type stored)
- Extensible for new types
- Pattern matching with `std::visit`

### Runtime Type Checking
- **Type Guards**: `isNumber()`, `isString()`, `isArray()`, etc.
- **Safe Access**: `asNumber()`, `asString()`, `asArray()` with validation
- **Error Reporting**: Clear type mismatch messages

## Object-Oriented Programming Support

### Class System
```cpp
struct QuantumClass {
    std::string name;
    std::shared_ptr<QuantumClass> base;  // Inheritance
    std::unordered_map<std::string, std::shared_ptr<QuantumFunction>> methods;
    std::unordered_map<std::string, std::shared_ptr<QuantumFunction>> staticMethods;
    std::unordered_map<std::string, QuantumValue> staticFields;
};
```

### Instance System
```cpp
struct QuantumInstance {
    std::shared_ptr<QuantumClass> klass;
    std::unordered_map<std::string, QuantumValue> fields;  // Instance state
    std::shared_ptr<Environment> env;                       // Method closure
};
```

**Features:**
- Single inheritance with method resolution
- Instance fields and class methods
- Static methods and fields
- Closure-based method execution

## Performance Optimizations

### Compiler Optimizations
1. **Single-pass Lexing**: O(n) tokenization with minimal backtracking
2. **Efficient Parsing**: Recursive descent with Pratt parsing for expressions
3. **Memory Efficiency**: Move semantics, smart pointers, variant storage
4. **Hash Table Lookup**: O(1) keyword and variable access

### Runtime Optimizations
1. **Environment Chain**: O(depth) variable lookup with caching opportunities
2. **Type Dispatch**: Compile-time variant dispatch for expressions
3. **Native Functions**: C++ implementations for performance-critical operations
4. **Memory Layout**: Cache-friendly data structures

## Error Handling Architecture

### Exception Hierarchy
```
std::runtime_error
    └── QuantumError
        ├── RuntimeError    (execution errors)
        ├── TypeError       (type mismatches)
        ├── NameError       (undefined variables)
        └── IndexError      (collection access)
```

### Error Reporting Strategy
1. **Position Tracking**: Line and column for all errors
2. **Context Information**: Rich error messages with expected vs actual
3. **Color Coding**: Visual distinction for different error types
4. **Recovery Mechanisms**: Attempt to continue after minor errors

## Standard Library Architecture

### Native Function Registration
```cpp
void registerNatives() {
    // Mathematical functions
    env->define("sin", QuantumValue(std::make_shared<QuantumNative>(...)));
    
    // String operations
    env->define("len", QuantumValue(std::make_shared<QuantumNative>(...)));
    
    // Cybersecurity functions
    env->define("hash", QuantumValue(std::make_shared<QuantumNative>(...)));
}
```

### Library Categories
1. **Mathematical**: Trigonometry, logarithms, basic operations
2. **String Operations**: Length, manipulation, searching
3. **Collection Methods**: Array and dictionary operations
4. **File I/O**: File system operations
5. **Time/Date**: Current time, formatting, sleeping
6. **Random Numbers**: Generation and seeding
7. **Cybersecurity**: Hashing, encryption, network scanning

## Extensibility Design

### Adding New Language Features
1. **Lexer**: Add new token types to `TokenType` enum
2. **Parser**: Add parsing methods for new constructs
3. **AST**: Add new node types to `NodeVariant`
4. **Interpreter**: Add execution methods for new nodes

### Adding New Native Functions
1. **Implementation**: Create C++ function with proper signature
2. **Registration**: Add to `registerNatives()` method
3. **Documentation**: Update function documentation
4. **Testing**: Add unit tests for new functionality

## Cross-Platform Considerations

### Windows Support
- **UTF-8 Console**: SetConsoleOutputCP(CP_UTF8) for proper Unicode
- **Line Endings**: Handle both `\n` and `\r\n`
- **Path Handling**: Windows path separator considerations

### Unix/Linux Support
- **ANSI Colors**: Full terminal color support
- **Signals**: Proper handling of SIGINT (Ctrl+C)
- **File System**: Unix-style path handling

## Security Considerations

### Input Validation
- **Source Code**: Validate input before processing
- **File Access**: Restrict file system access in sandboxed mode
- **Network Operations**: Control network access in cybersecurity functions

### Memory Safety
- **Smart Pointers**: Prevent memory leaks and dangling pointers
- **Bounds Checking**: Array and string access validation
- **Type Safety**: Runtime type checking prevents type confusion

## Testing Strategy

### Unit Testing
- **Lexer**: Tokenization accuracy for all token types
- **Parser**: AST generation for all language constructs
- **Interpreter**: Execution correctness for all operations
- **Value System**: Type operations and conversions

### Integration Testing
- **End-to-End**: Complete program execution
- **Error Handling**: Proper error reporting and recovery
- **Performance**: Benchmarking critical operations
- **Memory**: Leak detection and usage profiling

## Future Evolution

### Planned Enhancements
1. **JIT Compilation**: Just-in-time compilation for performance
2. **Static Analysis**: Optional type checking and linting
3. **Debugger**: Step-through debugging with breakpoints
4. **Package Manager**: Module distribution and dependency management
5. **IDE Integration**: Language server protocol support

### Architecture Scalability
- **Modular Design**: Clean separation enables independent component evolution
- **Interface Stability**: Well-defined APIs support backward compatibility
- **Performance Headroom**: Architecture supports significant performance improvements
- **Feature Extensibility**: Clean patterns support adding new language features

## Why This Architecture Works

### Comprehensive Design
- **Complete Language Support**: All modern language features implemented
- **Robust Error Handling**: Comprehensive error reporting and recovery
- **Performance Optimized**: Efficient algorithms and data structures
- **User Experience**: Professional CLI and helpful error messages

### Modern C++ Best Practices
- **Type Safety**: Variant-based system prevents runtime errors
- **Memory Management**: RAII and smart pointers prevent leaks
- **Performance**: Move semantics and efficient data structures
- **Maintainability**: Clean separation of concerns and modular design

### Domain-Specific Features
- **Cybersecurity Focus**: Specialized functions for security tasks
- **Multi-language Syntax**: Familiar syntax from multiple languages
- **Professional Tools**: Comprehensive CLI and development experience
- **Extensibility**: Architecture supports future growth and evolution

The Quantum Language compiler architecture demonstrates modern C++ best practices applied to language implementation, creating a robust, efficient, and user-friendly development tool specifically designed for cybersecurity applications.
