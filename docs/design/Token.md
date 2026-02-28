# Token.cpp - Token String Representation

## Overview
`Token.cpp` provides the string representation functionality for Token objects. This is a simple but crucial utility file that enables debugging, error reporting, and token inspection throughout the compiler pipeline.

## Line-by-Line Analysis

### Includes (Lines 1-2)
```cpp
#include "../include/Token.h"
#include <sstream>
```
- **Line 1**: Includes the Token class definition and TokenType enum
- **Line 2**: Includes string stream utilities for formatted string construction

### toString() Method (Lines 4-8)
```cpp
std::string Token::toString() const {
    std::ostringstream oss;
    oss << "[" << line << ":" << col << " " << value << "]";
    return oss.str();
}
```

**Detailed Analysis:**

#### Function Signature
```cpp
std::string Token::toString() const
```
- **Return type**: `std::string` - Returns a formatted string representation
- **Const method**: `const` - Does not modify the Token object, can be called on const Tokens
- **Class method**: `Token::` - Member function of the Token class

#### Implementation Details

**Line 5**: `std::ostringstream oss;`
- Creates a string stream object for efficient string building
- `ostringstream` is preferred over string concatenation for performance
- Automatically handles type conversions and formatting

**Line 6**: `oss << "[" << line << ":" << col << " " << value << "]";`
- **Format**: `[line:col value]` - Provides position and content information
- **`line`**: Source code line number where the token was found
- **`col`**: Column number within that line
- **`value`**: The actual token text (e.g., "let", "variable_name", "+")
- **Brackets**: Square brackets provide clear visual delimiters

**Example outputs:**
- `[1:1 let]` - The 'let' keyword at line 1, column 1
- `[5:10 myVariable]` - Identifier at line 5, column 10
- `[3:15 +]` - Plus operator at line 3, column 15

**Line 7**: `return oss.str();`
- Converts the string stream to a standard string
- `str()` method extracts the accumulated content

## Why This Simple Design is Effective

### Debugging Utility
The `toString()` method is primarily used for:
- **Compiler debugging**: Printing token streams during development
- **Error reporting**: Showing token context in parse errors
- **Testing**: Verifying tokenization results in unit tests
- **Logging**: Tracing compilation pipeline execution

### Information Density
The format `[line:col value]` provides three critical pieces of information:
1. **Location**: `line:col` - Exactly where in source code
2. **Content**: `value` - What the token actually is
3. **Delimiters**: `[]` - Clear visual boundaries

### Performance Considerations
- **String stream**: More efficient than repeated string concatenation
- **Single allocation**: `ostringstream` minimizes memory allocations
- **Inline-friendly**: Simple method can be inlined by the compiler

## Integration with Compiler Pipeline

### Lexer Integration
```cpp
// In Lexer.cpp, when creating tokens:
tokens.emplace_back(TokenType::IDENTIFIER, "variable", line, col);
// Later, for debugging:
std::cout << token.toString() << std::endl;
// Output: [5:10 variable]
```

### Error Reporting
```cpp
// In Parser.cpp, when reporting errors:
throw ParseError("Unexpected token: " + token.toString(), 
                  token.line, token.col);
```

### Testing Framework
```cpp
// In unit tests:
Lexer lexer("let x = 5");
auto tokens = lexer.tokenize();
ASSERT_EQ(tokens[0].toString(), "[1:1 let]");
ASSERT_EQ(tokens[1].toString(), "[1:5 x]");
```

## Design Patterns and Principles

### Utility Function Pattern
- **Single responsibility**: Only handles string representation
- **Stateless**: Doesn't modify the Token object
- **Reusable**: Can be called from anywhere in the codebase

### Information Hiding
- **Encapsulation**: Internal token structure is hidden
- **Interface**: Provides clean string representation without exposing internals
- **Abstraction**: Users don't need to know how tokens are stored

### Consistency
- **Standard format**: All tokens use the same `[line:col value]` format
- **Predictable output**: Easy to parse and understand
- **Tooling friendly**: Format can be easily parsed by external tools

## Potential Extensions

While the current implementation is simple and effective, potential enhancements could include:

### Enhanced Formatting
```cpp
// Possible future enhancement:
std::string Token::toDetailedString() const {
    std::ostringstream oss;
    oss << "Token{type=" << tokenTypeToString(type) 
        << ", value=\"" << value << "\""
        << ", pos=" << line << ":" << col << "}";
    return oss.str();
}
```

### Color Support
```cpp
// For terminal output with colors:
std::string Token::toColoredString() const {
    return Colors::CYAN + "[" + std::to_string(line) + ":" + 
           std::to_string(col) + Colors::RESET + " " + 
           Colors::YELLOW + value + Colors::RESET + "]";
}
```

### JSON Serialization
```cpp
// For machine-readable output:
std::string Token::toJson() const {
    return "{\"type\":\"" + tokenTypeToString(type) + 
           "\",\"value\":\"" + value + 
           "\",\"line\":" + std::to_string(line) + 
           ",\"col\":" + std::to_string(col) + "}";
}
```

## Conclusion

The `Token.cpp` file demonstrates the principle that simple, focused utilities are often the most effective. The `toString()` method provides exactly what's needed for debugging and error reporting without unnecessary complexity. Its design follows C++ best practices and integrates seamlessly with the broader compiler architecture.

The `[line:col value]` format has become a de facto standard in compiler implementations because it provides the perfect balance of information density and readability, making it an excellent choice for the Quantum Language compiler.
