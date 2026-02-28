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

    // Skip Python-style decorators (e.g. @property, @dataclass)
    while (check(TokenType::DECORATOR))
    {
        consume(); // eat @
        if (check(TokenType::IDENTIFIER))
        {
            consume(); // eat decorator name
            // Optional call parens e.g. @decorator(args)
            if (check(TokenType::LPAREN))
            {
                consume(); // eat (
                int depth = 1;
                while (!atEnd() && depth > 0)
                {
                    if (check(TokenType::LPAREN))
                        depth++;
                    else if (check(TokenType::RPAREN))
                        depth--;
                    consume();
                }
            }
        }
        skipNewlines();
    }

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
        if (current().type == TokenType::IDENTIFIER)
            return parseFunctionDecl();
        auto lam = parseLambda();
        while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
            consume();
        return std::make_unique<ASTNode>(ExprStmt{std::move(lam)}, lam->line);
    }
    case TokenType::CLASS:
    {
        consume();
        return parseClassDecl();
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
    case TokenType::IMPORT:
    case TokenType::FROM:
    {
        // For FROM, we don't consume here because parseImportStmt needs to know if it's 'from' or 'import'
        // But the current implementation consumes. Let's consume and pass a bool or just let parseImportStmt do it.
        // Actually, the current code just calls parseImportStmt().
        bool isFrom = (current().type == TokenType::FROM);
        consume();
        return parseImportStmt(isFrom);
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
    case TokenType::RAISE:
    {
        consume();
        ASTNodePtr val;
        if (!check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON) && !atEnd())
            val = parseExpr();
        while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
            consume();
        return std::make_unique<ASTNode>(RaiseStmt{std::move(val)}, ln);
    }
    case TokenType::TRY:
    {
        consume();
        match(TokenType::COLON);
        skipNewlines();
        auto body = parseBlock();
        TryStmt ts;
        ts.body = std::move(body);
        skipNewlines(); // skip blank lines between try body and except/finally
        // Parse except/catch clauses
        while (check(TokenType::EXCEPT))
        {
            consume(); // eat 'except'
            ExceptClause clause;
            // Optional error type: except ValueError or except (ValueError, TypeError)
            if (!check(TokenType::COLON) && !check(TokenType::NEWLINE) && !check(TokenType::INDENT))
            {
                // skip optional '(' around type list
                match(TokenType::LPAREN);
                if (check(TokenType::IDENTIFIER))
                    clause.errorType = consume().value;
                else if (isCTypeKeyword(current().type))
                    clause.errorType = consume().value;
                // skip extra types: except (A, B) — just use first
                while (check(TokenType::COMMA))
                {
                    consume();
                    if (check(TokenType::IDENTIFIER))
                        consume();
                }
                match(TokenType::RPAREN);
                // optional 'as e'
                if (check(TokenType::AS))
                {
                    consume();
                    if (check(TokenType::IDENTIFIER))
                        clause.alias = consume().value;
                }
            }
            match(TokenType::COLON);
            skipNewlines();
            clause.body = parseBlock();
            ts.handlers.push_back(std::move(clause));
            skipNewlines();
        }
        // Optional finally
        if (check(TokenType::FINALLY))
        {
            consume();
            match(TokenType::COLON);
            skipNewlines();
            ts.finallyBody = parseBlock();
        }
        return std::make_unique<ASTNode>(std::move(ts), ln);
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
    // Helper: tokens that terminate a block because they belong to the parent stmt
    auto isBlockTerminator = [&]() -> bool
    {
        switch (current().type)
        {
        case TokenType::EXCEPT:
        case TokenType::FINALLY:
        case TokenType::ELSE:
        case TokenType::ELIF:
            return true;
        default:
            return false;
        }
    };

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
        while (!check(TokenType::DEDENT) && !atEnd() && !isBlockTerminator())
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
    // Accept plain identifiers OR type keywords used as variable names
    // e.g. "let char = ..." where 'char' is TYPE_CHAR
    std::string name;
    if (check(TokenType::IDENTIFIER))
        name = consume().value;
    else if (isCTypeKeyword(current().type))
        name = consume().value;
    else
        throw ParseError("Expected variable name (got '" + current().value + "')", current().line, current().col);

    ASTNodePtr init;
    if (match(TokenType::ASSIGN))
        init = parseExpr();
    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
        consume();
    return std::make_unique<ASTNode>(VarDecl{isConst, name, std::move(init), ""}, ln);
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

ASTNodePtr Parser::parseClassDecl()
{
    int ln = current().line;
    auto name = expect(TokenType::IDENTIFIER, "Expected class name").value;
    std::string base;

    // Python-style: class Foo(Bar): or class Foo(Bar, ABC):
    if (check(TokenType::LPAREN))
    {
        consume(); // eat '('
        // grab first base class name
        if (check(TokenType::IDENTIFIER))
            base = consume().value;
        else if (isCTypeKeyword(current().type))
            base = consume().value;
        // skip any additional bases: (A, B, C) — just use first
        while (check(TokenType::COMMA))
        {
            consume();
            if (check(TokenType::IDENTIFIER) || isCTypeKeyword(current().type))
                consume(); // discard extra bases
        }
        expect(TokenType::RPAREN, "Expected ')' after base class list");
    }
    // Quantum/JS-style: class Foo extends Bar
    else if (check(TokenType::EXTENDS))
    {
        consume();
        base = expect(TokenType::IDENTIFIER, "Expected base class name").value;
    }

    match(TokenType::COLON);
    skipNewlines();

    ClassDecl cd;
    cd.name = name;
    cd.base = base;

    auto parseClassBody = [&]()
    {
        skipNewlines();
        // Check for 'pass' which just means an empty body
        if (check(TokenType::IDENTIFIER) && current().value == "pass")
        {
            consume();
            skipNewlines();
            return;
        }

        while (!check(TokenType::RBRACE) && !check(TokenType::DEDENT) && !atEnd())
        {
            skipNewlines();
            if (check(TokenType::RBRACE) || check(TokenType::DEDENT))
                break;

            // Handle decorators on methods
            while (check(TokenType::DECORATOR))
            {
                consume(); // eat @
                if (check(TokenType::IDENTIFIER))
                {
                    consume(); // eat decorator name
                    if (check(TokenType::LPAREN))
                    {
                        consume(); // eat (
                        int depth = 1;
                        while (!atEnd() && depth > 0)
                        {
                            if (check(TokenType::LPAREN))
                                depth++;
                            else if (check(TokenType::RPAREN))
                                depth--;
                            consume();
                        }
                    }
                }
                skipNewlines();
            }

            bool isStatic = false;

            // Skip access modifiers, detect static
            while (check(TokenType::IDENTIFIER) &&
                   (current().value == "public" || current().value == "private" ||
                    current().value == "protected" || current().value == "static" ||
                    current().value == "async"))
            {
                if (current().value == "static")
                    isStatic = true;
                consume();
            }

            // Optional fn/def/function keyword
            if (check(TokenType::FN) || check(TokenType::DEF) || check(TokenType::FUNCTION))
                consume();

            // Handle ~ClassName destructor syntax (C++ style)
            if (check(TokenType::BIT_NOT))
            {
                consume(); // eat ~
                if (check(TokenType::IDENTIFIER))
                    consume(); // eat class name
                auto params = parseParamList();

                // Skip optional return type annotation: -> ReturnType
                if (check(TokenType::ARROW))
                {
                    consume();
                    while (!atEnd() && !check(TokenType::COLON) && !check(TokenType::LBRACE) && !check(TokenType::NEWLINE) && !check(TokenType::INDENT))
                        consume();
                }

                match(TokenType::COLON);
                skipNewlines();
                auto body = parseBlock();
                auto fn = std::make_unique<ASTNode>(
                    FunctionDecl{"__del__", std::move(params), std::move(body)}, ln);
                cd.methods.push_back(std::move(fn));
                skipNewlines();
                continue;
            }

            if (!check(TokenType::IDENTIFIER))
            {
                skipNewlines();
                // If it's a pass inside the class but not the first thing
                if (check(TokenType::IDENTIFIER) && current().value == "pass")
                {
                    consume();
                    skipNewlines();
                }
                else if (!check(TokenType::RBRACE) && !check(TokenType::DEDENT))
                {
                    consume(); // skip whatever garbage token this is to avoid infinite loops
                }
                continue;
            }
            std::string methodName = consume().value;

            // Normalize constructor names
            if (methodName == "constructor" || methodName == "__init__")
                methodName = "init";
            // Normalize destructor names
            if (methodName == "destructor")
                methodName = "__del__";
            // Normalize toString
            if (methodName == "toString" || methodName == "to_string" || methodName == "to_str")
                methodName = "__str__";

            auto params = parseParamList();

            // Skip optional return type annotation: -> ReturnType  (Python style)
            if (check(TokenType::ARROW))
            {
                consume(); // eat ->
                // consume tokens until we hit : or { or NEWLINE or INDENT
                while (!atEnd() && !check(TokenType::COLON) && !check(TokenType::LBRACE) && !check(TokenType::NEWLINE) && !check(TokenType::INDENT))
                    consume();
            }

            match(TokenType::COLON);
            skipNewlines();
            auto body = parseBlock();

            auto fn = std::make_unique<ASTNode>(
                FunctionDecl{methodName, std::move(params), std::move(body)}, ln);

            if (isStatic)
                cd.staticMethods.push_back(std::move(fn));
            else
                cd.methods.push_back(std::move(fn));
            skipNewlines();
        }
    };

    if (check(TokenType::LBRACE))
    {
        consume();
        parseClassBody();
        expect(TokenType::RBRACE, "Expected \'}\'");
    }
    else if (check(TokenType::INDENT))
    {
        consume();
        parseClassBody();
        if (check(TokenType::DEDENT))
            consume();
    }
    else if (check(TokenType::IDENTIFIER) && current().value == "pass")
    {
        // One-liner: class X: pass
        consume();
    }
    else if (check(TokenType::NEWLINE) || atEnd())
    {
        // Empty class with nothing but newlines (Python will fail but we can be lenient)
        skipNewlines();
    }
    else
        throw ParseError("Expected \'{\' or indented class body", current().line, current().col);

    return std::make_unique<ASTNode>(std::move(cd), ln);
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

    // C-style for: for (init; condition; post) { body }
    // Detected by: for ( ...
    if (check(TokenType::LPAREN))
    {
        consume(); // eat (

        // ── Init ──────────────────────────────────────────────────────────
        ASTNodePtr initNode;
        std::string forOfVar; // set if this turns out to be for-of/for-in
        if (!check(TokenType::SEMICOLON))
        {
            // let/const/type-keyword declaration OR plain expression
            if (check(TokenType::LET) || check(TokenType::CONST))
            {
                bool isConst = current().type == TokenType::CONST;
                consume();
                // Peek: if next is identifier/type-kw followed by 'in'/'of' → for-in/for-of
                size_t la = pos;
                if (la < tokens.size() &&
                    (tokens[la].type == TokenType::IDENTIFIER || isCTypeKeyword(tokens[la].type)))
                {
                    size_t la2 = la + 1;
                    if (la2 < tokens.size() &&
                        (tokens[la2].type == TokenType::IN || tokens[la2].type == TokenType::OF))
                    {
                        // for (let x of iterable) — treat as for-of
                        forOfVar = consume().value; // var name
                        consume();                  // eat 'in' or 'of'
                        auto iterable = parseExpr();
                        expect(TokenType::RPAREN, "Expected ')'");
                        match(TokenType::COLON);
                        skipNewlines();
                        auto body = parseBodyOrStatement();
                        return std::make_unique<ASTNode>(ForStmt{forOfVar, "", std::move(iterable), std::move(body)}, ln);
                    }
                }
                initNode = parseVarDecl(isConst);
            }
            else if (isCTypeKeyword(current().type))
            {
                size_t la = pos + 1;
                while (la < tokens.size() && isCTypeKeyword(tokens[la].type))
                    ++la;
                if (la < tokens.size() && tokens[la].type == TokenType::IDENTIFIER)
                {
                    auto hint = consume().value;
                    while (isCTypeKeyword(current().type))
                        hint += " " + consume().value;
                    initNode = parseCTypeVarDecl(hint);
                }
                else
                    initNode = std::make_unique<ASTNode>(ExprStmt{parseExpr()}, ln);
            }
            else
            {
                auto expr = parseExpr();
                initNode = std::make_unique<ASTNode>(ExprStmt{std::move(expr)}, ln);
            }
        }
        while (check(TokenType::SEMICOLON))
            consume();

        // ── Condition ─────────────────────────────────────────────────────
        ASTNodePtr condition;
        if (!check(TokenType::SEMICOLON))
            condition = parseExpr();
        else
            condition = std::make_unique<ASTNode>(BoolLiteral{true}, ln); // infinite if omitted
        while (check(TokenType::SEMICOLON))
            consume();

        // ── Post expression ───────────────────────────────────────────────
        ASTNodePtr postNode;
        if (!check(TokenType::RPAREN))
        {
            auto expr = parseExpr();
            postNode = std::make_unique<ASTNode>(ExprStmt{std::move(expr)}, ln);
        }
        expect(TokenType::RPAREN, "Expected ')'");
        match(TokenType::COLON);
        skipNewlines();

        // ── Body ──────────────────────────────────────────────────────────
        auto rawBody = parseBodyOrStatement();

        // Append post expression at end of body block
        BlockStmt loopBlock;
        // Copy existing body statements
        for (auto &s : rawBody->as<BlockStmt>().statements)
            loopBlock.statements.push_back(std::move(const_cast<ASTNodePtr &>(s)));
        if (postNode)
            loopBlock.statements.push_back(std::move(postNode));
        auto loopBody = std::make_unique<ASTNode>(std::move(loopBlock), ln);

        // Build: while (condition) { body; post }
        auto whileNode = std::make_unique<ASTNode>(
            WhileStmt{std::move(condition), std::move(loopBody)}, ln);

        // Wrap in block: { init; while(...){...} }
        BlockStmt outer;
        if (initNode)
            outer.statements.push_back(std::move(initNode));
        outer.statements.push_back(std::move(whileNode));
        return std::make_unique<ASTNode>(std::move(outer), ln);
    }

    // Python / Quantum / JS for-in / for-of
    // Supports tuple unpacking: for k, v in dict.items()
    // Also accepts type keywords as variable names
    auto readLoopVar = [&]() -> std::string
    {
        if (check(TokenType::IDENTIFIER))
            return consume().value;
        if (isCTypeKeyword(current().type))
            return consume().value;
        throw ParseError("Expected variable in for loop (got '" + current().value + "')", current().line, current().col);
        return "";
    };

    std::string var = readLoopVar();
    std::string var2; // second variable for tuple unpacking

    // Tuple unpacking: for k, v in ...
    if (check(TokenType::COMMA))
    {
        consume();
        var2 = readLoopVar();
    }

    // Accept both 'in' and 'of' (JavaScript for...of)
    if (!match(TokenType::IN) && !match(TokenType::OF))
        throw ParseError("Expected 'in' or 'of' in for loop", current().line, current().col);

    auto iterable = parseExpr();
    match(TokenType::COLON);
    skipNewlines();
    auto body = parseBodyOrStatement();
    return std::make_unique<ASTNode>(ForStmt{var, var2, std::move(iterable), std::move(body)}, ln);
}

ASTNodePtr Parser::parseReturnStmt()
{
    int ln = current().line;
    ASTNodePtr val;
    if (!check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON) && !atEnd())
    {
        val = parseExpr();
        // Tuple return: return a, b  or  return a, b, c
        if (check(TokenType::COMMA))
        {
            TupleLiteral tup;
            tup.elements.push_back(std::move(val));
            while (match(TokenType::COMMA))
            {
                if (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON) || atEnd())
                    break;
                tup.elements.push_back(parseExpr());
            }
            val = std::make_unique<ASTNode>(std::move(tup), ln);
        }
    }
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

ASTNodePtr Parser::parseImportStmt(bool isFrom)
{
    int ln = current().line;
    ImportStmt stmt;

    if (isFrom)
    {
        // from module.sub import A, B
        // Actually, we'll just read an identifier (maybe with dots in the future)
        stmt.module = expect(TokenType::IDENTIFIER, "Expected module name after 'from'").value;
        expect(TokenType::IMPORT, "Expected 'import' after module name in 'from' statement");

        do
        {
            ImportStmt::Item item;
            item.name = expect(TokenType::IDENTIFIER, "Expected item name to import").value;
            if (match(TokenType::AS))
            {
                item.alias = expect(TokenType::IDENTIFIER, "Expected alias after 'as'").value;
            }
            stmt.imports.push_back(item);
        } while (match(TokenType::COMMA));
    }
    else
    {
        // import A as B, C
        stmt.module = ""; // No base module, importing directly
        do
        {
            ImportStmt::Item item;
            item.name = expect(TokenType::IDENTIFIER, "Expected module name to import").value;
            if (match(TokenType::AS))
            {
                item.alias = expect(TokenType::IDENTIFIER, "Expected alias after 'as'").value;
            }
            stmt.imports.push_back(item);
        } while (match(TokenType::COMMA));
    }

    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
        consume();
    return std::make_unique<ASTNode>(std::move(stmt), ln);
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
    // Python inline ternary: expr IF condition ELSE other_expr
    // e.g.  high = number if number > 1 else 1
    if (check(TokenType::IF))
    {
        // Lookahead to ensure this is actually a ternary, not a list comprehension filter:
        // A ternary MUST have an 'else' somewhere before a closing bracket/paren/newline.
        bool hasElse = false;
        int checkPos = pos + 1;
        int depth = 0;
        while (checkPos < tokens.size())
        {
            TokenType t = tokens[checkPos].type;
            if (t == TokenType::LPAREN || t == TokenType::LBRACKET || t == TokenType::LBRACE)
                depth++;
            else if (t == TokenType::RPAREN || t == TokenType::RBRACKET || t == TokenType::RBRACE)
            {
                if (depth == 0)
                    break;
                depth--;
            }
            else if (depth == 0 && t == TokenType::ELSE)
            {
                hasElse = true;
                break;
            }
            else if (depth == 0 && (t == TokenType::NEWLINE || t == TokenType::SEMICOLON || t == TokenType::COMMA))
            {
                break;
            }
            checkPos++;
        }

        if (hasElse)
        {
            consume(); // eat 'if'
            auto condition = parseOr();
            expect(TokenType::ELSE, "Expected 'else' in Python ternary expression");
            auto elseExpr = parseAssignment();
            return std::make_unique<ASTNode>(TernaryExpr{std::move(condition), std::move(left), std::move(elseExpr)}, ln);
        }
        // If there's no 'else', it's likely a list comprehension filter like `[x for x in lst if x > 0]`
        // which will be handled by parseExpr/parseListComp, so we just return `left`.
    }
    // JS/C ternary: condition ? thenExpr : elseExpr
    if (check(TokenType::QUESTION))
    {
        consume();
        auto thenExpr = parseExpr();
        expect(TokenType::COLON, "Expected ':' in ternary expression");
        auto elseExpr = parseExpr();
        return std::make_unique<ASTNode>(TernaryExpr{std::move(left), std::move(thenExpr), std::move(elseExpr)}, ln);
    }

    // Tuple-unpacking assignment: a, b, c = expr
    // ONLY activate when we can confirm via non-consuming lookahead that the
    // pattern is:  IDENT , IDENT , ... IDENT =
    // This prevents false-positives inside argument lists like f(a, b).
    if (check(TokenType::COMMA) && left->is<Identifier>())
    {
        // Lookahead: scan forward to confirm all commas are followed by
        // identifiers and the sequence ends with '='
        bool isUnpack = false;
        {
            size_t scan = pos; // points at the first COMMA
            while (scan < tokens.size() && tokens[scan].type == TokenType::COMMA)
            {
                ++scan; // skip comma
                while (scan < tokens.size() && tokens[scan].type == TokenType::NEWLINE)
                    ++scan;
                // must be an identifier
                if (scan >= tokens.size() || tokens[scan].type != TokenType::IDENTIFIER)
                    break;
                ++scan; // skip identifier
                while (scan < tokens.size() && tokens[scan].type == TokenType::NEWLINE)
                    ++scan;
                // if next is '=' (not '=='), this is a tuple-unpack assignment
                if (scan < tokens.size() && tokens[scan].type == TokenType::ASSIGN)
                {
                    isUnpack = true;
                    break;
                }
                // if next is another comma, keep scanning; otherwise stop
                if (scan >= tokens.size() || tokens[scan].type != TokenType::COMMA)
                    break;
            }
        }

        if (isUnpack)
        {
            std::vector<std::string> targets;
            targets.push_back(left->as<Identifier>().name);
            while (match(TokenType::COMMA))
            {
                skipNewlines();
                if (check(TokenType::IDENTIFIER))
                    targets.push_back(consume().value);
                else
                    break;
            }
            expect(TokenType::ASSIGN, "Expected '=' in tuple unpacking");
            auto right = parseAssignment();
            TupleLiteral lhsTuple;
            for (auto &t : targets)
                lhsTuple.elements.push_back(std::make_unique<ASTNode>(Identifier{t}, ln));
            auto lhsNode = std::make_unique<ASTNode>(std::move(lhsTuple), ln);
            return std::make_unique<ASTNode>(AssignExpr{"unpack", std::move(lhsNode), std::move(right)}, ln);
        }
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
    while (check(TokenType::EQ) || check(TokenType::NEQ) || check(TokenType::STRICT_EQ) || check(TokenType::STRICT_NEQ))
    {
        int ln = current().line;
        auto op = consume();
        // Treat === as == and !== as != (Quantum is dynamically typed)
        std::string opStr = (op.type == TokenType::STRICT_EQ) ? "==" : (op.type == TokenType::STRICT_NEQ) ? "!="
                                                                                                          : op.value;
        auto right = parseComparison();
        left = std::make_unique<ASTNode>(BinaryExpr{opStr, std::move(left), std::move(right)}, ln);
    }
    return left;
}

ASTNodePtr Parser::parseComparison()
{
    auto left = parseShift();
    while (check(TokenType::LT) || check(TokenType::GT) || check(TokenType::LTE) || check(TokenType::GTE) || check(TokenType::IN) || check(TokenType::NOT))
    {
        int ln = current().line;

        // 'not in' — two-token operator
        if (check(TokenType::NOT))
        {
            consume(); // eat 'not'
            if (!match(TokenType::IN))
                throw ParseError("Expected 'in' after 'not'", current().line, current().col);
            auto right = parseShift();
            left = std::make_unique<ASTNode>(BinaryExpr{"not in", std::move(left), std::move(right)}, ln);
            continue;
        }

        // 'in' — membership test
        if (check(TokenType::IN))
        {
            consume();
            auto right = parseShift();
            left = std::make_unique<ASTNode>(BinaryExpr{"in", std::move(left), std::move(right)}, ln);
            continue;
        }

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
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT) || check(TokenType::FLOOR_DIV))
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
        // Peek past newlines to support method chaining across lines:
        //   obj
        //     .filter(...)
        //     .map(...)
        size_t savedPos = pos;
        while (check(TokenType::NEWLINE))
            consume();
        if (!check(TokenType::DOT) && !check(TokenType::LBRACKET) && !check(TokenType::LPAREN) && !check(TokenType::PLUS_PLUS) && !check(TokenType::MINUS_MINUS))
        {
            pos = savedPos; // restore so newline terminates the statement
            break;
        }

        int ln = current().line;
        if (check(TokenType::PLUS_PLUS))
        {
            consume();
            auto one = std::make_unique<ASTNode>(NumberLiteral{1.0}, ln);
            expr = std::make_unique<ASTNode>(AssignExpr{"+=", std::move(expr), std::move(one)}, ln);
        }
        else if (check(TokenType::MINUS_MINUS))
        {
            consume();
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
            consume(); // eat '['
            // Detect slice: [start:stop:step] — any part optional
            // A leading ':' means start is omitted
            bool isSlice = false;
            ASTNodePtr idxOrStart;

            if (check(TokenType::COLON))
                isSlice = true; // [:...]  — start omitted
            else
            {
                idxOrStart = parseExpr();
                if (check(TokenType::COLON))
                    isSlice = true; // [expr:...]
            }

            if (isSlice)
            {
                // consume the first ':'
                consume();
                ASTNodePtr stop, step;
                if (!check(TokenType::RBRACKET) && !check(TokenType::COLON))
                    stop = parseExpr();
                if (match(TokenType::COLON))
                {
                    if (!check(TokenType::RBRACKET))
                        step = parseExpr();
                }
                expect(TokenType::RBRACKET, "Expected ']'");
                SliceExpr sl;
                sl.object = std::move(expr);
                sl.start = std::move(idxOrStart);
                sl.stop = std::move(stop);
                sl.step = std::move(step);
                expr = std::make_unique<ASTNode>(std::move(sl), ln);
            }
            else
            {
                expect(TokenType::RBRACKET, "Expected ']'");
                expr = std::make_unique<ASTNode>(IndexExpr{std::move(expr), std::move(idxOrStart)}, ln);
            }
        }
        else if (check(TokenType::DOT))
        {
            consume();
            // Accept any identifier or keyword as member name (e.g. type, filter, length)
            std::string mem;
            if (check(TokenType::IDENTIFIER))
                mem = consume().value;
            else if (isCTypeKeyword(current().type))
                mem = consume().value;
            else
                mem = expect(TokenType::IDENTIFIER, "Expected member name").value;

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

    // this / self → Identifier{"self"}
    if (tok.type == TokenType::THIS)
    {
        consume();
        return std::make_unique<ASTNode>(Identifier{"self"}, ln);
    }

    // new ClassName(args) → CallExpr on the class constructor
    if (tok.type == TokenType::NEW)
    {
        consume();
        auto name = expect(TokenType::IDENTIFIER, "Expected class name after \'new\'").value;
        auto callee = std::make_unique<ASTNode>(Identifier{name}, ln);
        auto args = parseArgList();
        return std::make_unique<ASTNode>(CallExpr{std::move(callee), std::move(args)}, ln);
    }

    // super → super() or super.method()
    if (tok.type == TokenType::SUPER)
    {
        consume();
        if (check(TokenType::DOT))
        {
            consume(); // eat '.'
            auto method = expect(TokenType::IDENTIFIER, "Expected method name after 'super.'").value;
            return std::make_unique<ASTNode>(SuperExpr{method}, ln);
        }
        // bare super — used as super(args) which becomes a CallExpr wrapping SuperExpr
        return std::make_unique<ASTNode>(SuperExpr{""}, ln);
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

        // Normal parenthesised expression (or tuple literal)
        auto expr = parseExpr();
        skipNewlines();

        // Tuple literal: (a, b, c)  — comma after first expr
        if (check(TokenType::COMMA))
        {
            TupleLiteral tup;
            tup.elements.push_back(std::move(expr));
            while (match(TokenType::COMMA))
            {
                skipNewlines();
                if (check(TokenType::RPAREN))
                    break; // trailing comma
                tup.elements.push_back(parseExpr());
                skipNewlines();
            }
            expect(TokenType::RPAREN, "Expected ')'");
            return std::make_unique<ASTNode>(std::move(tup), ln);
        }

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

    // Built-in keyword tokens that can appear as callable expressions in rhs context.
    // e.g.  x = input("prompt")   result = print(...)   val = len(arr)
    // We treat them as identifiers so parsePostfix can handle the call.
    switch (tok.type)
    {
    case TokenType::INPUT:
    case TokenType::PRINT:
    case TokenType::SCAN:
    case TokenType::PAYLOAD:
    case TokenType::ENCRYPT:
    case TokenType::DECRYPT:
    case TokenType::HASH:
    case TokenType::IMPORT:
    {
        auto name = tok.value;
        consume();
        return std::make_unique<ASTNode>(Identifier{name}, ln);
    }
    default:
        break;
    }

    throw ParseError("Unexpected token: '" + tok.value + "'", tok.line, tok.col);
}

ASTNodePtr Parser::parseArrayLiteral()
{
    int ln = current().line;
    expect(TokenType::LBRACKET, "Expected '['");
    skipNewlines();

    // Empty array
    if (check(TokenType::RBRACKET))
    {
        consume();
        return std::make_unique<ASTNode>(ArrayLiteral{}, ln);
    }

    // Parse first expression — then decide if it's a list comprehension
    auto firstExpr = parseExpr();
    skipNewlines();

    // List comprehension: [expr for var in iterable (if cond)?]
    if (check(TokenType::FOR))
    {
        consume(); // eat 'for'
        // Collect loop variable(s) — support tuple unpacking: for k, v in ...
        std::vector<std::string> vars;
        auto readVar = [&]()
        {
            if (check(TokenType::IDENTIFIER))
                vars.push_back(consume().value);
            else if (isCTypeKeyword(current().type))
                vars.push_back(consume().value);
            else
                vars.push_back(expect(TokenType::IDENTIFIER, "Expected variable in comprehension").value);
        };
        readVar();
        while (match(TokenType::COMMA))
            readVar();

        if (!match(TokenType::IN) && !match(TokenType::OF))
            throw ParseError("Expected 'in' in list comprehension", current().line, current().col);

        auto iterable = parseExpr();
        skipNewlines();

        // Optional filter: if condition
        ASTNodePtr condition;
        if (check(TokenType::IF))
        {
            consume();
            condition = parseExpr();
            skipNewlines();
        }

        expect(TokenType::RBRACKET, "Expected ']'");
        ListComp lc;
        lc.expr = std::move(firstExpr);
        lc.vars = std::move(vars);
        lc.iterable = std::move(iterable);
        lc.condition = std::move(condition);
        return std::make_unique<ASTNode>(std::move(lc), ln);
    }

    // Regular array literal
    ArrayLiteral arr;
    arr.elements.push_back(std::move(firstExpr));
    skipNewlines();
    while (match(TokenType::COMMA))
    {
        skipNewlines();
        if (check(TokenType::RBRACKET))
            break; // trailing comma
        arr.elements.push_back(parseExpr());
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
        // Key: accept quoted string, number, bare identifier, or type keyword
        // e.g.  "name": ...   or   firstName: ...   or   42: ...
        ASTNodePtr key;
        if (check(TokenType::IDENTIFIER) || isCTypeKeyword(current().type) || check(TokenType::TYPE_STRING))
        {
            // Peek ahead — if next token after this is COLON, treat as bare string key
            size_t la = pos + 1;
            if (la < tokens.size() && tokens[la].type == TokenType::COLON)
            {
                // Bare identifier key: firstName → StringLiteral "firstName"
                auto keyName = consume().value;
                key = std::make_unique<ASTNode>(StringLiteral{keyName}, ln);
            }
            else
                key = parseExpr();
        }
        else
            key = parseExpr();

        expect(TokenType::COLON, "Expected ':'");
        skipNewlines();
        auto val = parseExpr();
        dict.pairs.emplace_back(std::move(key), std::move(val));
        skipNewlines();
        if (!match(TokenType::COMMA))
            break;
        skipNewlines();
        // Allow trailing comma: { a: 1, b: 2, }
        if (check(TokenType::RBRACE))
            break;
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
        int argLn = current().line;
        // keyword argument: name=expr — skip the name= and just use the value
        if (check(TokenType::IDENTIFIER))
        {
            size_t la = pos + 1;
            while (la < tokens.size() && tokens[la].type == TokenType::NEWLINE)
                ++la;
            if (la < tokens.size() && tokens[la].type == TokenType::ASSIGN)
            {
                consume(); // name
                while (check(TokenType::NEWLINE))
                    consume();
                consume(); // '='
                skipNewlines();
            }
        }

        auto expr = parseExpr();
        skipNewlines();

        // Generator expression: f(expr for var in iterable)
        if (check(TokenType::FOR))
        {
            consume();
            std::vector<std::string> vars;
            auto readVar = [&]()
            {
                if (check(TokenType::IDENTIFIER))
                    vars.push_back(consume().value);
                else if (isCTypeKeyword(current().type))
                    vars.push_back(consume().value);
                else
                    vars.push_back(expect(TokenType::IDENTIFIER, "Expected variable").value);
            };
            readVar();
            while (match(TokenType::COMMA))
                readVar();
            if (!match(TokenType::IN) && !match(TokenType::OF))
                throw ParseError("Expected 'in' in generator expression", current().line, current().col);
            auto iterable = parseExpr();
            skipNewlines();
            ASTNodePtr condition;
            if (check(TokenType::IF))
            {
                consume();
                condition = parseExpr();
                skipNewlines();
            }
            ListComp lc;
            lc.expr = std::move(expr);
            lc.vars = std::move(vars);
            lc.iterable = std::move(iterable);
            lc.condition = std::move(condition);
            args.push_back(std::make_unique<ASTNode>(std::move(lc), argLn));
            skipNewlines();
            break; // generator consumes entire arg list
        }

        args.push_back(std::move(expr));
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

        if (check(TokenType::IDENTIFIER))
        {
            params.push_back(consume().value);
        }
        else if (check(TokenType::THIS))
        {
            params.push_back(consume().value);
        }
        else
        {
            throw ParseError("Expected parameter name", current().line, current().col);
        }

        // Python-style annotation: "x: int" or "x: str" — skip ": type"
        if (check(TokenType::COLON))
        {
            consume(); // eat :
            // consume the type — could be identifier, type keyword, or generic like List[X]
            if (check(TokenType::IDENTIFIER) || isCTypeKeyword(current().type))
            {
                consume(); // eat base type name
                // Handle generic subscript: List[X], Dict[str, int], Optional[X], etc.
                if (check(TokenType::LBRACKET))
                {
                    consume(); // eat '['
                    int depth = 1;
                    while (!atEnd() && depth > 0)
                    {
                        if (check(TokenType::LBRACKET))
                            depth++;
                        else if (check(TokenType::RBRACKET))
                            depth--;
                        consume();
                    }
                }
            }
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