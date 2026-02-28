#include "../include/Lexer.h"
#include "../include/Error.h"
#include <stdexcept>
#include <cctype>
#include <sstream>

const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"let", TokenType::LET},
    {"const", TokenType::CONST},
    {"fn", TokenType::FN},
    {"def", TokenType::DEF},
    {"function", TokenType::FUNCTION},
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
    std::vector<Token> rawTokens;

    while (pos < src.size())
    {
        skipWhitespace();
        if (pos >= src.size())
            break;

        char c = current();
        int startLine = line, startCol = col;

        if (c == '\n')
        {
            rawTokens.emplace_back(TokenType::NEWLINE, "\\n", startLine, startCol);
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
            rawTokens.push_back(readNumber());
            continue;
        }
        if (c == '"' || c == '\'')
        {
            rawTokens.push_back(readString(c));
            continue;
        }
        if (std::isalpha(c) || c == '_')
        {
            rawTokens.push_back(readIdentifierOrKeyword());
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
                rawTokens.emplace_back(TokenType::PLUS_PLUS, "++", startLine, startCol);
            }
            else if (current() == '=')
            {
                advance();
                rawTokens.emplace_back(TokenType::PLUS_ASSIGN, "+=", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::PLUS, "+", startLine, startCol);
            break;
        case '-':
            if (current() == '-')
            {
                advance();
                rawTokens.emplace_back(TokenType::MINUS_MINUS, "--", startLine, startCol);
            }
            else if (current() == '>')
            {
                advance();
                rawTokens.emplace_back(TokenType::ARROW, "->", startLine, startCol);
            }
            else if (current() == '=')
            {
                advance();
                rawTokens.emplace_back(TokenType::MINUS_ASSIGN, "-=", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::MINUS, "-", startLine, startCol);
            break;
        case '*':
            if (current() == '*')
            {
                advance();
                rawTokens.emplace_back(TokenType::POWER, "**", startLine, startCol);
            }
            else if (current() == '=')
            {
                advance();
                rawTokens.emplace_back(TokenType::STAR_ASSIGN, "*=", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::STAR, "*", startLine, startCol);
            break;
        case '/':
            if (current() == '/')
            {
                skipComment();
            }
            else if (current() == '=')
            {
                advance();
                rawTokens.emplace_back(TokenType::SLASH_ASSIGN, "/=", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::SLASH, "/", startLine, startCol);
            break;
        case '%':
            rawTokens.emplace_back(TokenType::PERCENT, "%", startLine, startCol);
            break;
        case '=':
            if (current() == '=')
            {
                advance();
                rawTokens.emplace_back(TokenType::EQ, "==", startLine, startCol);
            }
            else if (current() == '>')
            {
                advance();
                rawTokens.emplace_back(TokenType::FAT_ARROW, "=>", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::ASSIGN, "=", startLine, startCol);
            break;
        case '!':
            if (current() == '=')
            {
                advance();
                rawTokens.emplace_back(TokenType::NEQ, "!=", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::NOT, "!", startLine, startCol);
            break;
        case '<':
            if (current() == '=')
            {
                advance();
                rawTokens.emplace_back(TokenType::LTE, "<=", startLine, startCol);
            }
            else if (current() == '<')
            {
                advance();
                rawTokens.emplace_back(TokenType::LSHIFT, "<<", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::LT, "<", startLine, startCol);
            break;
        case '>':
            if (current() == '=')
            {
                advance();
                rawTokens.emplace_back(TokenType::GTE, ">=", startLine, startCol);
            }
            else if (current() == '>')
            {
                advance();
                rawTokens.emplace_back(TokenType::RSHIFT, ">>", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::GT, ">", startLine, startCol);
            break;
        case '&':
            if (current() == '&')
            {
                advance();
                rawTokens.emplace_back(TokenType::AND_AND, "&&", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::BIT_AND, "&", startLine, startCol);
            break;
        case '|':
            if (current() == '|')
            {
                advance();
                rawTokens.emplace_back(TokenType::OR_OR, "||", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::BIT_OR, "|", startLine, startCol);
            break;
        case '^':
            rawTokens.emplace_back(TokenType::BIT_XOR, "^", startLine, startCol);
            break;
        case '~':
            rawTokens.emplace_back(TokenType::BIT_NOT, "~", startLine, startCol);
            break;
        case '(':
            rawTokens.emplace_back(TokenType::LPAREN, "(", startLine, startCol);
            break;
        case ')':
            rawTokens.emplace_back(TokenType::RPAREN, ")", startLine, startCol);
            break;
        case '{':
            rawTokens.emplace_back(TokenType::LBRACE, "{", startLine, startCol);
            break;
        case '}':
            rawTokens.emplace_back(TokenType::RBRACE, "}", startLine, startCol);
            break;
        case '[':
            rawTokens.emplace_back(TokenType::LBRACKET, "[", startLine, startCol);
            break;
        case ']':
            rawTokens.emplace_back(TokenType::RBRACKET, "]", startLine, startCol);
            break;
        case ',':
            rawTokens.emplace_back(TokenType::COMMA, ",", startLine, startCol);
            break;
        case ';':
            rawTokens.emplace_back(TokenType::SEMICOLON, ";", startLine, startCol);
            break;
        case ':':
            rawTokens.emplace_back(TokenType::COLON, ":", startLine, startCol);
            break;
        case '.':
            rawTokens.emplace_back(TokenType::DOT, ".", startLine, startCol);
            break;
        case '?':
            rawTokens.emplace_back(TokenType::QUESTION, "?", startLine, startCol);
            break;
        default:
            throw QuantumError("LexError", std::string("Unexpected character: ") + c, startLine);
        }
    }

    rawTokens.emplace_back(TokenType::EOF_TOKEN, "", line, col);

    // ── Python-style INDENT/DEDENT post-processing ───────────────────────────
    // Scan for COLON + NEWLINE + more-indented line → inject INDENT/DEDENT tokens.
    // Brace-style { } files are completely unaffected.

    std::vector<Token> tokens;
    tokens.reserve(rawTokens.size() + 32);

    // Precompute leading-space count for each line (tabs = 4 spaces)
    std::vector<int> indentOf(line + 2, 0);
    {
        int curLine = 1, curIndent = 0;
        bool lineStart = true;
        for (size_t i = 0; i < src.size(); ++i)
        {
            char ch = src[i];
            if (ch == '\n')
            {
                indentOf[curLine] = curIndent;
                curLine++;
                curIndent = 0;
                lineStart = true;
            }
            else if (lineStart)
            {
                if (ch == ' ')
                    curIndent++;
                else if (ch == '\t')
                    curIndent += 4;
                else
                    lineStart = false;
            }
        }
        indentOf[curLine] = curIndent;
    }

    std::vector<int> indentStack = {0};

    for (size_t i = 0; i < rawTokens.size(); ++i)
    {
        Token &tok = rawTokens[i];

        // COLON followed by NEWLINE + deeper indent → open Python block
        if (tok.type == TokenType::COLON)
        {
            size_t j = i + 1;
            while (j < rawTokens.size() && rawTokens[j].type == TokenType::NEWLINE)
                ++j;
            if (j < rawTokens.size() && rawTokens[j].type != TokenType::EOF_TOKEN)
            {
                int nextIndent = indentOf[rawTokens[j].line];
                if (nextIndent > indentStack.back())
                {
                    tokens.push_back(tok);
                    for (size_t k = i + 1; k < j; ++k)
                        tokens.push_back(rawTokens[k]);
                    i = j - 1;
                    indentStack.push_back(nextIndent);
                    tokens.emplace_back(TokenType::INDENT, "INDENT", tok.line, tok.col);
                    continue;
                }
            }
            tokens.push_back(tok);
            continue;
        }

        // After NEWLINE, emit DEDENTs if next line is less indented
        if (tok.type == TokenType::NEWLINE)
        {
            tokens.push_back(tok);
            size_t j = i + 1;
            while (j < rawTokens.size() && rawTokens[j].type == TokenType::NEWLINE)
                ++j;
            if (j < rawTokens.size() && rawTokens[j].type != TokenType::EOF_TOKEN)
            {
                int nextIndent = indentOf[rawTokens[j].line];
                while (indentStack.size() > 1 && nextIndent < indentStack.back())
                {
                    indentStack.pop_back();
                    tokens.emplace_back(TokenType::DEDENT, "DEDENT", tok.line, tok.col);
                }
            }
            else
            {
                while (indentStack.size() > 1)
                {
                    indentStack.pop_back();
                    tokens.emplace_back(TokenType::DEDENT, "DEDENT", tok.line, tok.col);
                }
            }
            continue;
        }

        tokens.push_back(tok);
    }

    return tokens;
}