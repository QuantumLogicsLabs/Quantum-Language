#include "../include/Lexer.h"
#include "../include/Error.h"
#include <stdexcept>
#include <cctype>
#include <sstream>

const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"let", TokenType::LET},
    {"const", TokenType::CONST},
    {"fn", TokenType::FN},
    {"return", TokenType::RETURN},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"elif", TokenType::ELIF},
    {"while", TokenType::WHILE},
    {"for", TokenType::FOR},
    {"in", TokenType::IN},
    {"break", TokenType::BREAK},
    {"continue", TokenType::CONTINUE},
    {"print", TokenType::PRINT},
    {"printf", TokenType::PRINT},
    {"input", TokenType::INPUT},
    {"scanf", TokenType::INPUT},
    {"cout", TokenType::COUT},
    {"cin", TokenType::CIN},
    {"import", TokenType::IMPORT},
    {"true", TokenType::BOOL_TRUE},
    {"false", TokenType::BOOL_FALSE},
    {"nil", TokenType::NIL},
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"not", TokenType::NOT},
    // C/C++ style type keywords
    {"int", TokenType::TYPE_INT},
    {"float", TokenType::TYPE_FLOAT},
    {"double", TokenType::TYPE_DOUBLE},
    {"char", TokenType::TYPE_CHAR},
    {"string", TokenType::TYPE_STRING},
    {"bool", TokenType::TYPE_BOOL},
    {"void", TokenType::TYPE_VOID},
    {"long", TokenType::TYPE_LONG},
    {"short", TokenType::TYPE_SHORT},
    {"unsigned", TokenType::TYPE_UNSIGNED},
    // Cybersecurity future keywords
    {"scan", TokenType::SCAN},
    {"payload", TokenType::PAYLOAD},
    {"encrypt", TokenType::ENCRYPT},
    {"decrypt", TokenType::DECRYPT},
    {"hash", TokenType::HASH},
};

Lexer::Lexer(const std::string &source)
    : src(source), pos(0), line(1), col(1) {}

char Lexer::current() const
{
    return pos < src.size() ? src[pos] : '\0';
}

char Lexer::peek(int offset) const
{
    size_t p = pos + offset;
    return p < src.size() ? src[p] : '\0';
}

char Lexer::advance()
{
    char c = src[pos++];
    if (c == '\n')
    {
        line++;
        col = 1;
    }
    else
        col++;
    return c;
}

void Lexer::skipWhitespace()
{
    while (pos < src.size() && (current() == ' ' || current() == '\t' || current() == '\r'))
        advance();
}

void Lexer::skipComment()
{
    while (pos < src.size() && current() != '\n')
        advance();
}

Token Lexer::readNumber()
{
    int startLine = line, startCol = col;
    std::string num;
    bool hasDot = false;

    if (current() == '0' && (peek() == 'x' || peek() == 'X'))
    {
        num += advance();
        num += advance(); // 0x
        while (pos < src.size() && std::isxdigit(current()))
            num += advance();
    }
    else
    {
        while (pos < src.size() && (std::isdigit(current()) || current() == '.'))
        {
            if (current() == '.')
            {
                if (hasDot)
                    break;
                hasDot = true;
            }
            num += advance();
        }
    }
    return Token(TokenType::NUMBER, num, startLine, startCol);
}

Token Lexer::readString(char quote)
{
    int startLine = line, startCol = col;
    advance(); // skip opening quote
    std::string str;
    while (pos < src.size() && current() != quote)
    {
        if (current() == '\\')
        {
            advance();
            switch (current())
            {
            case 'n':
                str += '\n';
                break;
            case 't':
                str += '\t';
                break;
            case 'r':
                str += '\r';
                break;
            case '\\':
                str += '\\';
                break;
            case '\'':
                str += '\'';
                break;
            case '"':
                str += '"';
                break;
            case '0':
                str += '\0';
                break;
            default:
                str += current();
            }
            advance();
        }
        else
        {
            str += advance();
        }
    }
    if (pos >= src.size())
        throw QuantumError("LexError", "Unterminated string literal", startLine);
    advance(); // skip closing quote
    return Token(TokenType::STRING, str, startLine, startCol);
}

Token Lexer::readIdentifierOrKeyword()
{
    int startLine = line, startCol = col;
    std::string id;
    while (pos < src.size() && (std::isalnum(current()) || current() == '_'))
        id += advance();
    auto it = keywords.find(id);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
    return Token(type, id, startLine, startCol);
}

std::vector<Token> Lexer::tokenize()
{
    std::vector<Token> tokens;

    while (pos < src.size())
    {
        skipWhitespace();
        if (pos >= src.size())
            break;

        char c = current();
        int startLine = line, startCol = col;

        if (c == '\n')
        {
            tokens.emplace_back(TokenType::NEWLINE, "\\n", startLine, startCol);
            advance();
            continue;
        }

        if (c == '#')
        {
            skipComment();
            continue;
        }

        if (std::isdigit(c))
        {
            tokens.push_back(readNumber());
            continue;
        }
        if (c == '"' || c == '\'')
        {
            tokens.push_back(readString(c));
            continue;
        }
        if (std::isalpha(c) || c == '_')
        {
            tokens.push_back(readIdentifierOrKeyword());
            continue;
        }

        // Operators & delimiters
        advance();
        switch (c)
        {
        case '+':
            if (current() == '+')
            {
                advance();
                tokens.emplace_back(TokenType::PLUS_PLUS, "++", startLine, startCol);
            }
            else if (current() == '=')
            {
                advance();
                tokens.emplace_back(TokenType::PLUS_ASSIGN, "+=", startLine, startCol);
            }
            else
                tokens.emplace_back(TokenType::PLUS, "+", startLine, startCol);
            break;
        case '-':
            if (current() == '-')
            {
                advance();
                tokens.emplace_back(TokenType::MINUS_MINUS, "--", startLine, startCol);
            }
            else if (current() == '>')
            {
                advance();
                tokens.emplace_back(TokenType::ARROW, "->", startLine, startCol);
            }
            else if (current() == '=')
            {
                advance();
                tokens.emplace_back(TokenType::MINUS_ASSIGN, "-=", startLine, startCol);
            }
            else
                tokens.emplace_back(TokenType::MINUS, "-", startLine, startCol);
            break;
        case '*':
            if (current() == '*')
            {
                advance();
                tokens.emplace_back(TokenType::POWER, "**", startLine, startCol);
            }
            else if (current() == '=')
            {
                advance();
                tokens.emplace_back(TokenType::STAR_ASSIGN, "*=", startLine, startCol);
            }
            else
                tokens.emplace_back(TokenType::STAR, "*", startLine, startCol);
            break;
        case '/':
            if (current() == '/')
            {
                skipComment();
            }
            else if (current() == '=')
            {
                advance();
                tokens.emplace_back(TokenType::SLASH_ASSIGN, "/=", startLine, startCol);
            }
            else
                tokens.emplace_back(TokenType::SLASH, "/", startLine, startCol);
            break;
        case '%':
            tokens.emplace_back(TokenType::PERCENT, "%", startLine, startCol);
            break;
        case '=':
            if (current() == '=')
            {
                advance();
                tokens.emplace_back(TokenType::EQ, "==", startLine, startCol);
            }
            else
                tokens.emplace_back(TokenType::ASSIGN, "=", startLine, startCol);
            break;
        case '!':
            if (current() == '=')
            {
                advance();
                tokens.emplace_back(TokenType::NEQ, "!=", startLine, startCol);
            }
            else
                tokens.emplace_back(TokenType::NOT, "!", startLine, startCol);
            break;
        case '<':
            if (current() == '=')
            {
                advance();
                tokens.emplace_back(TokenType::LTE, "<=", startLine, startCol);
            }
            else if (current() == '<')
            {
                advance();
                tokens.emplace_back(TokenType::LSHIFT, "<<", startLine, startCol);
            }
            else
                tokens.emplace_back(TokenType::LT, "<", startLine, startCol);
            break;
        case '>':
            if (current() == '=')
            {
                advance();
                tokens.emplace_back(TokenType::GTE, ">=", startLine, startCol);
            }
            else if (current() == '>')
            {
                advance();
                tokens.emplace_back(TokenType::RSHIFT, ">>", startLine, startCol);
            }
            else
                tokens.emplace_back(TokenType::GT, ">", startLine, startCol);
            break;
        case '&':
            if (current() == '&')
            {
                advance();
                tokens.emplace_back(TokenType::AND_AND, "&&", startLine, startCol);
            }
            else
                tokens.emplace_back(TokenType::BIT_AND, "&", startLine, startCol);
            break;
        case '|':
            if (current() == '|')
            {
                advance();
                tokens.emplace_back(TokenType::OR_OR, "||", startLine, startCol);
            }
            else
                tokens.emplace_back(TokenType::BIT_OR, "|", startLine, startCol);
            break;
        case '^':
            tokens.emplace_back(TokenType::BIT_XOR, "^", startLine, startCol);
            break;
        case '~':
            tokens.emplace_back(TokenType::BIT_NOT, "~", startLine, startCol);
            break;
        case '(':
            tokens.emplace_back(TokenType::LPAREN, "(", startLine, startCol);
            break;
        case ')':
            tokens.emplace_back(TokenType::RPAREN, ")", startLine, startCol);
            break;
        case '{':
            tokens.emplace_back(TokenType::LBRACE, "{", startLine, startCol);
            break;
        case '}':
            tokens.emplace_back(TokenType::RBRACE, "}", startLine, startCol);
            break;
        case '[':
            tokens.emplace_back(TokenType::LBRACKET, "[", startLine, startCol);
            break;
        case ']':
            tokens.emplace_back(TokenType::RBRACKET, "]", startLine, startCol);
            break;
        case ',':
            tokens.emplace_back(TokenType::COMMA, ",", startLine, startCol);
            break;
        case ';':
            tokens.emplace_back(TokenType::SEMICOLON, ";", startLine, startCol);
            break;
        case ':':
            tokens.emplace_back(TokenType::COLON, ":", startLine, startCol);
            break;
        case '.':
            tokens.emplace_back(TokenType::DOT, ".", startLine, startCol);
            break;
        case '?':
            tokens.emplace_back(TokenType::QUESTION, "?", startLine, startCol);
            break;
        default:
            throw QuantumError("LexError", std::string("Unexpected character: ") + c, startLine);
        }
    }

    tokens.emplace_back(TokenType::EOF_TOKEN, "", line, col);
    return tokens;
}