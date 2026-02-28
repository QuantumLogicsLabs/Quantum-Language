# Token.h - Token Type Definitions and Token Structure

## Overview
`Token.h` defines the complete token system for the Quantum Language lexer. It contains the `TokenType` enumeration that categorizes all possible tokens, and the `Token` structure that represents individual tokens with their lexical information.

## Architecture Overview

The token system provides:
- **Comprehensive Token Coverage**: All language constructs represented
- **Multi-language Support**: Aliases for Python, JavaScript, and C-style syntax
- **Position Information**: Line and column tracking for error reporting
- **Future-proofing**: Reserved keywords for planned features

## Line-by-Line Analysis

### Header Guard (Line 1)
```cpp
#pragma once
```
- **Modern Include Guard**: Prevents multiple inclusion errors
- **Compiler Optimization**: More efficient than traditional include guards
- **Standard Practice**: Widely supported in modern C++ compilers

### Dependencies (Line 2)
```cpp
#include <string>
```
- **String Support**: Needed for token value storage
- **Minimal Dependencies**: Only includes what's necessary

### TokenType Enumeration (Lines 4-123)

#### Literal Tokens (Lines 6-12)
```cpp
enum class TokenType
{
    // Literals
    NUMBER,
    STRING,
    TEMPLATE_STRING, // backtick template literal segment (text before ${)
    BOOL_TRUE,
    BOOL_FALSE,
    NIL,
```

**Literal Categories:**
- **`NUMBER`**: Numeric literals (integers and floating-point)
- **`STRING`**: String literals in single or double quotes
- **`TEMPLATE_STRING`**: Template literal segments between `${}` expressions
- **`BOOL_TRUE`/`BOOL_FALSE`**: Boolean literals
- **`NIL`**: Null/undefined value

#### Identifier and Keywords (Lines 14-64)
```cpp
    // Identifiers & Keywords
    IDENTIFIER,
    LET,
    CONST,
    FN,
    DEF,      // Python: def
    FUNCTION, // JavaScript: function
    CLASS,    // class keyword
    EXTENDS,  // extends / inherits
    NEW,      // new keyword
    THIS,     // this (JS alias for self)
    SUPER,    // super keyword
    RETURN,
    IF,
    ELSE,
    ELIF,
    WHILE,
    FOR,
    IN,
    OF, // JavaScript for...of
    BREAK,
    CONTINUE,
    RAISE,
    TRY,
    EXCEPT,
    FINALLY,
    AS,
    PRINT,
    INPUT,
    COUT, // cout
    CIN, // cin
    FROM,
    IMPORT,
```

**Keyword Categories:**
1. **Variable Declaration**: `LET`, `CONST`
2. **Function Declaration**: `FN`, `DEF`, `FUNCTION` (multi-language support)
3. **Object-Oriented**: `CLASS`, `EXTENDS`, `NEW`, `THIS`, `SUPER`
4. **Control Flow**: `IF`, `ELSE`, `ELIF`, `WHILE`, `FOR`, `BREAK`, `CONTINUE`
5. **Exception Handling**: `RAISE`, `TRY`, `EXCEPT`, `FINALLY`
6. **I/O Operations**: `PRINT`, `INPUT`, `COUT`, `CIN`
7. **Modules**: `IMPORT`, `FROM`, `AS`

#### Type Keywords (Lines 47-57)
```cpp
    // C/C++ style type keywords
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_VOID,
    TYPE_LONG,
    TYPE_SHORT,
    TYPE_UNSIGNED,
```

**Type Hint Features:**
- **C-style Compatibility**: Familiar type declarations for C/C++ programmers
- **Optional Typing**: Type hints are optional, language remains dynamically typed
- **Future Compilation**: Potential for static type checking in future versions

#### Cybersecurity Keywords (Lines 59-64)
```cpp
    // Cybersecurity reserved keywords (future)
    SCAN,
    PAYLOAD,
    ENCRYPT,
    DECRYPT,
    HASH,
```

**Security Features:**
- **Domain-Specific**: Keywords for cybersecurity operations
- **Future Implementation**: Reserved for planned security features
- **Language Identity**: Establishes Quantum Language as security-focused

#### Operator Tokens (Lines 66-101)
```cpp
    // Operators
    PLUS,
    MINUS,
    STAR,
    SLASH,
    FLOOR_DIV, // // integer division (Python)
    PERCENT,
    POWER,
    EQ,         // ==
    NEQ,        // !=
    STRICT_EQ,  // ===
    STRICT_NEQ, // !==
    LT,
    GT,
    LTE,
    GTE,
    AND,
    OR,
    NOT,
    ASSIGN,
    PLUS_ASSIGN,
    MINUS_ASSIGN,
    STAR_ASSIGN,
    SLASH_ASSIGN,
    FAT_ARROW, // =>
    PLUS_PLUS,
    MINUS_MINUS,
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    BIT_NOT,
    LSHIFT,
    RSHIFT,
    AND_AND, // &&
    OR_OR,   // ||
```

**Operator Categories:**
1. **Arithmetic**: `PLUS`, `MINUS`, `STAR`, `SLASH`, `FLOOR_DIV`, `PERCENT`, `POWER`
2. **Assignment**: `ASSIGN`, `PLUS_ASSIGN`, `MINUS_ASSIGN`, `STAR_ASSIGN`, `SLASH_ASSIGN`
3. **Comparison**: `EQ`, `NEQ`, `STRICT_EQ`, `STRICT_NEQ`, `LT`, `GT`, `LTE`, `GTE`
4. **Logical**: `AND`, `OR`, `NOT`, `AND_AND`, `OR_OR`
5. **Bitwise**: `BIT_AND`, `BIT_OR`, `BIT_XOR`, `BIT_NOT`, `LSHIFT`, `RSHIFT`
6. **Special**: `FAT_ARROW`, `PLUS_PLUS`, `MINUS_MINUS`

**Multi-language Support:**
- **Python-style**: `FLOOR_DIV` (`//`)
- **JavaScript-style**: `STRICT_EQ` (`===`), `STRICT_NEQ` (`!==`), `FAT_ARROW` (`=>`)
- **C-style**: Traditional operators with familiar precedence

#### Delimiter Tokens (Lines 102-116)
```cpp
    // Delimiters
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    COMMA,
    SEMICOLON,
    COLON,
    DOT,
    ARROW,
    QUESTION,
    DECORATOR,
    NEWLINE,
```

**Delimiter Functions:**
- **Grouping**: `LPAREN`/`RPAREN`, `LBRACE`/`RBRACE`, `LBRACKET`/`RBRACKET`
- **Separation**: `COMMA`, `SEMICOLON`, `NEWLINE`
- **Access**: `DOT`, `ARROW`
- **Special**: `QUESTION` (ternary), `DECORATOR` (Python-style decorators)

#### Special Tokens (Lines 118-123)
```cpp
    // Special
    EOF_TOKEN,
    UNKNOWN,
    INDENT, // Python-style indentation block start
    DEDENT, // Python-style indentation block end
};
```

**Special Token Features:**
- **EOF_TOKEN**: Marks end of input for parser
- **UNKNOWN**: Represents unrecognized characters
- **INDENT/DEDENT**: Future support for Python-style indentation-based blocks

### Token Structure (Lines 125-136)

#### Token Definition (Lines 125-133)
```cpp
struct Token
{
    TokenType type;
    std::string value;
    int line;
    int col;

    Token(TokenType t, std::string v, int ln, int c)
        : type(t), value(std::move(v)), line(ln), col(c) {}
```

**Token Components:**
- **`type`**: Categorizes the token (from TokenType enum)
- **`value`**: The actual text content of the token
- **`line`**: Source line number (1-based)
- **`col`**: Source column number (1-based)

**Constructor Features:**
- **Efficient Initialization**: Member initializer list
- **Move Semantics**: `std::move(v)` avoids string copying
- **Position Tracking**: Complete location information for error reporting

#### toString() Method (Lines 135-136)
```cpp
    std::string toString() const;
};
```

**Method Declaration:**
- **String Representation**: For debugging and error reporting
- **Const Method**: Doesn't modify token state
- **Implementation**: Defined in `Token.cpp`

## Design Patterns and Architecture

### Enumeration Pattern
```cpp
enum class TokenType {
    // Strongly typed enum
};
```

**Benefits:**
- **Type Safety**: Prevents implicit conversion to integers
- **Namespace Scope**: No naming conflicts with other enums
- **Explicit Conversion**: Requires explicit casting when needed

### Structure Pattern
```cpp
struct Token {
    // Aggregate type with public members
};
```

**Benefits:**
- **Simple Data Container**: No complex behavior needed
- **Direct Access**: Easy access to member variables
- **Default Construction**: Can be default constructed if needed

### Factory Pattern (Implied)
```cpp
Token(TokenType::IDENTIFIER, "variable", 1, 5);
```

**Benefits:**
- **Consistent Creation**: Uniform token creation pattern
- **Validation**: Constructor ensures all required fields provided
- **Efficiency**: Move semantics reduce copying overhead

## Integration with Lexer

### Token Creation
```cpp
// In Lexer.cpp
tokens.emplace_back(TokenType::NUMBER, "123", line, col);
```

**Integration Features:**
- **Direct Construction**: Lexer creates tokens directly
- **Position Tracking**: Lexer provides line/column information
- **Type Assignment**: Lexer determines appropriate token type

### Keyword Lookup
```cpp
// In Lexer.cpp
auto it = keywords.find(identifier);
if (it != keywords.end()) {
    return Token(it->second, identifier, line, col);
}
```

**Keyword Processing:**
- **Hash Table Lookup**: Efficient O(1) keyword detection
- **Type Mapping**: Maps identifier strings to TokenType values
- **Fallback**: Non-keywords become IDENTIFIER tokens

## Error Handling Support

### Position Information
```cpp
struct Token {
    int line;
    int col;
    // ...
};
```

**Error Reporting Benefits:**
- **Precise Location**: Exact position of lexical errors
- **Context**: Line and column for user-friendly error messages
- **Debugging**: Helps developers locate issues quickly

### Unknown Token Handling
```cpp
UNKNOWN, // Represents unrecognized characters
```

**Error Recovery:**
- **Graceful Degradation**: Can continue parsing after unknown tokens
- **Error Reporting**: Clear indication of unrecognized input
- **Debugging**: Helps identify lexer issues

## Performance Considerations

### Memory Layout
```cpp
struct Token {
    TokenType type;     // 4 bytes (enum)
    std::string value;  // Dynamic allocation
    int line;           // 4 bytes
    int col;            // 4 bytes
};
```

**Optimization Features:**
- **Compact Base**: Small fixed-size members
- **Dynamic String**: Only allocates needed memory for value
- **Cache Friendly**: Sequential access in token vectors

### Move Semantics
```cpp
Token(TokenType t, std::string v, int ln, int c)
    : type(t), value(std::move(v)), line(ln), col(c) {}
```

**Performance Benefits:**
- **Reduced Allocations**: Avoids string copying
- **Efficient Transfer**: Moves string contents instead of copying
- **RAII**: Automatic memory management

## Extensibility Design

### Future Token Types
```cpp
// Cybersecurity reserved keywords (future)
SCAN,
PAYLOAD,
ENCRYPT,
DECRYPT,
HASH,
```

**Extensibility Features:**
- **Reserved Keywords**: Future implementation planned
- **Backward Compatibility**: Adding new tokens doesn't break existing code
- **Language Evolution**: Supports growth of language features

### Multi-language Support
```cpp
FN,        // Quantum/JavaScript style
DEF,       // Python style
FUNCTION,  // JavaScript full word
```

**Design Benefits:**
- **Programmer Familiarity**: Developers can use familiar syntax
- **Migration Support**: Easy transition from other languages
- **Flexibility**: Multiple ways to express the same concept

## Why This Design Works

### Comprehensive Coverage
- **Complete Language Support**: All constructs represented as tokens
- **Multi-paradigm**: Supports procedural, functional, and OOP styles
- **Future-proof**: Reserved space for planned features

### Developer Experience
- **Clear Semantics**: Each token type has unambiguous meaning
- **Good Error Messages**: Position information enables helpful errors
- **Familiar Syntax**: Supports patterns from popular languages

### Performance and Maintainability
- **Efficient Storage**: Compact representation with dynamic strings
- **Fast Lookup**: Hash table for keyword detection
- **Type Safety**: Strong typing prevents common errors

The token system provides a solid foundation for the Quantum Language lexer, balancing comprehensive language support with efficient implementation and clear design principles.
