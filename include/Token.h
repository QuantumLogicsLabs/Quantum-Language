#pragma once
#include <string>

enum class TokenType
{
    // Literals
    NUMBER,
    STRING,
    BOOL_TRUE,
    BOOL_FALSE,
    NIL,

    // Identifiers & Keywords
    IDENTIFIER,
    LET,
    CONST,
    FN,
    RETURN,
    IF,
    ELSE,
    ELIF,
    WHILE,
    FOR,
    IN,
    BREAK,
    CONTINUE,
    PRINT,
    INPUT,
    COUT, // cout
    CIN,  // cin
    IMPORT,
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

    // Cybersecurity reserved keywords (future)
    SCAN,
    PAYLOAD,
    ENCRYPT,
    DECRYPT,
    HASH,

    // Operators
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    POWER,
    EQ,
    NEQ,
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
    NEWLINE,

    // Special
    EOF_TOKEN,
    UNKNOWN
};

struct Token
{
    TokenType type;
    std::string value;
    int line;
    int col;

    Token(TokenType t, std::string v, int ln, int c)
        : type(t), value(std::move(v)), line(ln), col(c) {}

    std::string toString() const;
};