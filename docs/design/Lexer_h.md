# Lexer.h - Lexer Class Interface and Method Declarations

## Overview
`Lexer.h` defines the interface for the lexical analysis phase of the Quantum Language compiler. It declares the `Lexer` class responsible for converting source code into tokens, along with all the supporting methods for character processing, token recognition, and complex language features like template literals.

## Architecture Overview

The lexer interface provides:
- **Token Stream Generation**: Main entry point for tokenization
- **Character Processing**: Low-level character access and manipulation
- **Token Recognition**: Specialized methods for different token types
- **Complex Feature Support**: Template literals, comments, whitespace handling
- **Error Reporting**: Position tracking for accurate error messages

## Line-by-Line Analysis

### Header Guard and Dependencies (Lines 1-5)
```cpp
#pragma once
#include "Token.h"
#include <string>
#include <vector>
#include <unordered_map>
```

**Dependency Analysis:**
- **`Token.h`**: Token type definitions and Token structure
- **`string`**: Source code storage and token value handling
- **`vector`**: Token stream storage and return type
- **`unordered_map`**: Keyword lookup table for efficient keyword detection

### Lexer Class Declaration (Lines 7-34)

#### Class Definition (Line 7)
```cpp
class Lexer
{
```

**Class Design:**
- **Single Responsibility**: Focused solely on lexical analysis
- **Stateful Design**: Maintains position and state during tokenization
- **Encapsulation**: Private members hide implementation details

#### Public Interface (Lines 9-11)
```cpp
public:
    explicit Lexer(const std::string &source);
    std::vector<Token> tokenize();
```

**Public Methods Analysis:**

**Constructor (Line 10):**
```cpp
explicit Lexer(const std::string &source);
```
- **`explicit`**: Prevents implicit conversion from string to Lexer
- **Parameter**: Takes source code by const reference (efficient)
- **Purpose**: Initializes lexer with source code to tokenize

**Tokenize Method (Line 11):**
```cpp
std::vector<Token> tokenize();
```
- **Return Type**: Vector of tokens representing the entire source
- **Method**: Main entry point for lexical analysis
- **Ownership**: Returns token vector by value (move semantics optimize)

#### Private Members (Lines 13-34)

**State Variables (Lines 14-16):**
```cpp
private:
    std::string src;
    size_t pos;
    int line, col;
```

**State Analysis:**
- **`src`**: Complete source code string (const copy)
- **`pos`**: Current character position in source (0-based index)
- **`line`**: Current line number (1-based for user-friendly errors)
- **`col`**: Current column number (1-based for user-friendly errors)

**Position Tracking Benefits:**
- **Error Reporting**: Precise location for syntax errors
- **Debugging**: Exact positions for token identification
- **User Experience**: Familiar 1-based line/column numbering

**Static Keyword Table (Line 18):**
```cpp
    static const std::unordered_map<std::string, TokenType> keywords;
```

**Keyword Table Features:**
- **Static**: Single instance shared across all lexer objects
- **Const**: Immutable after initialization
- **Unordered_map**: O(1) average lookup for keyword detection
- **String to TokenType**: Maps identifier strings to token types

**Template Literal Support (Line 20):**
```cpp
    std::vector<Token> pendingTokens_; // used for f-string expansion
```

**Template Literal Features:**
- **Pending Tokens**: Stores tokens generated during template literal processing
- **F-string Expansion**: Supports complex template literal expressions
- **Multi-segment Output**: Template literals generate multiple tokens

**Character Processing Methods (Lines 22-24):**
```cpp
    char current() const;
    char peek(int offset = 1) const;
    char advance();
```

**Method Analysis:**

**`current()` (Line 22):**
```cpp
char current() const;
```
- **Purpose**: Returns current character without advancing position
- **Const**: Doesn't modify lexer state
- **Bounds Safety**: Returns null terminator at end of source

**`peek()` (Line 23):**
```cpp
char peek(int offset = 1) const;
```
- **Purpose**: Lookahead character access without consuming
- **Default Offset**: Peeks 1 character ahead by default
- **Multi-character Lookahead**: Supports arbitrary offset distances
- **Token Disambiguation**: Essential for distinguishing multi-character operators

**`advance()` (Line 24):**
```cpp
char advance();
```
- **Purpose**: Consumes current character and advances position
- **Return Value**: Returns consumed character for convenience
- **Position Management**: Updates line, column, and position counters
- **Newline Handling**: Special logic for line/column reset on newlines

**Whitespace and Comment Handling (Lines 25-27):**
```cpp
    void skipWhitespace();
    void skipComment();      // single-line: // ...
    void skipBlockComment(); // multi-line:  /* ... */
```

**Whitespace and Comment Analysis:**

**`skipWhitespace()` (Line 25):**
```cpp
void skipWhitespace();
```
- **Purpose**: Skips all whitespace characters (spaces, tabs, newlines)
- **Newline Tracking**: Updates line/column counters for newlines
- **Token Separation**: Whitespace serves as token separator

**`skipComment()` (Line 26):**
```cpp
void skipComment();      // single-line: // ...
```
- **Purpose**: Skips single-line comments starting with `//`
- **Line Ending**: Stops at end of line
- **Comment Content**: Entire comment line is ignored

**`skipBlockComment()` (Line 27):**
```cpp
void skipBlockComment(); // multi-line:  /* ... */
```
- **Purpose**: Skips multi-line comments between `/*` and `*/`
- **Nested Support**: Handles nested block comments (implementation-dependent)
- **Multi-line**: Spans multiple lines with proper line counting

**Token Reading Methods (Lines 29-33):**
```cpp
    Token readNumber();
    Token readString(char quote);
    void readTemplateLiteral(std::vector<Token> &out, int startLine, int startCol);
    Token readIdentifierOrKeyword();
    Token readOperator();
```

**Token Reading Analysis:**

**`readNumber()` (Line 29):**
```cpp
Token readNumber();
```
- **Purpose**: Reads numeric literals (integers, floats, scientific notation)
- **Number Formats**: Supports `123`, `123.456`, `1.23e-4`, etc.
- **Return**: Token with NUMBER type and string representation
- **Validation**: Ensures proper number format

**`readString()` (Line 30):**
```cpp
Token readString(char quote);
```
- **Purpose**: Reads string literals enclosed in quotes
- **Quote Types**: Supports both single (`'`) and double (`"`) quotes
- **Escape Sequences**: Handles `\n`, `\t`, `\\`, `\"`, etc.
- **Unicode Support**: Processes `\uXXXX` escape sequences
- **Parameter**: Quote character to match opening/closing quotes

**`readTemplateLiteral()` (Line 31):**
```cpp
void readTemplateLiteral(std::vector<Token> &out, int startLine, int startCol);
```
- **Purpose**: Processes template literals with interpolation
- **Complex Output**: Generates multiple tokens for text and expressions
- **Interpolation**: Handles `${expression}` syntax
- **Parameters**: 
  - `out`: Vector to store generated tokens
  - `startLine/startCol`: Position information for error reporting
- **Void Return**: Results stored in provided vector

**`readIdentifierOrKeyword()` (Line 32):**
```cpp
Token readIdentifierOrKeyword();
```
- **Purpose**: Reads identifiers and distinguishes keywords
- **Keyword Lookup**: Uses static keyword table for detection
- **Identifier Rules**: Supports letters, digits, underscores (cannot start with digit)
- **Return**: IDENTIFIER token or specific keyword token type

**`readOperator()` (Line 33):**
```cpp
Token readOperator();
```
- **Purpose**: Reads operators and punctuation
- **Multi-character Support**: Handles `==`, `===`, `+=`, `->`, etc.
- **Single Character**: Also handles single-character operators
- **Priority**: Longer operators matched before shorter ones
- **Return**: Appropriate operator token type

## Design Patterns and Architecture

### State Machine Pattern
```cpp
class Lexer {
    std::string src;
    size_t pos;
    int line, col;
    // State machine methods...
};
```

**Benefits:**
- **Clear State**: Current position and context always known
- **Deterministic**: Same input always produces same output
- **Maintainable**: Easy to understand and modify state transitions

### Strategy Pattern
```cpp
Token readNumber();      // Number strategy
Token readString(char);  // String strategy
Token readOperator();    // Operator strategy
```

**Benefits:**
- **Specialized Handling**: Each token type has optimal reading strategy
- **Extensible**: Easy to add new token types
- **Maintainable**: Each strategy isolated from others

### Factory Pattern (Implied)
```cpp
std::vector<Token> tokenize(); // Factory method for token stream
```

**Benefits:**
- **Encapsulated Creation**: Token creation logic hidden
- **Consistent Interface**: Single method for complete tokenization
- **Error Handling**: Centralized error handling for tokenization

## Integration with Compiler Pipeline

### Token Stream Generation
```cpp
// In main.cpp
Lexer lexer(source);
auto tokens = lexer.tokenize();
Parser parser(std::move(tokens));
```

**Integration Features:**
- **Source Input**: Takes source code as input
- **Token Output**: Produces token stream for parser
- **Error Propagation**: Position information flows to parser
- **Memory Efficiency**: Move semantics avoid copying

### Error Reporting Support
```cpp
// Position tracking enables precise error messages
throw LexicalError("Unexpected character", line, col);
```

**Error Features:**
- **Precise Location**: Line and column for all errors
- **Context Information**: Rich error context for debugging
- **User Friendly**: Familiar error message format

## Performance Considerations

### Efficient Character Access
```cpp
char current() const { return pos < src.size() ? src[pos] : '\0'; }
```

**Optimizations:**
- **Direct Array Access**: O(1) character access
- **Bounds Checking**: Minimal overhead with ternary operator
- **Inline Potential**: Simple function can be inlined

### Hash Table Lookup
```cpp
static const std::unordered_map<std::string, TokenType> keywords;
```

**Performance Features:**
- **O(1) Average Lookup**: Constant-time keyword detection
- **Static Storage**: Single instance shared across all lexers
- **Perfect Hash Potential**: Could be optimized for fixed keyword set

### Memory Management
```cpp
std::vector<Token> tokenize();
```

**Memory Features:**
- **Pre-allocation**: Vector can reserve capacity for known token count
- **Move Semantics**: Efficient return of token vector
- **Contiguous Storage**: Tokens stored in contiguous memory

## Extensibility Design

### New Token Types
```cpp
// Easy to add new token types to TokenType enum
enum class TokenType {
    // Existing tokens...
    NEW_TOKEN_TYPE,  // Easy to add
};
```

**Extensibility Features:**
- **Enum Extension**: New token types easily added
- **Strategy Addition**: New read methods for new token types
- **Keyword Table**: New keywords easily added to static map

### New Language Features
```cpp
// Template literal support shows extensibility
void readTemplateLiteral(std::vector<Token> &out, int startLine, int startCol);
```

**Feature Addition Benefits:**
- **Complex Features**: Template literals show ability to handle complex syntax
- **Multi-token Output**: Can generate multiple tokens from single construct
- **Position Preservation**: Maintains accurate position information

## Error Handling Strategy

### Position Tracking
```cpp
int line, col;  // Always available for error reporting
```

**Error Features:**
- **Precise Location**: Every token knows its origin
- **Context Rich**: Line and column provide full context
- **User Friendly**: Familiar error message format

### Graceful Recovery
```cpp
// Lexer attempts to continue after errors
Token readOperator() {
    // Try to read operator, return UNKNOWN if fails
    return Token(TokenType::UNKNOWN, "", line, col);
}
```

**Recovery Features:**
- **Error Tokens**: UNKNOWN token represents unrecognized input
- **Continuation**: Lexer can continue after minor errors
- **Parser Handling**: Parser can handle unknown tokens gracefully

## Why This Design Works

### Comprehensive Language Support
- **All Token Types**: Covers every language construct
- **Complex Features**: Template literals, comments, whitespace
- **Multi-language**: Supports syntax from multiple languages
- **Future-proof**: Easy to add new features

### Performance and Efficiency
- **Single Pass**: Processes source in one linear scan
- **Efficient Lookup**: Hash table for keyword detection
- **Memory Management**: Smart use of containers and move semantics
- **Cache Friendly**: Sequential memory access patterns

### Maintainability and Clarity
- **Clear Interface**: Well-defined public API
- **Encapsulated Implementation**: Private details hidden
- **Modular Design**: Separate methods for different token types
- **Extensible**: Easy to add new features and token types

The lexer interface provides a solid foundation for lexical analysis, balancing comprehensive language support with efficient implementation and clean design principles. It serves as the critical first step in the Quantum Language compilation pipeline.
