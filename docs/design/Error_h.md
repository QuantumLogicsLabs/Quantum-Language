# Error.h - Exception Hierarchy and Console Color Definitions

## Overview
`Error.h` defines the complete exception hierarchy for the Quantum Language compiler and interpreter. It provides specialized exception types for different error categories and includes console color definitions for enhanced user experience in error reporting and output formatting.

## Architecture Overview

The error system provides:
- **Exception Hierarchy**: Specialized exception types for different error categories
- **Position Information**: Line and column tracking for precise error location
- **Console Colors**: ANSI color codes for enhanced terminal output
- **User Experience**: Clear, informative error messages with visual formatting

## Line-by-Line Analysis

### Header Guard and Dependencies (Lines 1-3)
```cpp
#pragma once
#include <stdexcept>
#include <string>
```

**Dependency Analysis:**
- **`stdexcept`**: Base class for standard exception hierarchy
- **`string`**: Error message storage and manipulation

### Base Exception Class (Lines 5-13)
```cpp
class QuantumError : public std::runtime_error
{
public:
    int line;
    std::string kind;

    QuantumError(const std::string &kind, const std::string &msg, int line = -1)
        : std::runtime_error(msg), line(line), kind(kind) {}
};
```

**QuantumError Analysis:**

**Inheritance Hierarchy:**
- **Base Class**: Extends `std::runtime_error` for standard exception handling
- **Custom Properties**: Adds language-specific error information
- **Standard Interface**: Works with standard exception handling mechanisms

**Member Variables:**
- **`line`**: Source line number where error occurred (-1 if unknown)
- **`kind`**: Error category/type string for user identification
- **Message**: Inherited from `std::runtime_error` - detailed error description

**Constructor Features:**
- **Parameterized Constructor**: Takes error kind, message, and optional line
- **Base Class Initialization**: Properly initializes `std::runtime_error`
- **Default Line**: -1 indicates unknown or irrelevant line number
- **Member Initialization**: Uses member initializer list for efficiency

**Design Benefits:**
- **Type Safety**: Specific exception type for Quantum Language errors
- **Rich Context**: Provides both category and detailed message
- **Position Tracking**: Line numbers help users locate errors
- **Standard Compliance**: Works with existing C++ exception handling

### Specialized Exception Classes (Lines 15-42)

#### RuntimeError (Lines 15-20)
```cpp
class RuntimeError : public QuantumError
{
public:
    RuntimeError(const std::string &msg, int line = -1)
        : QuantumError("RuntimeError", msg, line) {}
};
```

**RuntimeError Features:**
- **Purpose**: Errors that occur during program execution
- **Examples**: Division by zero, type errors, invalid operations
- **Inheritance**: Extends QuantumError with specific error kind
- **Constructor**: Simplified interface focusing on message and location

**Use Cases:**
```cpp
// Division by zero
if (denominator == 0.0) {
    throw RuntimeError("Division by zero", line);
}

// Type mismatch
if (!left.isNumber() || !right.isNumber()) {
    throw RuntimeError("Operands must be numbers", line);
}
```

#### TypeError (Lines 22-27)
```cpp
class TypeError : public QuantumError
{
public:
    TypeError(const std::string &msg, int line = -1)
        : QuantumError("TypeError", msg, line) {}
};
```

**TypeError Features:**
- **Purpose**: Type-related errors during execution
- **Examples**: Invalid type conversions, wrong argument types
- **Specific Category**: "TypeError" clearly indicates type issues
- **User Friendly**: Helps developers understand type-related problems

**Use Cases:**
```cpp
// Invalid function arguments
if (args.size() != expected) {
    throw TypeError("Function expects " + std::to_string(expected) + " arguments", line);
}

// Type mismatch in operations
if (!obj.isString() && !obj.isArray()) {
    throw TypeError("Object must be string or array", line);
}
```

#### NameError (Lines 29-34)
```cpp
class NameError : public QuantumError
{
public:
    NameError(const std::string &msg, int line = -1)
        : QuantumError("NameError", msg, line) {}
};
```

**NameError Features:**
- **Purpose**: Variable and function name resolution errors
- **Examples**: Undefined variables, missing functions
- **Scope Issues**: Problems with variable lookup and scoping
- **Clear Identification**: "NameError" immediately suggests naming issues

**Use Cases:**
```cpp
// Undefined variable
if (!env->has(name)) {
    throw NameError("Undefined variable: '" + name + "'", line);
}

// Missing function
if (!function_exists(name)) {
    throw NameError("Function '" + name + "' is not defined", line);
}
```

#### IndexError (Lines 36-41)
```cpp
class IndexError : public QuantumError
{
public:
    IndexError(const std::string &msg, int line = -1)
        : QuantumError("IndexError", msg, line) {}
};
```

**IndexError Features:**
- **Purpose**: Collection indexing and access errors
- **Examples**: Array out of bounds, invalid dictionary keys
- **Collection Operations**: Problems with data structure access
- **Specific Context**: "IndexError" indicates access-related issues

**Use Cases:**
```cpp
// Array bounds checking
if (index < 0 || index >= array.size()) {
    throw IndexError("Array index out of bounds", line);
}

// Dictionary key checking
if (!dict.contains(key)) {
    throw IndexError("Key not found in dictionary", line);
}
```

### Console Color Definitions (Lines 43-54)

#### Colors Namespace (Line 43)
```cpp
namespace Colors
{
```

**Namespace Features:**
- **Organization**: Groups all color definitions together
- **Prevention**: Avoids naming conflicts with other code
- **Clarity**: Clear indication that these are color definitions
- **Usage**: Colors::RED, Colors::GREEN, etc.

#### Color Constants (Lines 44-53)
```cpp
    inline const char *RED = "\033[31m";
    inline const char *YELLOW = "\033[33m";
    inline const char *WHITE = "\033[37m";
    inline const char *CYAN = "\033[36m";
    inline const char *GREEN = "\033[32m";
    inline const char *BLUE = "\033[34m";
    inline const char *BOLD = "\033[1m";
    inline const char *RESET = "\033[0m";
    inline const char *MAGENTA = "\033[35m";
```

**Color Analysis:**

**ANSI Escape Codes:**
- **Format**: `\033[<code>m` where `\033` is ESC character
- **Standard**: Follows ANSI color standard for terminal compatibility
- **Cross-platform**: Works on most modern terminals and consoles

**Color Categories:**
- **Error Colors**: RED for errors, YELLOW for warnings
- **Information Colors**: CYAN for prompts, GREEN for success
- **Text Colors**: WHITE for normal text, BLUE for emphasis
- **Formatting**: BOLD for emphasis, RESET to clear formatting
- **Special**: MAGENTA for special highlighting

**Inline Constants:**
- **`inline`**: Allows definition in header without multiple definition errors
- **`const char*`**: String literals for color codes
- **Compile-time**: Values known at compile time for optimization

**Usage Examples:**
```cpp
// Error message with colors
std::cerr << Colors::RED << Colors::BOLD 
          << "[Error] " << Colors::RESET 
          << message << std::endl;

// Success message
std::cout << Colors::GREEN << "Operation completed successfully!" 
          << Colors::RESET << std::endl;

// Prompt with color
std::cout << Colors::CYAN << "quantum> " << Colors::RESET;
```

## Design Patterns and Architecture

### Exception Hierarchy Pattern
```cpp
std::runtime_error
    └── QuantumError
        ├── RuntimeError
        ├── TypeError
        ├── NameError
        └── IndexError
```

**Benefits:**
- **Type Safety**: Specific exception types for different error categories
- **Catch Granularity**: Can catch specific error types or base class
- **Polymorphism**: Base class pointer can handle all error types
- **Extensibility**: Easy to add new error types

### RAII Pattern (Implied)
```cpp
class QuantumError : public std::runtime_error {
    // Automatic cleanup when exception is caught and handled
};
```

**Benefits:**
- **Resource Management**: Automatic cleanup of exception objects
- **Exception Safety**: No memory leaks from exception handling
- **Standard Compliance**: Follows C++ exception handling best practices

### Namespace Pattern
```cpp
namespace Colors {
    // Color definitions
}
```

**Benefits:**
- **Organization**: Groups related functionality
- **Name Collision Prevention**: Avoids conflicts with other code
- **Clarity**: Clear indication of purpose
- **Usage**: Colors::RED provides clear, readable code

## Integration with Compiler Pipeline

### Error Generation in Lexer
```cpp
// In Lexer.cpp
throw LexicalError("Unexpected character: '" + char + "'", line, col);
```

**Integration Features:**
- **Position Tracking**: Line/column information preserved
- **Error Categorization**: Appropriate exception type for lexer errors
- **Message Clarity**: Detailed error messages for user understanding

### Error Generation in Parser
```cpp
// In Parser.cpp
throw ParseError("Expected '(' after function name", line, col);
```

**Parser Error Features:**
- **Syntax Errors**: Clear indication of syntax problems
- **Expected vs Actual**: Messages show what was expected
- **Recovery**: Parser can continue after some errors

### Error Generation in Interpreter
```cpp
// In Interpreter.cpp
throw TypeError("Operands must be numbers", line);
throw NameError("Undefined variable: '" + name + "'", line);
```

**Runtime Error Features:**
- **Execution Context**: Errors include execution context
- **Type Information**: Clear indication of type-related problems
- **Variable Information**: Specific variable names in error messages

### Error Handling in Main
```cpp
// In main.cpp
try {
    // Execute program
} catch (const ParseError &e) {
    std::cerr << Colors::RED << "[ParseError] " << Colors::RESET 
              << e.what() << " (line " << e.line << ")" << std::endl;
} catch (const QuantumError &e) {
    std::cerr << Colors::RED << "[" << e.kind << "] " << Colors::RESET 
              << e.what();
    if (e.line > 0) std::cerr << " (line " << e.line << ")";
    std::cerr << std::endl;
}
```

**Error Display Features:**
- **Color Coding**: Different colors for different error types
- **Structured Format**: Consistent error message format
- **Context Information**: Line numbers and error categories
- **User Friendly**: Clear, readable error messages

## Performance Considerations

### Exception Overhead
```cpp
// Exception objects are relatively lightweight
class QuantumError : public std::runtime_error {
    int line;           // 4 bytes
    std::string kind;   // Dynamic allocation
};
```

**Performance Features:**
- **Lightweight Base**: Minimal additional overhead over std::runtime_error
- **String Allocation**: Only when exceptions are thrown (rare case)
- **No Overhead**: Zero cost when no exceptions are thrown
- **Optimized**: Modern compilers optimize exception handling

### Color Code Efficiency
```cpp
inline const char *RED = "\033[31m";
```

**Optimization Features:**
- **Compile-time Constants**: Color codes known at compile time
- **Inline Functions**: No function call overhead
- **String Literals**: Stored in read-only memory
- **Minimal Overhead**: Simple pointer dereferencing

## User Experience Design

### Color Psychology
- **RED**: Errors, warnings, important information
- **GREEN**: Success, positive feedback
- **YELLOW**: Warnings, caution
- **BLUE**: Information, emphasis
- **CYAN**: Prompts, interactive elements
- **MAGENTA**: Special highlighting

### Error Message Format
```
[ErrorType] Description (line N)
[TypeError] Operands must be numbers (line 15)
[NameError] Undefined variable: 'x' (line 8)
```

**Format Benefits:**
- **Consistent Structure**: Predictable format for easy parsing
- **Quick Scanning**: Error type immediately visible
- **Context**: Line numbers for quick location
- **Clarity**: Human-readable descriptions

## Extensibility Design

### Adding New Exception Types
```cpp
class ValueError : public QuantumError {
public:
    ValueError(const std::string &msg, int line = -1)
        : QuantumError("ValueError", msg, line) {}
};
```

**Extension Benefits:**
- **Consistent Pattern**: All exceptions follow same structure
- **Easy Integration**: New types work with existing error handling
- **Type Safety**: Compile-time checking of new exception types
- **Backward Compatibility**: Existing code continues to work

### Adding New Colors
```cpp
namespace Colors {
    inline const char *ORANGE = "\033[38;5;208m";  // 256-color support
    inline const char *GRAY = "\033[90m";
}
```

**Color Extension Benefits:**
- **Namespace Organization**: New colors fit into existing structure
- **Consistent Format**: Same pattern as existing colors
- **Advanced Support**: Can add 256-color or RGB color support
- **Backward Compatibility**: Existing colors unchanged

## Why This Design Works

### Comprehensive Error Coverage
- **All Error Categories**: Covers lexical, parsing, and runtime errors
- **Specific Types**: Different exception types for different problems
- **Rich Context**: Line numbers and detailed descriptions
- **User Friendly**: Clear, actionable error messages

### Enhanced User Experience
- **Color Coding**: Visual distinction for different message types
- **Consistent Format**: Predictable error message structure
- **Quick Identification**: Error types immediately visible
- **Professional Appearance**: Modern terminal output formatting

### Maintainability and Extensibility
- **Clean Hierarchy**: Well-organized exception class hierarchy
- **Standard Patterns**: Consistent design across all components
- **Easy Extension**: Simple to add new error types and colors
- **Cross-platform**: Works on all modern terminals and systems

The error system provides a robust foundation for error handling and user feedback in the Quantum Language compiler, combining comprehensive exception handling with enhanced visual presentation for an excellent developer experience.
