#include "../include/Parser.h"
#include <sstream>

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)), pos(0) {}

Token &Parser::current() { return tokens[pos]; }
Token &Parser::peek(int offset)
{
    size_t p = pos + offset;
    return p < tokens.size() ? tokens[p] : tokens.back();
}

Token &Parser::consume() { return tokens[pos++]; }

Token &Parser::expect(TokenType t, const std::string &msg)
{
    if (current().type != t)
        throw ParseError(msg + " (got '" + current().value + "')", current().line, current().col);
    return consume();
}

bool Parser::check(TokenType t) const { return tokens[pos].type == t; }
bool Parser::match(TokenType t)
{
    if (check(t))
    {
        consume();
        return true;
    }
    return false;
}
bool Parser::atEnd() const { return tokens[pos].type == TokenType::EOF_TOKEN; }

void Parser::skipNewlines()
{
    while (check(TokenType::NEWLINE))
        consume();
}

ASTNodePtr Parser::parse()
{
    auto block = std::make_unique<ASTNode>(BlockStmt{}, 0);
    auto &stmts = block->as<BlockStmt>().statements;
    skipNewlines();
    while (!atEnd())
    {
        stmts.push_back(parseStatement());
        skipNewlines();
    }
    return block;
}

ASTNodePtr Parser::parseStatement()
{
    skipNewlines();
    int ln = current().line;
    switch (current().type)
    {
    case TokenType::LET:
    {
        consume();
        return parseVarDecl(false);
    }
    case TokenType::CONST:
    {
        consume();
        return parseVarDecl(true);
    }
    case TokenType::FN:
    case TokenType::DEF:
    case TokenType::FUNCTION:
    {
        consume();
        // "function name(...)" → named declaration
        // "function(...)"      → anonymous, treat as expr statement (lambda)
        if (current().type == TokenType::IDENTIFIER)
            return parseFunctionDecl();
        // anonymous function used as statement — rare but valid
        // fall through to expr parsing via parseExprStmt would double-consume;
        // instead parse it as an expression statement manually
        auto lam = parseLambda();
        while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
            consume();
        return std::make_unique<ASTNode>(ExprStmt{std::move(lam)}, lam->line);
    }
    case TokenType::IF:
    {
        consume();
        return parseIfStmt();
    }
    case TokenType::WHILE:
    {
        consume();
        return parseWhileStmt();
    }
    case TokenType::FOR:
    {
        consume();
        return parseForStmt();
    }
    case TokenType::RETURN:
    {
        consume();
        return parseReturnStmt();
    }
    case TokenType::PRINT:
    {
        consume();
        return parsePrintStmt();
    }
    case TokenType::INPUT:
    {
        consume();
        return parseInputStmt();
    }
    case TokenType::COUT:
    {
        consume();
        return parseCoutStmt();
    }
    case TokenType::CIN:
    {
        consume();
        return parseCinStmt();
    }
    case TokenType::IMPORT:
    {
        consume();
        return parseImportStmt();
    }
    case TokenType::BREAK:
    {
        consume();
        while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
            consume();
        return std::make_unique<ASTNode>(BreakStmt{}, ln);
    }
    case TokenType::CONTINUE:
    {
        consume();
        while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
            consume();
        return std::make_unique<ASTNode>(ContinueStmt{}, ln);
    }
    case TokenType::LBRACE:
        return parseBlock();
    // ── C/C++ style typed declarations ────────────────────────────────────
    case TokenType::TYPE_INT:
    case TokenType::TYPE_FLOAT:
    case TokenType::TYPE_DOUBLE:
    case TokenType::TYPE_CHAR:
    case TokenType::TYPE_STRING:
    case TokenType::TYPE_BOOL:
    case TokenType::TYPE_VOID:
    case TokenType::TYPE_LONG:
    case TokenType::TYPE_SHORT:
    case TokenType::TYPE_UNSIGNED:
    {
        // Only treat as a C-type declaration if followed by an identifier.
        // e.g.  "int x = 5"  → declaration
        //       "string = x" → plain assignment (string used as variable name)
        // Peek past any chained type qualifiers to find the next real token.
        size_t lookahead = pos + 1;
        while (lookahead < tokens.size() && isCTypeKeyword(tokens[lookahead].type))
            ++lookahead;
        if (lookahead < tokens.size() && tokens[lookahead].type == TokenType::IDENTIFIER)
        {
            auto typeHint = consume().value;
            while (isCTypeKeyword(current().type))
                typeHint += " " + consume().value;
            return parseCTypeVarDecl(typeHint);
        }
        // Not a declaration — fall through to expression statement
        return parseExprStmt();
    }
    default:
        return parseExprStmt();
    }
}

ASTNodePtr Parser::parseBlock()
{
    // Brace-style: { statements }
    if (check(TokenType::LBRACE))
    {
        int ln = current().line;
        expect(TokenType::LBRACE, "Expected '{'");
        skipNewlines();
        BlockStmt block;
        while (!check(TokenType::RBRACE) && !atEnd())
        {
            block.statements.push_back(parseStatement());
            skipNewlines();
        }
        expect(TokenType::RBRACE, "Expected '}'");
        return std::make_unique<ASTNode>(std::move(block), ln);
    }
    // Python-style: INDENT statements DEDENT
    if (check(TokenType::INDENT))
    {
        int ln = current().line;
        consume(); // eat INDENT
        skipNewlines();
        BlockStmt block;
        while (!check(TokenType::DEDENT) && !atEnd())
        {
            block.statements.push_back(parseStatement());
            skipNewlines();
        }
        if (check(TokenType::DEDENT))
            consume(); // eat DEDENT
        return std::make_unique<ASTNode>(std::move(block), ln);
    }
    throw ParseError("Expected '{' or indented block", current().line, current().col);
}

// Accepts { block }, INDENT block, or a single statement
ASTNodePtr Parser::parseBodyOrStatement()
{
    if (check(TokenType::LBRACE) || check(TokenType::INDENT))
        return parseBlock();
    int ln = current().line;
    BlockStmt block;
    block.statements.push_back(parseStatement());
    return std::make_unique<ASTNode>(std::move(block), ln);
}

ASTNodePtr Parser::parseVarDecl(bool isConst)
{
    int ln = current().line;
    auto nameToken = expect(TokenType::IDENTIFIER, "Expected variable name");
    ASTNodePtr init;
    if (match(TokenType::ASSIGN))
    {
        init = parseExpr();
    }
    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
        consume();
    return std::make_unique<ASTNode>(VarDecl{isConst, nameToken.value, std::move(init), ""}, ln);
}

ASTNodePtr Parser::parseFunctionDecl()
{
    int ln = current().line;
    auto nameToken = expect(TokenType::IDENTIFIER, "Expected function name");
    auto params = parseParamList();

    // Skip optional return type annotation: -> type  or  -> SomeType
    if (check(TokenType::ARROW))
    {
        consume(); // eat ->
        // consume tokens until we hit : or { or NEWLINE or INDENT
        while (!atEnd() && !check(TokenType::COLON) && !check(TokenType::LBRACE) && !check(TokenType::NEWLINE) && !check(TokenType::INDENT))
            consume();
    }

    match(TokenType::COLON); // optional Python-style colon
    skipNewlines();
    auto body = parseBlock();
    return std::make_unique<ASTNode>(FunctionDecl{nameToken.value, std::move(params), std::move(body)}, ln);
}

ASTNodePtr Parser::parseIfStmt()
{
    int ln = current().line;
    auto cond = parseExpr();
    // consume optional colon (Python style: "if x > 0:")
    match(TokenType::COLON);
    skipNewlines();
    auto then = parseBodyOrStatement();
    skipNewlines();
    ASTNodePtr elseBranch;
    if (check(TokenType::ELIF))
    {
        consume();
        // optional colon after elif condition is handled inside recursive call
        elseBranch = parseIfStmt();
    }
    else if (check(TokenType::ELSE))
    {
        consume();
        skipNewlines();
        if (check(TokenType::IF))
        {
            consume();
            elseBranch = parseIfStmt();
        }
        else
        {
            match(TokenType::COLON); // optional colon: "else:"
            skipNewlines();
            elseBranch = parseBodyOrStatement();
        }
    }
    return std::make_unique<ASTNode>(IfStmt{std::move(cond), std::move(then), std::move(elseBranch)}, ln);
}

ASTNodePtr Parser::parseWhileStmt()
{
    int ln = current().line;
    auto cond = parseExpr();
    match(TokenType::COLON); // optional Python-style colon
    skipNewlines();
    auto body = parseBodyOrStatement();
    return std::make_unique<ASTNode>(WhileStmt{std::move(cond), std::move(body)}, ln);
}

ASTNodePtr Parser::parseForStmt()
{
    int ln = current().line;
    auto var = expect(TokenType::IDENTIFIER, "Expected variable in for loop").value;
    expect(TokenType::IN, "Expected 'in'");
    auto iterable = parseExpr();
    match(TokenType::COLON); // optional Python-style colon
    skipNewlines();
    auto body = parseBodyOrStatement();
    return std::make_unique<ASTNode>(ForStmt{var, std::move(iterable), std::move(body)}, ln);
}

ASTNodePtr Parser::parseReturnStmt()
{
    int ln = current().line;
    ASTNodePtr val;
    if (!check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON) && !atEnd())
        val = parseExpr();
    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
        consume();
    return std::make_unique<ASTNode>(ReturnStmt{std::move(val)}, ln);
}

ASTNodePtr Parser::parsePrintStmt()
{
    int ln = current().line;
    bool newline = true;
    std::vector<ASTNodePtr> args;
    if (check(TokenType::LPAREN))
    {
        consume();
        while (!check(TokenType::RPAREN) && !atEnd())
        {
            args.push_back(parseExpr());
            if (!match(TokenType::COMMA))
                break;
        }
        expect(TokenType::RPAREN, "Expected ')'");
    }
    else
    {
        args.push_back(parseExpr());
        while (match(TokenType::COMMA))
            args.push_back(parseExpr());
    }
    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
        consume();
    // Always emit PrintStmt — execPrint handles both python-style and printf-style at runtime
    return std::make_unique<ASTNode>(PrintStmt{std::move(args), newline}, ln);
}

ASTNodePtr Parser::parseInputStmt()
{
    // Handles:
    //   scanf("%d", &var)     — C-style: format string + address-of target
    //   input("prompt", var)  — prompt + target variable
    //   input(var)            — just target variable
    int ln = current().line;
    std::string target;
    ASTNodePtr prompt;

    if (check(TokenType::LPAREN))
    {
        consume();
        if (check(TokenType::STRING))
        {
            // First arg is a string: either a format string or a prompt
            auto fmtTok = current();
            consume();
            if (match(TokenType::COMMA))
            {
                // scanf("%d", &var) or input("prompt", var)
                prompt = std::make_unique<ASTNode>(StringLiteral{fmtTok.value}, ln);
                if (check(TokenType::BIT_AND))
                    consume(); // strip optional &
                target = expect(TokenType::IDENTIFIER, "Expected variable name after ','").value;
            }
            else
            {
                // input("prompt") — prompt only, no target
                prompt = std::make_unique<ASTNode>(StringLiteral{fmtTok.value}, ln);
                target = "";
            }
        }
        else
        {
            // input(&var) or input(var)
            if (check(TokenType::BIT_AND))
                consume();
            target = expect(TokenType::IDENTIFIER, "Expected variable name").value;
        }
        expect(TokenType::RPAREN, "Expected ')'");
    }
    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
        consume();
    return std::make_unique<ASTNode>(InputStmt{target, std::move(prompt)}, ln);
}

ASTNodePtr Parser::parseCoutStmt()
{
    // cout << expr1 << expr2 << endl;
    // We must NOT call parseExpr() here because parseShift() inside it would
    // greedily consume << as a bitwise-shift operator.
    // Instead we call parseAddSub() — one level below shift — so each <<
    // stays available as the stream-insertion separator.
    int ln = current().line;
    std::vector<ASTNodePtr> args;
    bool newline = false;

    while (check(TokenType::LSHIFT))
    {
        consume(); // eat <<

        // "endl" triggers a newline (no value pushed)
        if (check(TokenType::IDENTIFIER) && current().value == "endl")
        {
            consume();
            newline = true;
            continue;
        }

        // Parse the next segment at add/sub precedence so << isn't swallowed
        auto expr = parseAddSub();

        // If the segment is a string ending with \n, keep it as-is (contains the newline)
        // Only treat a bare "\n" string as endl
        if (expr->is<StringLiteral>() && expr->as<StringLiteral>().value == "\n")
            newline = true;
        else
            args.push_back(std::move(expr));
    }

    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
        consume();

    return std::make_unique<ASTNode>(PrintStmt{std::move(args), newline}, ln);
}

ASTNodePtr Parser::parseCinStmt()
{
    // cin >> var1 >> var2;
    // Each >> reads one variable from stdin
    int ln = current().line;

    // Collect all target variable names
    std::vector<std::string> targets;
    while (check(TokenType::RSHIFT))
    {
        consume(); // eat >>
        if (check(TokenType::BIT_AND))
            consume(); // strip optional &
        auto name = expect(TokenType::IDENTIFIER, "Expected variable name after '>>'").value;
        targets.push_back(name);
    }
    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
        consume();

    // Emit one InputStmt per target (no prompt, auto-detect type)
    // Wrap multiple targets in a block
    if (targets.size() == 1)
        return std::make_unique<ASTNode>(InputStmt{targets[0], nullptr}, ln);

    BlockStmt block;
    for (auto &t : targets)
        block.statements.push_back(std::make_unique<ASTNode>(InputStmt{t, nullptr}, ln));
    return std::make_unique<ASTNode>(std::move(block), ln);
}

ASTNodePtr Parser::parseImportStmt()
{
    int ln = current().line;
    auto mod = expect(TokenType::IDENTIFIER, "Expected module name").value;
    std::string alias;
    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
        consume();
    return std::make_unique<ASTNode>(ImportStmt{mod, alias}, ln);
}

ASTNodePtr Parser::parseExprStmt()
{
    int ln = current().line;
    auto expr = parseExpr();
    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
        consume();
    return std::make_unique<ASTNode>(ExprStmt{std::move(expr)}, ln);
}

// ─── Expression Parsing (Pratt precedence) ───────────────────────────────────

ASTNodePtr Parser::parseExpr() { return parseAssignment(); }

ASTNodePtr Parser::parseAssignment()
{
    int ln = current().line;
    auto left = parseOr();
    // Ternary: condition ? thenExpr : elseExpr
    if (check(TokenType::QUESTION))
    {
        consume();
        auto thenExpr = parseExpr();
        expect(TokenType::COLON, "Expected ':' in ternary expression");
        auto elseExpr = parseExpr();
        return std::make_unique<ASTNode>(TernaryExpr{std::move(left), std::move(thenExpr), std::move(elseExpr)}, ln);
    }
    if (check(TokenType::ASSIGN) || check(TokenType::PLUS_ASSIGN) ||
        check(TokenType::MINUS_ASSIGN) || check(TokenType::STAR_ASSIGN) ||
        check(TokenType::SLASH_ASSIGN))
    {
        auto op = consume().value;
        auto right = parseAssignment();
        return std::make_unique<ASTNode>(AssignExpr{op, std::move(left), std::move(right)}, ln);
    }
    return left;
}

ASTNodePtr Parser::parseOr()
{
    auto left = parseAnd();
    while (check(TokenType::OR) || check(TokenType::OR_OR))
    {
        int ln = current().line;
        consume(); // eat 'or' or '||'
        auto right = parseAnd();
        left = std::make_unique<ASTNode>(BinaryExpr{"or", std::move(left), std::move(right)}, ln);
    }
    return left;
}

ASTNodePtr Parser::parseAnd()
{
    auto left = parseBitwise();
    while (check(TokenType::AND) || check(TokenType::AND_AND))
    {
        int ln = current().line;
        consume(); // eat 'and' or '&&'
        auto right = parseBitwise();
        left = std::make_unique<ASTNode>(BinaryExpr{"and", std::move(left), std::move(right)}, ln);
    }
    return left;
}

ASTNodePtr Parser::parseBitwise()
{
    auto left = parseEquality();
    while (check(TokenType::BIT_AND) || check(TokenType::BIT_OR) || check(TokenType::BIT_XOR))
    {
        int ln = current().line;
        auto op = consume().value;
        auto right = parseEquality();
        left = std::make_unique<ASTNode>(BinaryExpr{op, std::move(left), std::move(right)}, ln);
    }
    return left;
}

ASTNodePtr Parser::parseEquality()
{
    auto left = parseComparison();
    while (check(TokenType::EQ) || check(TokenType::NEQ))
    {
        int ln = current().line;
        auto op = consume().value;
        auto right = parseComparison();
        left = std::make_unique<ASTNode>(BinaryExpr{op, std::move(left), std::move(right)}, ln);
    }
    return left;
}

ASTNodePtr Parser::parseComparison()
{
    auto left = parseShift();
    while (check(TokenType::LT) || check(TokenType::GT) || check(TokenType::LTE) || check(TokenType::GTE))
    {
        int ln = current().line;
        auto op = consume().value;
        auto right = parseShift();
        left = std::make_unique<ASTNode>(BinaryExpr{op, std::move(left), std::move(right)}, ln);
    }
    return left;
}

ASTNodePtr Parser::parseShift()
{
    auto left = parseAddSub();
    while (check(TokenType::LSHIFT) || check(TokenType::RSHIFT))
    {
        int ln = current().line;
        auto op = consume().value;
        auto right = parseAddSub();
        left = std::make_unique<ASTNode>(BinaryExpr{op, std::move(left), std::move(right)}, ln);
    }
    return left;
}

ASTNodePtr Parser::parseAddSub()
{
    auto left = parseMulDiv();
    while (check(TokenType::PLUS) || check(TokenType::MINUS))
    {
        int ln = current().line;
        auto op = consume().value;
        auto right = parseMulDiv();
        left = std::make_unique<ASTNode>(BinaryExpr{op, std::move(left), std::move(right)}, ln);
    }
    return left;
}

ASTNodePtr Parser::parseMulDiv()
{
    auto left = parsePower();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT))
    {
        int ln = current().line;
        auto op = consume().value;
        auto right = parsePower();
        left = std::make_unique<ASTNode>(BinaryExpr{op, std::move(left), std::move(right)}, ln);
    }
    return left;
}

ASTNodePtr Parser::parsePower()
{
    auto left = parseUnary();
    if (check(TokenType::POWER))
    {
        int ln = current().line;
        consume();
        auto right = parsePower(); // right-associative
        return std::make_unique<ASTNode>(BinaryExpr{"**", std::move(left), std::move(right)}, ln);
    }
    return left;
}

ASTNodePtr Parser::parseUnary()
{
    int ln = current().line;
    // Prefix ++ and --
    if (check(TokenType::PLUS_PLUS))
    {
        consume();
        auto operand = parseUnary();
        auto one = std::make_unique<ASTNode>(NumberLiteral{1.0}, ln);
        return std::make_unique<ASTNode>(AssignExpr{"+=", std::move(operand), std::move(one)}, ln);
    }
    if (check(TokenType::MINUS_MINUS))
    {
        consume();
        auto operand = parseUnary();
        auto one = std::make_unique<ASTNode>(NumberLiteral{1.0}, ln);
        return std::make_unique<ASTNode>(AssignExpr{"-=", std::move(operand), std::move(one)}, ln);
    }
    if (check(TokenType::MINUS))
    {
        consume();
        return std::make_unique<ASTNode>(UnaryExpr{"-", parseUnary()}, ln);
    }
    if (check(TokenType::NOT))
    {
        consume();
        return std::make_unique<ASTNode>(UnaryExpr{"not", parseUnary()}, ln);
    }
    if (check(TokenType::BIT_NOT))
    {
        consume();
        return std::make_unique<ASTNode>(UnaryExpr{"~", parseUnary()}, ln);
    }
    // C-style address-of: &var — silently strip & and treat as the variable itself
    if (check(TokenType::BIT_AND))
    {
        consume();
        return parseUnary();
    }
    return parsePostfix();
}

ASTNodePtr Parser::parsePostfix()
{
    auto expr = parsePrimary();
    while (true)
    {
        int ln = current().line;
        // Postfix ++ and --
        if (check(TokenType::PLUS_PLUS))
        {
            consume();
            // x++ is sugar for x += 1
            auto one = std::make_unique<ASTNode>(NumberLiteral{1.0}, ln);
            expr = std::make_unique<ASTNode>(AssignExpr{"+=", std::move(expr), std::move(one)}, ln);
        }
        else if (check(TokenType::MINUS_MINUS))
        {
            consume();
            // x-- is sugar for x -= 1
            auto one = std::make_unique<ASTNode>(NumberLiteral{1.0}, ln);
            expr = std::make_unique<ASTNode>(AssignExpr{"-=", std::move(expr), std::move(one)}, ln);
        }
        else if (check(TokenType::LPAREN))
        {
            auto args = parseArgList();
            expr = std::make_unique<ASTNode>(CallExpr{std::move(expr), std::move(args)}, ln);
        }
        else if (check(TokenType::LBRACKET))
        {
            consume();
            auto idx = parseExpr();
            expect(TokenType::RBRACKET, "Expected ']'");
            expr = std::make_unique<ASTNode>(IndexExpr{std::move(expr), std::move(idx)}, ln);
        }
        else if (check(TokenType::DOT))
        {
            consume();
            auto mem = expect(TokenType::IDENTIFIER, "Expected member name").value;
            if (check(TokenType::LPAREN))
            {
                auto memExpr = std::make_unique<ASTNode>(MemberExpr{std::move(expr), mem}, ln);
                auto args = parseArgList();
                expr = std::make_unique<ASTNode>(CallExpr{std::move(memExpr), std::move(args)}, ln);
            }
            else
            {
                expr = std::make_unique<ASTNode>(MemberExpr{std::move(expr), mem}, ln);
            }
        }
        else
            break;
    }
    return expr;
}

ASTNodePtr Parser::parsePrimary()
{
    int ln = current().line;
    auto &tok = current();

    if (tok.type == TokenType::NUMBER)
    {
        double v;
        if (tok.value.size() > 1 && tok.value[1] == 'x')
            v = (double)std::stoull(tok.value, nullptr, 16);
        else
            v = std::stod(tok.value);
        consume();
        return std::make_unique<ASTNode>(NumberLiteral{v}, ln);
    }
    if (tok.type == TokenType::STRING)
    {
        auto s = tok.value;
        consume();
        return std::make_unique<ASTNode>(StringLiteral{s}, ln);
    }
    if (tok.type == TokenType::BOOL_TRUE)
    {
        consume();
        return std::make_unique<ASTNode>(BoolLiteral{true}, ln);
    }
    if (tok.type == TokenType::BOOL_FALSE)
    {
        consume();
        return std::make_unique<ASTNode>(BoolLiteral{false}, ln);
    }
    if (tok.type == TokenType::NIL)
    {
        consume();
        return std::make_unique<ASTNode>(NilLiteral{}, ln);
    }

    if (tok.type == TokenType::LBRACKET)
        return parseArrayLiteral();
    if (tok.type == TokenType::LBRACE)
        return parseDictLiteral();

    if (tok.type == TokenType::FN || tok.type == TokenType::FUNCTION || tok.type == TokenType::DEF)
    {
        consume();
        return parseLambda();
    }

    if (tok.type == TokenType::LPAREN)
    {
        int ln = tok.line;
        consume();
        skipNewlines();

        // Check for arrow function: () => or (x, y) =>
        // We need to speculatively collect identifiers separated by commas
        // If we see RPAREN then ARROW it's an arrow function param list
        std::vector<std::string> arrowParams;
        bool isArrow = false;
        size_t savedPos = pos;

        // Try to parse as param list: only identifiers and commas allowed
        bool valid = true;
        std::vector<std::string> tryParams;
        size_t tryPos = pos; // pos is after '('
        // peek without consuming
        {
            size_t p = pos;
            // empty params: ()
            while (p < tokens.size() && tokens[p].type == TokenType::NEWLINE)
                ++p;
            if (tokens[p].type == TokenType::RPAREN)
            {
                // () — could be arrow
                size_t after = p + 1;
                while (after < tokens.size() && tokens[after].type == TokenType::NEWLINE)
                    ++after;
                if (tokens[after].type == TokenType::FAT_ARROW)
                    isArrow = true;
            }
            else
            {
                // Try collecting identifiers
                while (p < tokens.size() && tokens[p].type == TokenType::IDENTIFIER)
                {
                    tryParams.push_back(tokens[p].value);
                    ++p;
                    while (p < tokens.size() && tokens[p].type == TokenType::NEWLINE)
                        ++p;
                    if (tokens[p].type == TokenType::COMMA)
                    {
                        ++p;
                        continue;
                    }
                    if (tokens[p].type == TokenType::RPAREN)
                    {
                        size_t after = p + 1;
                        while (after < tokens.size() && tokens[after].type == TokenType::NEWLINE)
                            ++after;
                        if (tokens[after].type == TokenType::FAT_ARROW)
                            isArrow = true;
                    }
                    break;
                }
            }
        }

        if (isArrow)
        {
            // Consume the params and closing paren
            while (!check(TokenType::RPAREN) && !atEnd())
            {
                if (isCTypeKeyword(current().type))
                    consume(); // skip type hints
                if (check(TokenType::IDENTIFIER))
                    arrowParams.push_back(consume().value);
                match(TokenType::COMMA);
            }
            expect(TokenType::RPAREN, "Expected ')'");
            return parseArrowFunction(std::move(arrowParams), ln);
        }

        // Normal parenthesised expression
        auto expr = parseExpr();
        skipNewlines();
        expect(TokenType::RPAREN, "Expected ')'");
        return expr;
    }

    // Single-param arrow without parens: x => expr
    if (tok.type == TokenType::IDENTIFIER)
    {
        // peek ahead for =>
        size_t j = pos + 1;
        while (j < tokens.size() && tokens[j].type == TokenType::NEWLINE)
            ++j;
        if (j < tokens.size() && tokens[j].type == TokenType::FAT_ARROW)
        {
            std::string paramName = tok.value;
            consume(); // eat identifier
            return parseArrowFunction({paramName}, tok.line);
        }
        auto name = tok.value;
        consume();
        return std::make_unique<ASTNode>(Identifier{name}, ln);
    }

    // C-type keywords used as variable names (e.g. "string = 'hello'", "double = 3.14")
    if (isCTypeKeyword(tok.type))
    {
        auto name = tok.value;
        consume();
        return std::make_unique<ASTNode>(Identifier{name}, ln);
    }

    throw ParseError("Unexpected token: '" + tok.value + "'", tok.line, tok.col);
}

ASTNodePtr Parser::parseArrayLiteral()
{
    int ln = current().line;
    expect(TokenType::LBRACKET, "Expected '['");
    skipNewlines();
    ArrayLiteral arr;
    while (!check(TokenType::RBRACKET) && !atEnd())
    {
        arr.elements.push_back(parseExpr());
        skipNewlines();
        if (!match(TokenType::COMMA))
            break;
        skipNewlines();
    }
    expect(TokenType::RBRACKET, "Expected ']'");
    return std::make_unique<ASTNode>(std::move(arr), ln);
}

ASTNodePtr Parser::parseDictLiteral()
{
    int ln = current().line;
    expect(TokenType::LBRACE, "Expected '{'");
    skipNewlines();
    DictLiteral dict;
    while (!check(TokenType::RBRACE) && !atEnd())
    {
        auto key = parseExpr();
        expect(TokenType::COLON, "Expected ':'");
        auto val = parseExpr();
        dict.pairs.emplace_back(std::move(key), std::move(val));
        skipNewlines();
        if (!match(TokenType::COMMA))
            break;
        skipNewlines();
    }
    expect(TokenType::RBRACE, "Expected '}'");
    return std::make_unique<ASTNode>(std::move(dict), ln);
}

ASTNodePtr Parser::parseLambda()
{
    // Called after consuming fn / function / def keyword (anonymous form)
    int ln = current().line;
    auto params = parseParamList();
    match(TokenType::COLON); // Python: def style
    if (!match(TokenType::FAT_ARROW))
        match(TokenType::ARROW); // JS => or Quantum ->
    skipNewlines();
    auto body = parseBlock();
    return std::make_unique<ASTNode>(LambdaExpr{std::move(params), std::move(body)}, ln);
}

// Arrow function: already consumed '(' params ')' as an expression,
// then caller detects '=>' and calls this.
ASTNodePtr Parser::parseArrowFunction(std::vector<std::string> params, int ln)
{
    // consume => (FAT_ARROW) or -> (ARROW)
    if (!match(TokenType::FAT_ARROW) && !match(TokenType::ARROW))
        throw ParseError("Expected '=>' or '->'", current().line, current().col);
    skipNewlines();
    // Body can be a block OR a single expression (implicit return)
    if (check(TokenType::LBRACE) || check(TokenType::INDENT))
    {
        auto body = parseBlock();
        return std::make_unique<ASTNode>(LambdaExpr{std::move(params), std::move(body)}, ln);
    }
    // Expression body: (x) => x * 2  →  wrap in implicit return block
    auto expr = parseExpr();
    int eln = expr->line;
    auto retStmt = std::make_unique<ASTNode>(ReturnStmt{std::move(expr)}, eln);
    BlockStmt block;
    block.statements.push_back(std::move(retStmt));
    auto body = std::make_unique<ASTNode>(std::move(block), ln);
    return std::make_unique<ASTNode>(LambdaExpr{std::move(params), std::move(body)}, ln);
}

std::vector<ASTNodePtr> Parser::parseArgList()
{
    expect(TokenType::LPAREN, "Expected '('");
    std::vector<ASTNodePtr> args;
    skipNewlines();
    while (!check(TokenType::RPAREN) && !atEnd())
    {
        args.push_back(parseExpr());
        skipNewlines();
        if (!match(TokenType::COMMA))
            break;
        skipNewlines();
    }
    expect(TokenType::RPAREN, "Expected ')'");
    return args;
}

std::vector<std::string> Parser::parseParamList()
{
    expect(TokenType::LPAREN, "Expected '('");
    std::vector<std::string> params;
    while (!check(TokenType::RPAREN) && !atEnd())
    {
        // C-style: "int x" — type keyword before name
        if (isCTypeKeyword(current().type))
        {
            consume();
            while (isCTypeKeyword(current().type))
                consume(); // multi-word types
        }

        params.push_back(expect(TokenType::IDENTIFIER, "Expected parameter name").value);

        // Python-style annotation: "x: int" or "x: str" — skip ": type"
        if (check(TokenType::COLON))
        {
            consume(); // eat :
            // consume the type — could be identifier or type keyword
            if (check(TokenType::IDENTIFIER) || isCTypeKeyword(current().type))
                consume();
        }

        // Default value: "x = 5" or "x: int = 5" — skip "= expr"
        if (check(TokenType::ASSIGN))
        {
            consume(); // eat =
            // consume tokens until comma or closing paren
            int depth = 0;
            while (!atEnd())
            {
                if (check(TokenType::LPAREN) || check(TokenType::LBRACKET))
                    depth++;
                else if (check(TokenType::RPAREN) || check(TokenType::RBRACKET))
                {
                    if (depth == 0)
                        break;
                    depth--;
                }
                else if (check(TokenType::COMMA) && depth == 0)
                    break;
                consume();
            }
        }

        if (!match(TokenType::COMMA))
            break;
    }
    expect(TokenType::RPAREN, "Expected ')'");
    return params;
}

bool Parser::isCTypeKeyword(TokenType t) const
{
    switch (t)
    {
    case TokenType::TYPE_INT:
    case TokenType::TYPE_FLOAT:
    case TokenType::TYPE_DOUBLE:
    case TokenType::TYPE_CHAR:
    case TokenType::TYPE_STRING:
    case TokenType::TYPE_BOOL:
    case TokenType::TYPE_VOID:
    case TokenType::TYPE_LONG:
    case TokenType::TYPE_SHORT:
    case TokenType::TYPE_UNSIGNED:
        return true;
    default:
        return false;
    }
}

ASTNodePtr Parser::parseCTypeVarDecl(const std::string &typeHint)
{
    int ln = current().line;
    auto nameToken = expect(TokenType::IDENTIFIER, "Expected variable name after type");
    ASTNodePtr init;
    if (match(TokenType::ASSIGN))
        init = parseExpr();
    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
        consume();
    return std::make_unique<ASTNode>(VarDecl{false, nameToken.value, std::move(init), typeHint}, ln);
}