# Lexer.cpp - Lexical Analysis and Tokenization

## Overview
`Lexer.cpp` implements the lexical analysis phase of the Quantum Language compiler. It converts raw source code into a stream of tokens that the parser can understand. This is a critical component that handles keywords, identifiers, literals, operators, and complex language features like template literals.

## Architecture Overview

The lexer uses a single-pass approach with:
- **Character-by-character processing** for precise token boundaries
- **Keyword lookup table** for efficient identifier vs keyword distinction
- **State machine pattern** for handling complex literals (strings, numbers, templates)
- **Unicode support** for modern character encoding

## Line-by-Line Analysis

### Includes and Dependencies (Lines 1-5)
```cpp
#include "../include/Lexer.h"
#include "../include/Error.h"
#include <stdexcept>
#include <cctype>
#include <sstream>
```
- **Line 1**: Lexer class interface and Token definitions
- **Line 2**: Error handling for lexical errors
- **Lines 3-5**: Standard libraries for exceptions, character classification, and string operations

### Keyword Table (Lines 7-72)
```cpp
const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"let", TokenType::LET},
    {"const", TokenType::CONST},
    {"fn", TokenType::FN},
    {"def", TokenType::DEF},
    {"function", TokenType::FUNCTION},
    // ... continues with all keywords ...
    {"scan", TokenType::SCAN},
    {"payload", TokenType::PAYLOAD},
    {"encrypt", TokenType::ENCRYPT},
    {"decrypt", TokenType::DECRYPT},
    {"hash", TokenType::HASH},
};
```

**Keyword Categories:**
1. **Variable Declaration**: `let`, `const`
2. **Function Declaration**: `fn`, `def`, `function` (multi-language support)
3. **Control Flow**: `if`, `else`, `elif`, `while`, `for`, `break`, `continue`
4. **Object-Oriented**: `class`, `extends`, `new`, `this`, `self`, `super`
5. **Exception Handling**: `try`, `except`, `catch`, `finally`, `raise`, `throw`
6. **I/O Operations**: `print`, `printf`, `input`, `scanf`, `cout`, `cin`
7. **Modules**: `import`, `from`
8. **Literals**: `true`, `false`, `nil` (with language aliases)
9. **Logical Operators**: `and`, `or`, `not`
10. **Type Hints**: `int`, `float`, `double`, `char`, `string`, `bool`, `void`
11. **Cybersecurity**: `scan`, `payload`, `encrypt`, `decrypt`, `hash`

**Design Benefits:**
- **O(1) Lookup**: Hash table provides constant-time keyword detection
- **Multi-language Support**: Aliases for Python (`True`, `False`, `None`) and JavaScript (`null`, `undefined`)
- **Extensibility**: Easy to add new keywords for language evolution
- **Future-proofing**: Cybersecurity keywords reserved for future implementation

### Constructor (Lines 74-75)
```cpp
Lexer::Lexer(const std::string &source)
    : src(source), pos(0), line(1), col(1) {}
```
- **Source Storage**: Complete source code stored for processing
- **Position Tracking**: `pos` for character index, `line` and `col` for error reporting
- **Initialization**: Starts at beginning of file (line 1, column 1)

### Character Access Methods (Lines 77-99)

#### current() Method (Lines 77-80)
```cpp
char Lexer::current() const
{
    return pos < src.size() ? src[pos] : '\0';
}
```
- **Bounds Checking**: Returns null terminator at end of source
- **Const Method**: Doesn't modify lexer state
- **Efficiency**: Direct array access with bounds check

#### peek() Method (Lines 82-86)
```cpp
char Lexer::peek(int offset) const
{
    size_t p = pos + offset;
    return p < src.size() ? src[p] : '\0';
}
```
- **Lookahead Capability**: Peek ahead without consuming characters
- **Multi-character Lookahead**: Supports arbitrary offset distances
- **Token Disambiguation**: Essential for distinguishing `==` vs `=`, `>=` vs `>`, etc.

#### advance() Method (Lines 88-99)
```cpp
char Lexer::advance()
{
    char c = src[pos++];
    if (c == '\n')
    {
        line++;
        col = 1;
    }
    else
    {
        col++;
    }
    return c;
}
```
- **Position Management**: Updates all position tracking variables
- **Newline Handling**: Special logic for line and column reset
- **Return Value**: Returns consumed character for convenience

### Tokenization Entry Point (Lines 100-150)
```cpp
std::vector<Token> Lexer::tokenize()
{
    std::vector<Token> tokens;

    while (!atEnd())
    {
        skipWhitespace();
        skipComment();

        if (atEnd()) break;

        char c = current();
        int startLine = line, startCol = col;

        if (std::isdigit(c)) {
            tokens.push_back(readNumber());
        }
        else if (c == '"' || c == '\'') {
            tokens.push_back(readString(c));
        }
        else if (c == '`') {
            readTemplateLiteral(tokens, startLine, startCol);
        }
        else if (std::isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        }
        else if (std::ispunct(c)) {
            Token t = readOperator();
            if (t.type != TokenType::UNKNOWN)
                tokens.push_back(std::move(t));
        }
        else {
            throw LexicalError("Unexpected character: '" + std::string(1, c) + "'", line, col);
        }
    }

    tokens.emplace_back(TokenType::EOF_TOKEN, "", line, col);
    return tokens;
}
```

**Tokenization Flow:**
1. **Preprocessing**: Skip whitespace and comments
2. **Character Classification**: Determine token type from first character
3. **Token Reading**: Dispatch to appropriate reader method
4. **Error Handling**: Throw exceptions for unexpected characters
5. **EOF Token**: Add end-of-file marker for parser

### Number Reading (Lines 152-180)
```cpp
Token Lexer::readNumber()
{
    int startLine = line, startCol = col;
    std::string number;
    bool isFloat = false;

    while (std::isdigit(current()) || current() == '.') {
        if (current() == '.') {
            if (isFloat) break; // Second dot - stop number
            isFloat = true;
        }
        number += advance();
    }

    // Handle scientific notation
    if (current() == 'e' || current() == 'E') {
        number += advance();
        if (current() == '+' || current() == '-') {
            number += advance();
        }
        while (std::isdigit(current())) {
            number += advance();
        }
    }

    return Token(TokenType::NUMBER, number, startLine, startCol);
}
```

**Number Format Support:**
- **Integers**: `123`, `456`
- **Floats**: `123.456`, `.789`
- **Scientific Notation**: `1.23e-4`, `5.67E+8`
- **Error Prevention**: Multiple dots handled correctly

### String Reading (Lines 182-210)
```cpp
Token Lexer::readString(char quote)
{
    int startLine = line, startCol = col;
    std::string content;
    advance(); // Skip opening quote

    while (!atEnd() && current() != quote) {
        if (current() == '\\') {
            advance(); // Skip backslash
            char escaped = advance();
            switch (escaped) {
                case 'n': content += '\n'; break;
                case 't': content += '\t'; break;
                case 'r': content += '\r'; break;
                case '\\': content += '\\'; break;
                case '"': content += '"'; break;
                case '\'': content += '\''; break;
                case 'u': { // Unicode escape
                    std::string hex;
                    for (int i = 0; i < 4 && !atEnd(); ++i) {
                        hex += advance();
                    }
                    // Convert hex to Unicode character
                    char32_t code = std::stoul(hex, nullptr, 16);
                    content += encodeUTF8(code);
                    break;
                }
                default:
                    content += escaped; // Unknown escape, keep literal
            }
        } else {
            content += advance();
        }
    }

    if (atEnd()) {
        throw LexicalError("Unterminated string literal", startLine, startCol);
    }

    advance(); // Skip closing quote
    return Token(TokenType::STRING, content, startLine, startCol);
}
```

**String Features:**
- **Multiple Quote Types**: Single (`'`) and double (`"`) quotes
- **Escape Sequences**: `\n`, `\t`, `\r`, `\\`, `\"`, `\'`
- **Unicode Support**: `\uXXXX` escape sequences
- **Error Detection**: Unterminated string literals
- **Raw Content**: Stores unescaped content for interpreter

### Template Literal Reading (Lines 212-250)
```cpp
void Lexer::readTemplateLiteral(std::vector<Token> &out, int startLine, int startCol)
{
    advance(); // Skip opening backtick
    std::string currentSegment;

    while (!atEnd() && current() != '`') {
        if (current() == '$' && peek() == '{') {
            // End of text segment, start of expression
            if (!currentSegment.empty()) {
                out.emplace_back(TokenType::TEMPLATE_STRING, currentSegment, startLine, startCol);
                currentSegment.clear();
            }
            
            advance(); advance(); // Skip ${
            
            // Find matching closing brace (handling nested braces)
            int braceDepth = 1;
            std::string expression;
            while (!atEnd() && braceDepth > 0) {
                if (current() == '{') braceDepth++;
                else if (current() == '}') braceDepth--;
                
                if (braceDepth > 0) {
                    expression += advance();
                }
            }
            
            if (braceDepth > 0) {
                throw LexicalError("Unterminated template expression", startLine, startCol);
            }
            
            advance(); // Skip closing }
            
            // Tokenize the expression
            Lexer exprLexer(expression);
            exprLexer.line = line;  // Preserve line numbers
            exprLexer.col = col;
            auto exprTokens = exprLexer.tokenize();
            
            // Remove EOF token and add to output
            exprTokens.pop_back();
            out.insert(out.end(), exprTokens.begin(), exprTokens.end());
        } else if (current() == '\\') {
            // Handle escape sequences in template literals
            advance();
            char escaped = advance();
            switch (escaped) {
                case 'n': currentSegment += '\n'; break;
                case 't': currentSegment += '\t'; break;
                case '`': currentSegment += '`'; break;
                case '$': currentSegment += '$'; break;
                default: currentSegment += escaped; break;
            }
        } else {
            currentSegment += advance();
        }
    }
    
    // Add final text segment
    if (!currentSegment.empty()) {
        out.emplace_back(TokenType::TEMPLATE_STRING, currentSegment, startLine, startCol);
    }
    
    if (atEnd()) {
        throw LexicalError("Unterminated template literal", startLine, startCol);
    }
    
    advance(); // Skip closing backtick
}
```

**Template Literal Features:**
- **String Interpolation**: `` `Hello ${name}!` ``
- **Complex Expressions**: `` `Result: ${2 + 2}` ``
- **Nested Braces**: Handles `{ inner { nested } }`
- **Escape Sequences**: `\``, `\$` for literal characters
- **Multi-segment Output**: Generates alternating text and expression tokens

### Identifier and Keyword Reading (Lines 252-270)
```cpp
Token Lexer::readIdentifierOrKeyword()
{
    int startLine = line, startCol = col;
    std::string identifier;
    
    while (!atEnd() && (std::isalnum(current()) || current() == '_')) {
        identifier += advance();
    }
    
    // Check if it's a keyword
    auto it = keywords.find(identifier);
    if (it != keywords.end()) {
        return Token(it->second, identifier, startLine, startCol);
    }
    
    return Token(TokenType::IDENTIFIER, identifier, startLine, startCol);
}
```

**Identifier Rules:**
- **Valid Characters**: Letters, digits, underscore
- **Cannot start with digit**: Handled by character classification
- **Keyword Priority**: Keywords take precedence over identifiers
- **Case Sensitivity**: Exact matching for keywords

### Operator Reading (Lines 272-400)
```cpp
Token Lexer::readOperator()
{
    int startLine = line, startCol = col;
    char c = advance();
    
    switch (c) {
        case '+':
            if (match('=')) return Token(TokenType::PLUS_ASSIGN, "+=", startLine, startCol);
            if (match('+')) return Token(TokenType::PLUS_PLUS, "++", startLine, startCol);
            return Token(TokenType::PLUS, "+", startLine, startCol);
            
        case '-':
            if (match('=')) return Token(TokenType::MINUS_ASSIGN, "-=", startLine, startCol);
            if (match('-')) return Token(TokenType::MINUS_MINUS, "--", startLine, startCol);
            if (match('>')) return Token(TokenType::ARROW, "->", startLine, startCol);
            return Token(TokenType::MINUS, "-", startLine, startCol);
            
        case '=':
            if (match('=')) {
                if (match('=')) return Token(TokenType::STRICT_EQ, "===", startLine, startCol);
                return Token(TokenType::EQ, "==", startLine, startCol);
            }
            return Token(TokenType::ASSIGN, "=", startLine, startCol);
            
        // ... continues for all operators ...
    }
}
```

**Operator Categories:**
1. **Arithmetic**: `+`, `-`, `*`, `/`, `%`, `**`
2. **Assignment**: `=`, `+=`, `-=`, `*=`, `/=`
3. **Comparison**: `==`, `===`, `!=`, `!==`, `<`, `>`, `<=`, `>=`
4. **Logical**: `&&`, `||`, `!`
5. **Bitwise**: `&`, `|`, `^`, `~`, `<<`, `>>`
6. **Special**: `->`, `=>`, `?:`
7. **Delimiters`: `(`, `)`, `{`, `}`, `[`, `]`, `,`, `.`, `:`

### Helper Methods

#### skipWhitespace() (Lines 402-410)
```cpp
void Lexer::skipWhitespace()
{
    while (!atEnd() && std::isspace(current())) {
        if (current() == '\n') {
            // Handle Python-style indentation
            pendingTokens_.push_back(Token(TokenType::NEWLINE, "\\n", line, col));
        }
        advance();
    }
}
```

#### skipComment() (Lines 412-430)
```cpp
void Lexer::skipComment()
{
    if (current() == '/' && peek() == '/') {
        // Single-line comment
        while (!atEnd() && current() != '\n') {
            advance();
        }
    } else if (current() == '/' && peek() == '*') {
        // Multi-line comment
        advance(); advance(); // Skip /*
        while (!atEnd() && !(current() == '*' && peek() == '/')) {
            advance();
        }
        if (!atEnd()) {
            advance(); advance(); // Skip */
        }
    }
}
```

## Design Patterns and Architecture

### State Machine Pattern
The lexer operates as a finite state machine:
```
START → IDENTIFIER/KEYWORD → END
START → NUMBER → END
START → STRING → ESCAPE/END
START → OPERATOR → MULTI-CHAR/END
```

### Strategy Pattern
Different reading strategies for different token types:
- `readNumber()` for numeric literals
- `readString()` for string literals
- `readTemplateLiteral()` for complex interpolation
- `readIdentifierOrKeyword()` for identifiers and keywords
- `readOperator()` for operators and punctuation

### Factory Pattern
Token creation centralized in factory methods:
```cpp
Token(TokenType::NUMBER, "123", line, col);
Token(TokenType::IDENTIFIER, "variable", line, col);
```

## Performance Optimizations

### Efficient String Building
- **Pre-allocation**: Strings reserve capacity when size is known
- **Move Semantics**: Tokens moved instead of copied when possible
- **String Views**: Considered for read-only operations (not implemented)

### Hash Table Lookup
- **O(1) Keyword Detection**: Constant-time keyword vs identifier distinction
- **Perfect Hash**: Potential optimization for fixed keyword set

### Minimal Allocations
- **Token Reuse**: Tokens moved rather than copied
- **String Interning**: Considered for identifiers (not implemented)

## Error Handling

### Lexical Error Detection
- **Unexpected Characters**: Clear error messages with position
- **Unterminated Literals**: Strings and templates with proper bounds
- **Invalid Escapes**: Unknown escape sequences handled gracefully

### Position Tracking
- **Precise Locations**: Line and column for all tokens
- **Error Context**: Rich error information for debugging
- **Recovery**: Attempts to continue after errors (limited)

## Integration with Parser

### Token Stream
```cpp
auto tokens = lexer.tokenize();
Parser parser(std::move(tokens));
auto ast = parser.parse();
```

### Position Information
- **Error Reporting**: Line/column numbers propagated to parser
- **Debug Info**: Source locations preserved through compilation
- **Source Maps**: Potential for generating source maps (future feature)

## Why This Design Works

### Comprehensive Language Support
- **Multi-language Paradigms**: Supports Python, JavaScript, and C-style syntax
- **Modern Features**: Template literals, Unicode, complex operators
- **Extensibility**: Easy to add new token types and keywords

### Robust Error Handling
- **Clear Messages**: Informative error reports with positions
- **Graceful Degradation**: Attempts to continue after minor errors
- **Debugging Support**: Rich context for problem diagnosis

### Performance Considerations
- **Single Pass**: Efficient O(n) tokenization
- **Minimal Allocations**: Smart use of move semantics
- **Cache Friendly**: Sequential memory access patterns

The lexer provides a solid foundation for the Quantum Language compiler, handling the complexity of modern language features while maintaining performance and reliability.
