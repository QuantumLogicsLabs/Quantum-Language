# Lexer.h Design Document

## Overview

The `Lexer.h` header file defines the lexical analyzer (tokenizer) for the Quantum Language. It's responsible for breaking down source code into a stream of tokens that the parser can understand.

## Class Definition

```cpp
class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string src;
    size_t pos;
    int line, col;

    static const std::unordered_map<std::string, TokenType> keywords;

    char current() const;
    char peek(int offset = 1) const;
    char advance();
    void skipWhitespace();
    void skipComment();

    Token readNumber();
    Token readString(char quote);
    Token readIdentifierOrKeyword();
    Token readOperator();
};
```

## Core Components

### State Management
- `src`: The source code string to be tokenized
- `pos`: Current position in the source string
- `line`, `col`: Current line and column for error reporting

### Public Interface
- `Lexer(source)`: Constructor that takes source code
- `tokenize()`: Main method that returns a vector of tokens

### Private Helper Methods

#### Character Access
- `current()`: Returns current character
- `peek(offset)`: Looks ahead at future characters
- `advance()`: Moves to next character and updates line/col

#### Skipping Methods
- `skipWhitespace()`: Skips spaces and tabs
- `skipComment()`: Skips single-line comments starting with #

#### Token Reading Methods
- `readNumber()`: Parses numeric literals (decimal and hex)
- `readString()`: Parses string literals with escape sequences
- `readIdentifierOrKeyword()`: Distinguishes identifiers from keywords
- `readOperator()`: Parses multi-character operators

## Keyword Recognition

The lexer uses a static hash map for O(1) keyword lookup:

```cpp
static const std::unordered_map<std::string, TokenType> keywords = {
    {"let",      TokenType::LET},
    {"const",    TokenType::CONST},
    {"fn",       TokenType::FN},
    // ... more keywords
};
```

## Tokenization Process

1. **Initialization**: Set up source code and position tracking
2. **Main Loop**: Continue until end of source
   - Skip whitespace and comments
   - Identify token type based on current character
   - Call appropriate token reading method
   - Add token to result vector
3. **EOF Token**: Add EOF_TOKEN to mark end of input

## Error Handling

The lexer throws `QuantumError` exceptions for:
- Unterminated string literals
- Unexpected characters
- Invalid numeric formats

## Performance Considerations

- **Single Pass**: Processes source in O(n) time
- **Hash Map Lookup**: O(1) keyword identification
- **Minimal Memory**: Only stores tokens, not intermediate structures

## Integration Points

- **Token.h**: Defines TokenType enum and Token struct
- **Error.h**: Provides exception types for error handling
- **Parser.cpp**: Consumes the token stream

## Example Usage

```cpp
std::string source = "let x = 42";
Lexer lexer(source);
auto tokens = lexer.tokenize();
// Results in: [LET, IDENTIFIER("x"), ASSIGN, NUMBER(42), EOF_TOKEN]
```

## Future Enhancements

- Support for multi-line comments
- Better Unicode handling
- Token position tracking for improved error messages
- Preprocessor directives
