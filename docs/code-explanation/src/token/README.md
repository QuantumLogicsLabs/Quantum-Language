# Token.cpp - Token String Representation Implementation

## Complete Source Code

```cpp
#include "../include/Token.h"
#include <sstream>

std::string Token::toString() const {
    std::ostringstream oss;
    oss << "[" << line << ":" << col << " " << value << "]";
    return oss.str();
}
```

## Purpose

The `Token.cpp` file implements the `toString()` method for the `Token` structure, providing a human-readable string representation of tokens that includes their position information and value. This implementation is essential for debugging, error reporting, and development tools that need to display token information in a clear, formatted manner.

## Block-by-Block Analysis

### **Include Statements (Lines 1-2)**
```cpp
#include "../include/Token.h"
#include <sstream>
```

**Header Inclusion Analysis:**
- **`#include "../include/Token.h"`**: Includes the Token header file containing the Token structure declaration and TokenType enumeration. The relative path indicates a source file structure where implementation files are in `src/` and headers in `include/`.
- **`#include <sstream>`**: Includes the string stream header for `std::ostringstream`, which provides efficient string building capabilities with stream operators.

**Purpose:**
- **Token Definition Access**: Needed to implement the `Token::toString()` method
- **String Building**: `ostringstream` provides type-safe string concatenation
- **Standard Library**: Uses proven, efficient string manipulation

### **Method Implementation (Lines 4-8)**
```cpp
std::string Token::toString() const {
    std::ostringstream oss;
    oss << "[" << line << ":" << col << " " << value << "]";
    return oss.str();
}
```

**Method Signature Analysis:**
- **`std::string`**: Returns a formatted string representation
- **`Token::toString()`**: Member function of the Token structure
- **`const`**: Method doesn't modify Token state (read-only)
- **No Parameters**: Uses the token's own member variables

**Implementation Breakdown:**

#### **String Stream Creation (Line 5)**
```cpp
std::ostringstream oss;
```
- **`std::ostringstream`**: Creates an output string stream
- **`oss`**: Local variable name for the stream
- **Efficient Building**: Stream operators provide type-safe concatenation
- **Memory Management**: Automatic buffer management

#### **Formatted Output (Line 6)**
```cpp
oss << "[" << line << ":" << col << " " << value << "]";
```

**Format Structure:**
- **Opening Bracket**: `[` marks the start of token representation
- **Line Number**: `line` member variable
- **Colon Separator**: `:` separates line and column
- **Column Number**: `col` member variable  
- **Space Separator**: ` ` separates position from value
- **Token Value**: `value` member variable (actual token text)
- **Closing Bracket**: `]` marks the end of token representation

**Example Output:**
```cpp
// For token at line 5, column 10 with value "print"
"[5:10 print]"

// For token at line 1, column 1 with value "42"
"[1:1 42]"

// For token at line 3, column 7 with value "function"
"[3:7 function]"
```

#### **String Return (Line 7)**
```cpp
return oss.str();
```
- **`oss.str()`**: Extracts the built string from the stream
- **Return Value**: Returns the formatted token string
- **Efficient Transfer**: Move semantics avoid unnecessary copying

## Design Patterns and Architecture

### **String Builder Pattern**
```cpp
std::ostringstream oss;
oss << "[" << line << ":" << col << " " << value << "]";
return oss.str();
```
- **Type Safety**: Stream operators ensure proper type conversion
- **Performance**: Efficient string building without multiple reallocations
- **Readability**: Clear, intuitive string construction
- **Standard Practice**: Uses well-established C++ pattern

### **Const Method Pattern**
```cpp
std::string Token::toString() const {
    // Implementation doesn't modify token state
}
```
- **Read-Only**: Guarantees no modification of token data
- **Thread Safety**: Safe to call on shared token objects
- **Compiler Optimization**: Enables certain optimizations
- **Clear Intent**: Method is for observation, not modification

### **Format Pattern**
```cpp
"[line:col value]"
```
- **Consistent Format**: Standardized representation across all tokens
- **Position Information**: Line and column for error context
- **Value Display**: Actual token text for identification
- **Delimited Structure**: Brackets clearly mark token boundaries

## Technical Considerations

### **Performance Characteristics**
```cpp
// Time complexity: O(n) where n = length of formatted string
// Space complexity: O(n) for the resulting string
```
- **Single Allocation**: `ostringstream` manages memory efficiently
- **Type Safety**: Stream operators handle type conversions
- **Minimal Overhead**: No unnecessary temporary strings
- **Cache Friendly**: Sequential memory access pattern

### **Memory Management**
```cpp
std::ostringstream oss;  // Automatic buffer management
return oss.str();       // String move semantics
```
- **RAII**: String stream manages its own memory
- **Move Semantics**: Return value optimization (RVO) or move
- **No Leaks**: Automatic cleanup on function exit
- **Exception Safe**: Strong exception guarantee

### **Integration Points**
```cpp
// Usage in error reporting
Token token = /* ... */;
std::cerr << "Error at " << token.toString() << std::endl;

// Usage in debugging
std::cout << "Token: " << token.toString() << std::endl;

// Usage in testing
assert(token.toString() == "[1:1 print]");
```
- **Error Context**: Provides location and content for errors
- **Debugging Support**: Clear token visualization
- **Testing Aid**: Predictable format for unit tests
- **Logging**: Structured token information for logs

## Usage Examples

### **Basic Usage**
```cpp
Token token(TokenType::IDENTIFIER, "variable", 5, 10);
std::string representation = token.toString();
// result: "[5:10 variable]"
```

### **Error Reporting Integration**
```cpp
void reportError(const Token &token, const std::string &message) {
    std::cerr << "Error at " << token.toString() 
              << ": " << message << std::endl;
}
```

### **Debugging Output**
```cpp
void debugTokens(const std::vector<Token> &tokens) {
    for (const auto &token : tokens) {
        std::cout << token.toString() << std::endl;
    }
}
```

### **Testing Support**
```cpp
void testTokenFormatting() {
    Token token(TokenType::NUMBER, "42", 1, 1);
    assert(token.toString() == "[1:1 42]");
}
```

## Extension Points

### **Enhanced Formatting**
```cpp
std::string Token::toStringVerbose() const {
    std::ostringstream oss;
    oss << "Token[" << typeToString(type) << "] ";
    oss << "[" << line << ":" << col << "] ";
    oss << "'" << value << "'";
    return oss.str();
}
```

### **JSON Output**
```cpp
std::string Token::toJSON() const {
    std::ostringstream oss;
    oss << "{\"type\":\"" << typeToString(type) << "\",";
    oss << "\"line\":" << line << ",";
    oss << "\"col\":" << col << ",";
    oss << "\"value\":\"" << value << "\"}";
    return oss.str();
}
```

### **Colored Output**
```cpp
std::string Token::toStringColored() const {
    std::ostringstream oss;
    oss << Colors::CYAN << "[" << line << ":" << col << Colors::RESET << " ";
    oss << Colors::YELLOW << value << Colors::RESET;
    oss << Colors::CYAN << "]" << Colors::RESET;
    return oss.str();
}
```

## Why This Design Works

### **Simplicity**
- **Clear Purpose**: Obviously formats token for display
- **Minimal Implementation**: Only essential code included
- **Self-Documenting**: Format is intuitive and readable
- **No Dependencies**: Uses only standard library

### **Effectiveness**
- **Complete Information**: Shows position and value
- **Consistent Format**: Same structure for all tokens
- **Debugging Friendly**: Easy to read and understand
- **Error Context**: Provides location for error reporting

### **Performance**
- **Efficient Building**: Single string stream operation
- **Memory Conscious**: No unnecessary allocations
- **Type Safe**: Stream operators prevent format errors
- **Standard Practice**: Uses proven C++ patterns

### **Maintainability**
- **Single Responsibility**: Only handles string formatting
- **Easy to Modify**: Simple to change format or add features
- **Testable**: Straightforward to unit test
- **Well-Documented**: Clear intent and behavior

The `Token.cpp` implementation demonstrates excellent utility function design, providing a simple, efficient, and reliable string representation method that serves the debugging, error reporting, and development needs of the Quantum Language lexer system.
