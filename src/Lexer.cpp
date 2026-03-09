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
    {"class", TokenType::CLASS},
    {"extends", TokenType::EXTENDS},
    {"new", TokenType::NEW},
    {"this", TokenType::THIS},
    {"self", TokenType::THIS}, // Quantum alias → same token
    {"super", TokenType::SUPER},
    {"return", TokenType::RETURN},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"elif", TokenType::ELIF},
    {"while", TokenType::WHILE},
    {"for", TokenType::FOR},
    {"in", TokenType::IN},
    {"of", TokenType::OF},
    {"break", TokenType::BREAK},
    {"continue", TokenType::CONTINUE},
    {"raise", TokenType::RAISE},
    {"throw", TokenType::RAISE},
    {"try", TokenType::TRY},
    {"except", TokenType::EXCEPT},
    {"catch", TokenType::EXCEPT}, // JS alias
    {"finally", TokenType::FINALLY},
    {"as", TokenType::AS}, // JS alias
    {"print", TokenType::PRINT},
    {"printf", TokenType::PRINT},
    {"input", TokenType::INPUT},
    {"scanf", TokenType::INPUT},
    {"cout", TokenType::COUT},
    {"cin", TokenType::CIN},
    {"import", TokenType::IMPORT},
    {"from", TokenType::FROM},
    {"true", TokenType::BOOL_TRUE},
    {"True", TokenType::BOOL_TRUE}, // Python
    {"false", TokenType::BOOL_FALSE},
    {"False", TokenType::BOOL_FALSE}, // Python
    {"nil", TokenType::NIL},
    {"null", TokenType::NIL},      // JavaScript alias
    {"undefined", TokenType::NIL}, // JavaScript alias
    {"None", TokenType::NIL},      // Python alias
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"not", TokenType::NOT},
    {"is", TokenType::IS},
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

void Lexer::skipBlockComment()
{
    // We've already consumed '/*' — skip until '*/'
    while (pos < src.size())
    {
        if (current() == '*' && peek() == '/')
        {
            advance(); // skip *
            advance(); // skip /
            return;
        }
        advance();
    }
    // Unterminated block comment — just silently reach EOF
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

// Template literal: `Hello ${name}, you are ${age} years old!`
// Emits: STRING("Hello ") PLUS LPAREN STRING(name-as-expr) RPAREN PLUS STRING(", you are ") ...
// We expand into: "seg0" + (expr1) + "seg1" + (expr2) + "seg2" ...
void Lexer::readTemplateLiteral(std::vector<Token> &out, int startLine, int startCol)
{
    advance(); // skip opening backtick

    // Collect alternating: text segment, then expression source string
    // We'll build a list of parts: {isExpr, content}
    struct Part
    {
        bool isExpr;
        std::string content;
    };
    std::vector<Part> parts;

    std::string seg;
    while (pos < src.size() && current() != '`')
    {
        if (current() == '\\')
        {
            advance();
            switch (current())
            {
            case 'n':
                seg += '\n';
                break;
            case 't':
                seg += '\t';
                break;
            case '`':
                seg += '`';
                break;
            case '\\':
                seg += '\\';
                break;
            case '$':
                seg += '$';
                break;
            default:
                seg += '\\';
                seg += current();
                break;
            }
            advance();
        }
        else if (current() == '$' && pos + 1 < src.size() && src[pos + 1] == '{')
        {
            // Flush current text segment
            parts.push_back({false, seg});
            seg.clear();
            advance(); // skip $
            advance(); // skip {
            // Collect expression source until matching }
            std::string expr;
            int depth = 1;
            while (pos < src.size() && depth > 0)
            {
                if (current() == '{')
                    depth++;
                else if (current() == '}')
                {
                    if (--depth == 0)
                    {
                        advance();
                        break;
                    }
                }
                expr += current();
                advance();
            }
            parts.push_back({true, expr});
        }
        else
        {
            seg += current();
            advance();
        }
    }
    if (pos < src.size())
        advance(); // skip closing backtick

    // Flush final text segment
    parts.push_back({false, seg});

    // Remove empty text-only parts at start/end to keep output clean
    // Build token sequence: part[0] + part[1] + ...
    // Text parts → STRING tokens, expr parts → re-lexed tokens wrapped in parens

    bool first = true;
    auto emitPlus = [&]()
    {
        if (!first)
            out.emplace_back(TokenType::PLUS, "+", startLine, startCol);
        first = false;
    };

    for (auto &p : parts)
    {
        if (!p.isExpr)
        {
            // Always emit text segments (even empty ones if they're the only part)
            if (!p.content.empty() || parts.size() == 1)
            {
                emitPlus();
                out.emplace_back(TokenType::STRING, p.content, startLine, startCol);
            }
        }
        else
        {
            // Re-lex the expression and wrap in parentheses
            // Use str() wrapper so numbers/bools get stringified in concat context
            emitPlus();
            // emit: str( <expr tokens> )
            out.emplace_back(TokenType::LPAREN, "(", startLine, startCol);
            Lexer subLex(p.content);
            auto subTokens = subLex.tokenize();
            for (auto &t : subTokens)
                if (t.type != TokenType::EOF_TOKEN && t.type != TokenType::NEWLINE)
                    out.push_back(t);
            out.emplace_back(TokenType::RPAREN, ")", startLine, startCol);
        }
    }

    // If the entire template was empty
    if (first)
        out.emplace_back(TokenType::STRING, "", startLine, startCol);
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

    // f-string prefix: f"..." or f'...'  — treat like a backtick template literal
    if ((id == "f" || id == "F") && pos < src.size() && (current() == '"' || current() == '\''))
    {
        char quote = current();
        advance(); // skip opening quote
        // Convert {expr} → ${expr} then re-lex as template
        std::string raw;
        while (pos < src.size() && current() != quote)
        {
            if (current() == '{')
            {
                advance(); // skip {
                std::string exprPart;
                std::string fmtPart;
                int depth = 1;
                bool inFormat = false;
                while (pos < src.size() && depth > 0)
                {
                    if (current() == '{')
                        depth++;
                    else if (current() == '}')
                    {
                        depth--;
                        if (depth == 0)
                        {
                            advance();
                            break;
                        }
                    }

                    // Only start formatting if we are at the top level of the interpolation
                    // to prevent matching dict colons like {"a": 1}
                    if (depth == 1 && current() == ':' && !inFormat)
                    {
                        inFormat = true;
                        advance();
                        continue;
                    }

                    if (inFormat)
                    {
                        fmtPart += current();
                    }
                    else
                    {
                        exprPart += current();
                    }
                    advance();
                }

                if (inFormat)
                {
                    raw += "${__format__(" + exprPart + ", \"" + fmtPart + "\")}";
                }
                else
                {
                    raw += "${" + exprPart + "}";
                }
            }
            else if (current() == '\\')
            {
                raw += current();
                advance();
                if (pos < src.size())
                {
                    raw += current();
                    advance();
                }
            }
            else
            {
                raw += current();
                advance();
            }
        }
        if (pos < src.size())
            advance(); // skip closing quote
        // Re-lex wrapped in backticks using the existing template literal engine
        std::string backtickSrc = "`" + raw + "`";
        Lexer subLex(backtickSrc);
        // We can't return multiple tokens from here — caller handles this
        // Store pending tokens and return a sentinel; instead, use a different approach:
        // Return a special STRING token with the expanded value by evaluating immediately
        // Actually the cleanest: tokenize the backtick source and push into the caller's stream
        // Since we can't do that from here, return a placeholder and let tokenize() handle it
        // INSTEAD: we set a member flag with the pending tokens
        pendingTokens_ = subLex.tokenize();
        // Remove EOF from pending
        if (!pendingTokens_.empty() && pendingTokens_.back().type == TokenType::EOF_TOKEN)
            pendingTokens_.pop_back();
        // Return a dummy that tokenize() will replace with pendingTokens_
        return Token(TokenType::UNKNOWN, "__fstring__", startLine, startCol);
    }

    auto it = keywords.find(id);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;

    // C preprocessor macro expansion: if this identifier was #defined, substitute it
    auto dit = defines_.find(id);
    if (dit != defines_.end() && !dit->second.empty())
    {
        // Single-token macro (most common: #define ROWS 18, #define RIGHT 3)
        if (dit->second.size() == 1)
            return dit->second[0]; // return the replacement token directly
        // Multi-token macro: push all but first into pendingTokens_
        pendingTokens_.insert(pendingTokens_.begin(),
                              dit->second.begin() + 1,
                              dit->second.end());
        return dit->second[0];
    }

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
            advance(); // consume the '#' itself
            // Skip horizontal whitespace after '#'
            while (pos < src.size() && (current() == ' ' || current() == '\t'))
                advance();
            // Read the directive name
            std::string directive;
            while (pos < src.size() && std::isalpha(current()))
                directive += advance();
            if (directive == "define")
            {
                // Skip spaces
                while (pos < src.size() && (current() == ' ' || current() == '\t'))
                    advance();
                // Read macro name
                std::string macroName;
                while (pos < src.size() && (std::isalnum(current()) || current() == '_'))
                    macroName += advance();
                // Skip spaces between name and value
                while (pos < src.size() && (current() == ' ' || current() == '\t'))
                    advance();
                // Read replacement value tokens until end of line
                std::vector<Token> replacement;
                while (pos < src.size() && current() != '\n' && current() != '\r')
                {
                    // Skip whitespace between tokens
                    while (pos < src.size() && (current() == ' ' || current() == '\t'))
                        advance();
                    if (pos >= src.size() || current() == '\n' || current() == '\r')
                        break;
                    int tl = line, tc = col;
                    char rc = current();
                    if (std::isdigit(rc) || (rc == '-' && pos + 1 < src.size() && std::isdigit(src[pos + 1])))
                    {
                        replacement.push_back(readNumber());
                    }
                    else if (rc == '\'' || rc == '"')
                    {
                        replacement.push_back(readString(rc));
                    }
                    else if (std::isalpha(rc) || rc == '_')
                    {
                        // Read identifier — may itself be a previously defined macro
                        std::string id;
                        while (pos < src.size() && (std::isalnum(current()) || current() == '_'))
                            id += advance();
                        // Check if this identifier is itself a macro (simple one-level expansion)
                        auto dit = defines_.find(id);
                        if (dit != defines_.end())
                        {
                            for (auto &t : dit->second)
                                replacement.push_back(t);
                        }
                        else
                        {
                            replacement.emplace_back(TokenType::IDENTIFIER, id, tl, tc);
                        }
                    }
                    else
                    {
                        // Operator/punctuation — skip rest of line to keep things simple
                        while (pos < src.size() && current() != '\n' && current() != '\r')
                            advance();
                        break;
                    }
                }
                if (!macroName.empty())
                    defines_[macroName] = std::move(replacement);
            }
            // Skip rest of directive line (#include, #ifdef, #ifndef, #endif, #else, #undef, etc.)
            while (pos < src.size() && current() != '\n' && current() != '\r')
                advance();
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
        if (c == '`')
        {
            readTemplateLiteral(rawTokens, startLine, startCol);
            continue;
        }
        if (std::isalpha(c) || c == '_')
        {
            auto tok = readIdentifierOrKeyword();
            if (tok.type == TokenType::UNKNOWN && tok.value == "__fstring__")
            {
                // f-string expansion — flush pending tokens
                for (auto &pt : pendingTokens_)
                    rawTokens.push_back(pt);
                pendingTokens_.clear();
            }
            else
            {
                rawTokens.push_back(tok);
                // Flush any extra tokens from multi-token macro expansion
                if (!pendingTokens_.empty())
                {
                    for (auto &pt : pendingTokens_)
                        rawTokens.push_back(pt);
                    pendingTokens_.clear();
                }
            }
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
            // Regex literal detection: /pattern/flags
            // Only when prev token is NOT a value (number/string/ident/)/])
            if (current() != '/' && current() != '*' && current() != '=')
            {
                bool prevIsVal = false;
                if (!rawTokens.empty())
                {
                    TokenType ptt = rawTokens.back().type;
                    prevIsVal = (ptt == TokenType::NUMBER || ptt == TokenType::STRING ||
                                 ptt == TokenType::IDENTIFIER || ptt == TokenType::RPAREN ||
                                 ptt == TokenType::RBRACKET || ptt == TokenType::BOOL_TRUE ||
                                 ptt == TokenType::BOOL_FALSE);
                }
                if (!prevIsVal)
                {
                    // Lex regex: scan to unescaped '/' respecting character classes [...]
                    std::string regStr = "/";
                    bool inCls = false;
                    while (pos < src.size() && current() != '\n')
                    {
                        char rc = src[pos];
                        if (rc == '\\' && pos + 1 < src.size())
                        {
                            regStr += rc;
                            regStr += src[pos + 1];
                            pos += 2;
                            col += 2;
                            continue;
                        }
                        if (rc == '[')
                            inCls = true;
                        if (rc == ']')
                            inCls = false;
                        if (rc == '/' && !inCls)
                        {
                            advance();
                            regStr += '/';
                            break;
                        }
                        regStr += rc;
                        advance();
                    }
                    // eat flags: g i m s u y
                    while (pos < src.size() && std::isalpha(current()))
                        regStr += advance();
                    rawTokens.emplace_back(TokenType::STRING, regStr, startLine, startCol);
                    continue;
                }
            }
            if (current() == '/')
            {
                // Distinguish Python floor-division // from C/JS // line comment.
                // Strategy: only treat // as floor-div when the immediately preceding
                // token is a value-producing token (number, string, identifier, ), ]).
                // In all other positions (start of line, after operator, etc.) it is
                // a comment.  This matches real-world usage across languages.
                {
                    bool prevIsValue = false;
                    if (!rawTokens.empty())
                    {
                        TokenType pt = rawTokens.back().type;
                        prevIsValue = (pt == TokenType::NUMBER ||
                                       pt == TokenType::STRING ||
                                       pt == TokenType::BOOL_TRUE ||
                                       pt == TokenType::BOOL_FALSE ||
                                       pt == TokenType::NIL ||
                                       pt == TokenType::IDENTIFIER ||
                                       pt == TokenType::RPAREN ||
                                       pt == TokenType::RBRACKET);
                    }
                    if (prevIsValue)
                    {
                        advance(); // consume second '/'
                        rawTokens.emplace_back(TokenType::FLOOR_DIV, "//", startLine, startCol);
                    }
                    else
                        skipComment();
                }
            }
            else if (current() == '*')
            {
                advance(); // consume the *
                skipBlockComment();
            }
            else if (current() == '=')
            {
                advance();
                rawTokens.emplace_back(TokenType::SLASH_ASSIGN, "/=", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::SLASH, "/", startLine, startCol);
            break;
        case '=':
            if (current() == '=')
            {
                advance();
                if (current() == '=')
                {
                    advance();
                    rawTokens.emplace_back(TokenType::STRICT_EQ, "===", startLine, startCol);
                }
                else
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
                if (current() == '=')
                {
                    advance();
                    rawTokens.emplace_back(TokenType::STRICT_NEQ, "!==", startLine, startCol);
                }
                else
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
            else if (current() == '=')
            {
                advance();
                rawTokens.emplace_back(TokenType::AND_ASSIGN, "&=", startLine, startCol);
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
            else if (current() == '=')
            {
                advance();
                rawTokens.emplace_back(TokenType::OR_ASSIGN, "|=", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::BIT_OR, "|", startLine, startCol);
            break;
        case '^':
            if (current() == '=')
            {
                advance();
                rawTokens.emplace_back(TokenType::XOR_ASSIGN, "^=", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::BIT_XOR, "^", startLine, startCol);
            break;
        case '~':
            rawTokens.emplace_back(TokenType::BIT_NOT, "~", startLine, startCol);
            break;
        case '%':
            if (current() == '=')
            {
                advance();
                rawTokens.emplace_back(TokenType::MOD_ASSIGN, "%=", startLine, startCol);
            }
            else
                rawTokens.emplace_back(TokenType::PERCENT, "%", startLine, startCol);
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
            if (current() == '.' && pos + 1 < src.size() && src[pos + 1] == '.')
            {
                advance();
                advance(); // eat the remaining two dots
                rawTokens.emplace_back(TokenType::IDENTIFIER, "...", startLine, startCol);
            }
            else
            {
                rawTokens.emplace_back(TokenType::DOT, ".", startLine, startCol);
            }
            break;
        case '?':
            rawTokens.emplace_back(TokenType::QUESTION, "?", startLine, startCol);
            break;
        case '@':
            rawTokens.emplace_back(TokenType::DECORATOR, "@", startLine, startCol);
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
    int bracketDepth = 0; // track ( { [ depth — never emit INDENT/DEDENT inside these
    int parenBracketDepth = 0; // track ( [ depth only — entirely drop NEWLINE inside these

    for (size_t i = 0; i < rawTokens.size(); ++i)
    {
        Token &tok = rawTokens[i];

        // Track bracket/brace/paren depth
        if (tok.type == TokenType::LBRACE ||
            tok.type == TokenType::LBRACKET ||
            tok.type == TokenType::LPAREN)
        {
            bracketDepth++;
            if (tok.type != TokenType::LBRACE)
                parenBracketDepth++;
        }
        else if (tok.type == TokenType::RBRACE ||
                 tok.type == TokenType::RBRACKET ||
                 tok.type == TokenType::RPAREN)
        {
            bracketDepth = std::max(0, bracketDepth - 1);
            if (tok.type != TokenType::RBRACE)
                parenBracketDepth = std::max(0, parenBracketDepth - 1);
        }

        if (tok.type == TokenType::NEWLINE && parenBracketDepth > 0)
            continue; // Drop NEWLINE entirely inside ( ) and [ ]

        // COLON followed by NEWLINE + deeper indent → open Python block
        // But NOT inside brackets/braces/parens (dict literal, array, call args)
        if (tok.type == TokenType::COLON && bracketDepth == 0)
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
        // But NOT inside brackets/braces/parens
        if (tok.type == TokenType::NEWLINE && bracketDepth == 0)
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