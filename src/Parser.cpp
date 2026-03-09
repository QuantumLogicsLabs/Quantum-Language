#include "../include/Parser.h"
#include <sstream>
#include <unordered_set>
#include <cctype>

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
        consume(); // eat 'const'
        // C-style: "const int* p = &a" — const followed by a type keyword
        // In this case treat as a C-type var decl (the const is just a qualifier, ignore it)
        if (isCTypeKeyword(current().type))
        {
            // Re-use the TYPE_INT/etc. branch logic inline:
            // eat all type keywords (e.g. "const int", "const unsigned long")
            std::string typeHint = consume().value;
            while (isCTypeKeyword(current().type))
                typeHint += " " + consume().value;
            // eat trailing const after type: "int* const ptr"
            if (check(TokenType::CONST))
                consume();
            auto firstDecl = parseCTypeVarDecl(typeHint);
            if (!check(TokenType::COMMA))
                return firstDecl;
            auto block = std::make_unique<ASTNode>(BlockStmt{}, ln);
            block->as<BlockStmt>().statements.push_back(std::move(firstDecl));
            while (check(TokenType::COMMA))
            {
                consume();
                std::string hint = typeHint;
                if (isCTypeKeyword(current().type) || check(TokenType::CONST))
                {
                    hint = consume().value;
                    while (isCTypeKeyword(current().type) || check(TokenType::CONST))
                        hint += " " + consume().value;
                }
                block->as<BlockStmt>().statements.push_back(parseCTypeVarDecl(hint));
            }
            while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
                consume();
            return block;
        }
        // Quantum/JS style: "const x = 5"
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
        // If followed by '(' it could be a user-defined function named "print"
        // Only treat as print statement if NOT followed by ( with comma-separated args
        // that look like a function call (i.e. not the Quantum print keyword usage)
        if (pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::LPAREN)
            return parseExprStmt();
        consume();
        return parsePrintStmt();
    }
    case TokenType::INPUT:
    {
        // If followed by '(' it's a user-defined function named "input(...)", not a cin stmt
        if (pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::LPAREN)
            return parseExprStmt();
        consume();
        return parseInputStmt();
    }
    case TokenType::CIN:
    {
        consume();
        return parseCinStmt();
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
        //       "int main()" → function declaration
        //       "string = x" → plain assignment (string used as variable name)
        // Peek past any chained type qualifiers to find the next real token.
        size_t lookahead = pos + 1;
        while (lookahead < tokens.size() && (isCTypeKeyword(tokens[lookahead].type) ||
                                             tokens[lookahead].type == TokenType::CONST))
            ++lookahead;
        // Skip pointer/reference qualifiers between type and name: int* funcName, int& ref
        while (lookahead < tokens.size() &&
               (tokens[lookahead].type == TokenType::STAR || tokens[lookahead].type == TokenType::BIT_AND ||
                tokens[lookahead].type == TokenType::CONST))
            ++lookahead;
        // Also accept keyword tokens used as function names (e.g. "input", "display", "print")
        auto isValidFuncName = [&](TokenType t)
        {
            return t == TokenType::IDENTIFIER || t == TokenType::INPUT || t == TokenType::PRINT;
        };
        if (lookahead < tokens.size() && isValidFuncName(tokens[lookahead].type))
        {
            // Check if this is actually a function definition: ReturnType funcName(params)
            size_t la2 = lookahead + 1;
            while (la2 < tokens.size() && tokens[la2].type == TokenType::NEWLINE)
                la2++;
            if (la2 < tokens.size() && tokens[la2].type == TokenType::LPAREN)
            {
                // It's a function definition — consume return type and parse as function
                while (isCTypeKeyword(current().type) || check(TokenType::CONST))
                    consume(); // eat return type keywords + const
                // Also eat any pointer/reference qualifiers: int* funcName → eat *
                while (check(TokenType::STAR) || check(TokenType::BIT_AND) || check(TokenType::CONST))
                    consume();
                return parseFunctionDecl();
            }
            auto typeHint = consume().value;
            while (isCTypeKeyword(current().type) || check(TokenType::CONST))
                typeHint += " " + consume().value;

            // Consume pointer stars that come BEFORE any name, e.g. "int* a, *b, c"
            // We let parseCTypeVarDecl handle per-name stars; here we just check
            // if this is a multi-variable declaration: int a, b, c;
            // Build a block of VarDecls for comma-separated names.
            {
                auto firstDecl = parseCTypeVarDecl(typeHint);
                if (!check(TokenType::COMMA))
                    return firstDecl;
                // Multi-var: int a, b, c;  or  int a, *b, c = 5;  or  int a, int b, int c;
                auto block = std::make_unique<ASTNode>(BlockStmt{}, ln);
                block->as<BlockStmt>().statements.push_back(std::move(firstDecl));
                while (check(TokenType::COMMA))
                {
                    consume(); // eat ','
                    // Each var may repeat the type keyword: "int a, int b" → eat optional type
                    std::string hint = typeHint;
                    if (isCTypeKeyword(current().type) || check(TokenType::CONST))
                    {
                        hint = consume().value;
                        while (isCTypeKeyword(current().type) || check(TokenType::CONST))
                            hint += " " + consume().value;
                    }
                    block->as<BlockStmt>().statements.push_back(parseCTypeVarDecl(hint));
                }
                while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
                    consume();
                return block;
            }
        }
        // Not a declaration — fall through to expression statement
        return parseExprStmt();
    }
    default:
        // Handle C++ "delete ptr" and "delete[] ptr" as no-ops (GC handles memory)
        if (check(TokenType::IDENTIFIER) && current().value == "delete")
        {
            consume(); // eat 'delete'
            if (check(TokenType::LBRACKET))
            {
                consume();
                if (check(TokenType::RBRACKET))
                    consume();
            }
            if (!check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON) && !atEnd())
                parseExpr();
            while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
                consume();
            return std::make_unique<ASTNode>(BlockStmt{}, ln);
        }
        // Handle C++ "using namespace X;" as a no-op
        if (check(TokenType::IDENTIFIER) && current().value == "using")
        {
            while (!atEnd() && !check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON))
                consume();
            while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
                consume();
            // Return an empty block as a no-op
            return std::make_unique<ASTNode>(BlockStmt{}, ln);
        }
        // Handle C "typedef ..." as a near-no-op — skip the declaration but
        // register the alias name as an empty class so later uses like
        // "Cell board[N]" or "Point snake[N]" resolve without NameError.
        if (check(TokenType::IDENTIFIER) && current().value == "typedef")
        {
            consume(); // eat 'typedef'

            // Detect "typedef enum { ... } Alias;" and extract enumerator constants.
            bool isEnum = (check(TokenType::IDENTIFIER) &&
                           (current().value == "enum" || current().value == "struct" || current().value == "union"));
            bool isRawEnum = isEnum && current().value == "enum";
            if (isEnum)
                consume(); // eat 'enum'/'struct'/'union'

            // Optional tag name before '{'
            if (check(TokenType::IDENTIFIER) && !check(TokenType::LBRACE))
            {
                // peek: if next is '{' it's the body, otherwise it's a tag name
                if (pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::LBRACE)
                    consume(); // eat optional tag name
                // If no '{' follows, this is a forward decl or typedef of existing type
            }

            // Parse enum body { A=0, B, C=5, ... } to collect enumerator constants
            std::vector<std::pair<std::string, double>> enumerators;
            if (isRawEnum && check(TokenType::LBRACE))
            {
                consume(); // eat '{'
                double nextVal = 0.0;
                while (!check(TokenType::RBRACE) && !atEnd())
                {
                    if (check(TokenType::NEWLINE))
                    {
                        consume();
                        continue;
                    }
                    if (!check(TokenType::IDENTIFIER))
                    {
                        consume();
                        continue;
                    }
                    std::string eName = consume().value;
                    double eVal = nextVal;
                    if (match(TokenType::ASSIGN))
                    {
                        // parse a simple numeric constant (possibly negative)
                        bool neg = false;
                        if (check(TokenType::MINUS))
                        {
                            neg = true;
                            consume();
                        }
                        if (check(TokenType::NUMBER))
                            eVal = std::stod(consume().value) * (neg ? -1 : 1);
                        else if (check(TokenType::IDENTIFIER))
                        {
                            // reference to a previously defined enumerator
                            std::string ref = consume().value;
                            for (auto &p : enumerators)
                                if (p.first == ref)
                                {
                                    eVal = p.second;
                                    break;
                                }
                        }
                    }
                    enumerators.push_back({eName, eVal});
                    nextVal = eVal + 1.0;
                    if (check(TokenType::COMMA))
                        consume();
                }
                if (check(TokenType::RBRACE))
                    consume(); // eat '}'
            }
            else
            {
                // Non-enum typedef: skip to matching '}' if there's a body, else to ';'
                int depth = 0;
                while (!atEnd())
                {
                    if (check(TokenType::LBRACE))
                    {
                        depth++;
                        consume();
                    }
                    else if (check(TokenType::RBRACE))
                    {
                        depth--;
                        consume();
                        if (depth == 0)
                            break;
                    }
                    else if (check(TokenType::SEMICOLON) && depth == 0)
                        break;
                    else if (check(TokenType::NEWLINE) && depth == 0)
                        break;
                    else
                        consume();
                }
            }

            // Collect tokens up to ';' to find the alias name (last identifier before ';')
            std::string aliasName;
            {
                std::string lastIdent;
                while (!atEnd())
                {
                    if (check(TokenType::SEMICOLON) || (check(TokenType::NEWLINE)))
                    {
                        aliasName = lastIdent;
                        consume();
                        break;
                    }
                    if (check(TokenType::IDENTIFIER))
                        lastIdent = current().value;
                    consume();
                }
            }

            // Build result: emit enumerator VarDecls + ClassDecl alias in a BlockStmt
            if (!enumerators.empty() || !aliasName.empty())
            {
                auto block = std::make_unique<ASTNode>(BlockStmt{}, ln);
                // Emit one VarDecl for each enumerator constant
                for (auto &[eName, eVal] : enumerators)
                {
                    auto numNode = std::make_unique<ASTNode>(NumberLiteral{eVal}, ln);
                    VarDecl vd;
                    vd.name = eName;
                    vd.initializer = std::move(numNode);
                    block->as<BlockStmt>().statements.push_back(
                        std::make_unique<ASTNode>(std::move(vd), ln));
                }
                // Emit ClassDecl for the alias name
                if (!aliasName.empty())
                {
                    ClassDecl cd;
                    cd.name = aliasName;
                    block->as<BlockStmt>().statements.push_back(
                        std::make_unique<ASTNode>(std::move(cd), ln));
                }
                return block;
            }
            return std::make_unique<ASTNode>(BlockStmt{}, ln);
        }
        // Handle C++ class-type variable declaration: "ClassName varName;" or "ClassName varName(args);"
        // Also handles: "struct TypeName var1, var2;" where 'struct' is an IDENTIFIER token.
        if (check(TokenType::IDENTIFIER))
        {
            // Detect optional leading 'struct' / 'union' / 'enum' keyword
            bool hasStructKeyword = (current().value == "struct" ||
                                     current().value == "union" ||
                                     current().value == "enum");

            size_t la = pos + (hasStructKeyword ? 1 : 0); // index of the type-name token
            size_t laName = la + 1;                       // index of the first var-name token

            // Skip template arguments: unique_ptr<int[]>, shared_ptr<Foo>, vector<int>, etc.
            if (laName < tokens.size() && tokens[laName].type == TokenType::LT)
            {
                int tdepth = 0;
                while (laName < tokens.size())
                {
                    if (tokens[laName].type == TokenType::LT)
                        tdepth++;
                    else if (tokens[laName].type == TokenType::GT)
                    {
                        tdepth--;
                        laName++;
                        break;
                    }
                    else if (tokens[laName].type == TokenType::RSHIFT)
                    {
                        tdepth -= 2;
                        laName++;
                        break;
                    }
                    laName++;
                }
            }

            // Skip pointer/ref qualifiers between type and first var name
            while (laName < tokens.size() &&
                   (tokens[laName].type == TokenType::STAR ||
                    tokens[laName].type == TokenType::BIT_AND ||
                    tokens[laName].type == TokenType::CONST))
                ++laName;

            if (la < tokens.size() && tokens[la].type == TokenType::IDENTIFIER &&
                laName < tokens.size() && tokens[laName].type == TokenType::IDENTIFIER)
            {
                // Two identifiers in a row (optionally prefixed by struct/union/enum):
                // could be "ClassName varName" or "ClassName varName(args)" or
                // "struct TermType oldt, newt;"
                size_t la2 = laName + 1;
                // If varName is followed by ; or = or ( or , or [ it's a declaration
                if (la2 < tokens.size() &&
                    (tokens[la2].type == TokenType::SEMICOLON ||
                     tokens[la2].type == TokenType::NEWLINE ||
                     tokens[la2].type == TokenType::ASSIGN ||
                     tokens[la2].type == TokenType::LPAREN ||
                     tokens[la2].type == TokenType::COMMA ||
                     tokens[la2].type == TokenType::LBRACKET))
                {
                    if (hasStructKeyword)
                        consume(); // eat 'struct' / 'union' / 'enum'

                    std::string typeName = consume().value; // eat type name
                    // Skip template arguments: unique_ptr<int[]>, shared_ptr<Foo>, etc.
                    if (check(TokenType::LT))
                    {
                        consume(); // eat '<'
                        int tdepth = 1;
                        while (!atEnd() && tdepth > 0)
                        {
                            if (check(TokenType::LT))
                                tdepth++;
                            else if (check(TokenType::GT))
                                tdepth--;
                            else if (check(TokenType::RSHIFT))
                            {
                                tdepth -= 2;
                                consume();
                                continue;
                            }
                            consume();
                        }
                    }

                    // Helper: parse one variable name (with optional pointer stars) and its initializer
                    auto parseOneVar = [&]() -> ASTNodePtr
                    {
                        bool isPtr = false;
                        while (check(TokenType::STAR) || check(TokenType::CONST) || check(TokenType::BIT_AND))
                        {
                            if (check(TokenType::STAR))
                                isPtr = true;
                            consume();
                        }
                        std::string varName = expect(TokenType::IDENTIFIER, "Expected variable name").value;
                        // Skip C array dimension brackets: board[ROWS][COLS], snake[MAX_LEN], etc.
                        while (check(TokenType::LBRACKET))
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
                        ASTNodePtr init;
                        if (check(TokenType::LPAREN))
                        {
                            // ClassName varName(args) — constructor call as initializer
                            auto callee = std::make_unique<ASTNode>(Identifier{typeName}, ln);
                            CallExpr ce;
                            ce.callee = std::move(callee);
                            consume(); // eat (
                            while (!check(TokenType::RPAREN) && !atEnd())
                            {
                                ce.args.push_back(parseExpr());
                                if (!match(TokenType::COMMA))
                                    break;
                            }
                            if (check(TokenType::RPAREN))
                                consume();
                            init = std::make_unique<ASTNode>(std::move(ce), ln);
                        }
                        else if (match(TokenType::ASSIGN))
                        {
                            init = parseExpr();
                        }
                        else
                        {
                            // ClassName varName; — call default constructor
                            auto callee = std::make_unique<ASTNode>(Identifier{typeName}, ln);
                            CallExpr ce;
                            ce.callee = std::move(callee);
                            init = std::make_unique<ASTNode>(std::move(ce), ln);
                        }
                        auto decl = VarDecl{false, varName, std::move(init), typeName};
                        decl.isPointer = isPtr;
                        return std::make_unique<ASTNode>(std::move(decl), ln);
                    };

                    auto firstDecl = parseOneVar();

                    // If no comma follows, return single declaration
                    if (!check(TokenType::COMMA))
                    {
                        while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
                            consume();
                        return firstDecl;
                    }

                    // Multi-var: struct termios oldt, newt;
                    auto block = std::make_unique<ASTNode>(BlockStmt{}, ln);
                    block->as<BlockStmt>().statements.push_back(std::move(firstDecl));
                    while (check(TokenType::COMMA))
                    {
                        consume(); // eat ','
                        block->as<BlockStmt>().statements.push_back(parseOneVar());
                    }
                    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
                        consume();
                    return block;
                }
            }
        }
        // Handle C++ "do { } while (cond);" loop
        if (check(TokenType::IDENTIFIER) && current().value == "do")
        {
            consume(); // eat 'do'
            skipNewlines();
            auto body = parseBlock();
            skipNewlines();
            if (check(TokenType::WHILE))
                consume();
            else if (check(TokenType::IDENTIFIER) && current().value == "while")
                consume();
            expect(TokenType::LPAREN, "Expected '(' after 'while' in do-while");
            auto condition = parseExpr();
            expect(TokenType::RPAREN, "Expected ')' after do-while condition");
            while (check(TokenType::SEMICOLON) || check(TokenType::NEWLINE))
                consume();

            // Desugar to: while(true) { body; if(!cond) break; }
            BlockStmt whileBody;
            for (auto &s : body->as<BlockStmt>().statements)
                whileBody.statements.push_back(std::move(const_cast<ASTNodePtr &>(s)));
            // Add if(!cond){break;} at end
            UnaryExpr notCond;
            notCond.op = "!";
            notCond.operand = std::move(condition);
            BlockStmt breakBlock;
            breakBlock.statements.push_back(std::make_unique<ASTNode>(BreakStmt{}, ln));
            IfStmt ifBreak;
            ifBreak.condition = std::make_unique<ASTNode>(std::move(notCond), ln);
            ifBreak.thenBranch = std::make_unique<ASTNode>(std::move(breakBlock), ln);
            whileBody.statements.push_back(std::make_unique<ASTNode>(std::move(ifBreak), ln));

            auto whileBodyNode = std::make_unique<ASTNode>(std::move(whileBody), ln);
            auto trueNode = std::make_unique<ASTNode>(BoolLiteral{true}, ln);
            return std::make_unique<ASTNode>(WhileStmt{std::move(trueNode), std::move(whileBodyNode)}, ln);
        }
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

// Accepts { block }, INDENT block, a single statement, or a bare ';' (empty body)
ASTNodePtr Parser::parseBodyOrStatement()
{
    // C empty-body: while(cond); or for(...);
    if (check(TokenType::SEMICOLON))
    {
        int ln = current().line;
        consume(); // eat ';'
        return std::make_unique<ASTNode>(BlockStmt{}, ln);
    }
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
    // Accept IDENTIFIER or keyword tokens used as function names (e.g. "input", "display")
    Token nameToken = check(TokenType::IDENTIFIER) ? consume() : (check(TokenType::INPUT) || check(TokenType::PRINT)) ? consume()
                                                                                                                      : expect(TokenType::IDENTIFIER, "Expected function name");
    std::vector<bool> paramIsRef;
    auto params = parseParamList(&paramIsRef);

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

    // ── C++ forward declaration / prototype support ───────────────────────────
    // If there's no body (just a ';' or end-of-line), this is a forward
    // declaration like:  void cubeByPtr(int* p);
    // We treat it as a no-op and return an empty function stub.
    if (check(TokenType::SEMICOLON) || check(TokenType::NEWLINE) || atEnd())
    {
        while (check(TokenType::SEMICOLON) || check(TokenType::NEWLINE))
            consume();
        // Return an empty block as the body (no-op prototype)
        auto emptyBody = std::make_unique<ASTNode>(BlockStmt{}, ln);
        FunctionDecl fd;
        fd.name = nameToken.value;
        fd.params = std::move(params);
        fd.paramIsRef = std::move(paramIsRef);
        fd.body = std::move(emptyBody);
        return std::make_unique<ASTNode>(std::move(fd), ln);
    }

    auto body = parseBlock();
    FunctionDecl fd;
    fd.name = nameToken.value;
    fd.params = std::move(params);
    fd.paramIsRef = std::move(paramIsRef);
    fd.body = std::move(body);
    return std::make_unique<ASTNode>(std::move(fd), ln);
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

            // Skip access modifiers and C++ qualifiers
            while (check(TokenType::IDENTIFIER) &&
                   (current().value == "public" || current().value == "private" ||
                    current().value == "protected" || current().value == "static" ||
                    current().value == "async" || current().value == "virtual" ||
                    current().value == "inline" || current().value == "explicit" ||
                    current().value == "override" || current().value == "final"))
            {
                if (current().value == "static")
                    isStatic = true;
                consume();
            }
            // Skip leading const (e.g. const int foo = 5;)
            if (check(TokenType::CONST))
                consume();

            // Optional fn/def/function keyword
            if (check(TokenType::FN) || check(TokenType::DEF) || check(TokenType::FUNCTION))
                consume();

            // Handle ~ClassName destructor syntax (C++ style)
            if (check(TokenType::BIT_NOT))
            {
                consume(); // eat ~
                if (check(TokenType::IDENTIFIER))
                    consume(); // eat class name
                std::vector<bool> destructorParamIsRef;
                auto params = parseParamList(&destructorParamIsRef);

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
                FunctionDecl delFd;
                delFd.name = "__del__";
                delFd.params = std::move(params);
                delFd.body = std::move(body);
                auto fn = std::make_unique<ASTNode>(std::move(delFd), ln);
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
            // --- Detect C++ member variable declarations ---
            // Pattern: TypeName varName; OR TypeKeyword varName;
            // This is when we have IDENTIFIER followed by IDENTIFIER (then ; or =)
            // e.g.  string heroName;   OR   TownsvilleGuardian obj;
            // Also handles C type keywords: int x; double y;
            {
                bool isMemberVar = false;
                std::string typeToken;

                if (isCTypeKeyword(current().type))
                {
                    // e.g. int energyLevel; OR int getEnergyLevel()
                    // Peek ahead to determine if this is a field or method
                    size_t la = pos + 1;
                    while (la < tokens.size() && isCTypeKeyword(tokens[la].type))
                        la++;
                    // skip & or *
                    while (la < tokens.size() && (tokens[la].type == TokenType::BIT_AND || tokens[la].type == TokenType::STAR))
                        la++;
                    if (la < tokens.size() && tokens[la].type == TokenType::IDENTIFIER)
                    {
                        // Check if method name is followed by '('
                        size_t la2 = la + 1;
                        while (la2 < tokens.size() && tokens[la2].type == TokenType::NEWLINE)
                            la2++;
                        if (la2 < tokens.size() && tokens[la2].type == TokenType::LPAREN)
                        {
                            // It's a method with return type: "int getEnergyLevel()"
                            // Consume the type tokens as return type and fall through to method parsing
                            consume(); // eat first type keyword
                            while (isCTypeKeyword(current().type))
                                consume(); // multi-word types
                            while (check(TokenType::BIT_AND) || check(TokenType::STAR))
                                consume();
                            // Fall through to method name parsing
                        }
                        else
                        {
                            // It's a field: "int energyLevel;"
                            typeToken = consume().value;
                            while (isCTypeKeyword(current().type))
                                typeToken += " " + consume().value;
                            while (check(TokenType::BIT_AND) || check(TokenType::STAR))
                                consume();
                            isMemberVar = true;
                        }
                    }
                    else
                    {
                        // No identifier after type — skip
                        consume();
                    }
                }
                else if (check(TokenType::IDENTIFIER))
                {
                    // Peek: if the token after this identifier is another identifier (var name)
                    // AND that identifier is NOT followed by '(' (i.e. it's a field, not a method)
                    size_t la = pos + 1;
                    while (la < tokens.size() && tokens[la].type == TokenType::BIT_AND)
                        la++;
                    if (la < tokens.size() && tokens[la].type == TokenType::IDENTIFIER && tokens[la].value != cd.name) // not constructor pattern
                    {
                        // Check if the name token is followed by '(' → it's a method, not a field
                        size_t la2 = la + 1;
                        while (la2 < tokens.size() && tokens[la2].type == TokenType::NEWLINE)
                            la2++;
                        if (la2 < tokens.size() && tokens[la2].type == TokenType::LPAREN)
                        {
                            // This is a return-type + method-name pattern (e.g. "string getHeroName()")
                            // Don't treat as member var — treat it as method with return type prefix
                            // We'll consume the type token as a return type hint and fall through to method parsing
                            consume(); // eat return type (e.g. "string", "int", "void")
                            while (check(TokenType::BIT_AND) || check(TokenType::STAR))
                                consume();
                            // Fall through to method name parsing below
                        }
                        else
                        {
                            typeToken = consume().value; // eat type name
                            while (check(TokenType::BIT_AND) || check(TokenType::STAR))
                                consume();
                            isMemberVar = true;
                        }
                    }
                }

                if (isMemberVar)
                {
                    // It's a field declaration: varName [= init];
                    if (!check(TokenType::IDENTIFIER))
                    {
                        // skip garbage
                        while (!atEnd() && !check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON))
                            consume();
                        while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
                            consume();
                        continue;
                    }
                    std::string fieldName = consume().value;
                    ASTNodePtr init;
                    if (match(TokenType::ASSIGN))
                        init = parseExpr();
                    // store as a VarDecl field
                    auto fld = std::make_unique<ASTNode>(
                        VarDecl{false, fieldName, std::move(init), typeToken}, ln);
                    cd.fields.push_back(std::move(fld));
                    while (!atEnd() && !check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON) && !check(TokenType::RBRACE) && !check(TokenType::DEDENT))
                        consume(); // skip anything remaining on line
                    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
                        consume();
                    continue;
                }
            }

            std::string methodName = consume().value;

            // Normalize constructor names
            if (methodName == "constructor" || methodName == "__init__" || methodName == cd.name)
                methodName = "init";
            // Normalize destructor names
            if (methodName == "destructor")
                methodName = "__del__";
            // Normalize toString
            if (methodName == "toString" || methodName == "to_string" || methodName == "to_str")
                methodName = "__str__";

            // Check for method vs field: if next token is NOT '(' it might be a field
            if (!check(TokenType::LPAREN))
            {
                // Skip the rest of the line as a field/statement we can't parse
                while (!atEnd() && !check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON) && !check(TokenType::RBRACE))
                    consume();
                while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
                    consume();
                skipNewlines();
                continue;
            }

            std::vector<bool> methodParamIsRef;
            auto params = parseParamList(&methodParamIsRef);

            // Skip trailing C++ const: method() const { }
            if (check(TokenType::CONST))
                consume();
            // Also handle if 'const' appears as identifier token somehow
            if (check(TokenType::IDENTIFIER) && current().value == "const")
                consume();
            // Skip noexcept, override, final qualifiers
            while (check(TokenType::IDENTIFIER) &&
                   (current().value == "noexcept" || current().value == "override" ||
                    current().value == "final"))
                consume();

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

            FunctionDecl methodFd;
            methodFd.name = methodName;
            methodFd.params = std::move(params);
            methodFd.paramIsRef = std::move(methodParamIsRef);
            methodFd.body = std::move(body);
            auto fn = std::make_unique<ASTNode>(std::move(methodFd), ln);

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
        // C++ style: class Foo { }; — skip trailing semicolon
        while (check(TokenType::SEMICOLON))
            consume();
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
    std::string sep = " ";
    std::string end_str = "\n";
    std::vector<ASTNodePtr> args;
    if (check(TokenType::LPAREN))
    {
        consume();
        skipNewlines();
        while (!check(TokenType::RPAREN) && !atEnd())
        {
            // Detect keyword arguments: sep=, end=, file=, flush=
            if (check(TokenType::IDENTIFIER) &&
                (current().value == "sep" || current().value == "end" ||
                 current().value == "file" || current().value == "flush") &&
                pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::ASSIGN)
            {
                std::string kw = consume().value; // eat keyword name
                consume();                        // eat '='
                if (kw == "sep")
                {
                    if (check(TokenType::STRING))
                        sep = consume().value;
                    else
                        parseExpr(); // consume but discard non-literal
                }
                else if (kw == "end")
                {
                    if (check(TokenType::STRING))
                    {
                        end_str = consume().value;
                        newline = false; // end= overrides default newline
                    }
                    else
                        parseExpr();
                }
                else
                {
                    parseExpr(); // file= / flush= — consume and discard
                }
            }
            else
            {
                args.push_back(parseExpr());
            }
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
    PrintStmt ps;
    ps.args = std::move(args);
    ps.newline = newline;
    ps.sep = sep;
    ps.end = end_str;
    return std::make_unique<ASTNode>(std::move(ps), ln);
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
    // cin >> var  /  cin >> arr[i]  /  cin >> *(ptr+i)  /  cin >> var1 >> var2;
    // We cannot use parseExpr() because >> would be consumed as shift operator.
    // Strategy: if the target starts with * and (, parse the full parenthesised
    // expression inside, then wrap in DerefExpr. Otherwise parse postfix only.
    int ln = current().line;

    // Handle cin.ignore() / cin.get(...) / cin.getline(...) etc. — treat as no-ops.
    if (check(TokenType::DOT))
    {
        consume(); // eat '.'
        if (check(TokenType::IDENTIFIER))
            consume(); // eat method name (ignore, get, getline, peek, ...)
        // eat argument list if present
        if (check(TokenType::LPAREN))
        {
            consume(); // eat '('
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
        while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
            consume();
        return std::make_unique<ASTNode>(BlockStmt{}, ln);
    }

    BlockStmt block;
    while (check(TokenType::RSHIFT))
    {
        consume(); // eat >>
        if (check(TokenType::BIT_AND))
            consume(); // strip optional &

        ASTNodePtr lval;

        if (check(TokenType::STAR))
        {
            // *(expr) — dereference of an arbitrary expression in parens
            consume(); // eat *
            if (check(TokenType::LPAREN))
            {
                consume();                // eat (
                auto inner = parseExpr(); // safe: balanced parens, >> inside parens is shift
                expect(TokenType::RPAREN, "Expected ')' after dereferenced expression");
                DerefExpr de;
                de.operand = std::move(inner);
                lval = std::make_unique<ASTNode>(std::move(de), ln);
            }
            else
            {
                // *var — simple dereference
                auto operand = parsePrimary();
                DerefExpr de;
                de.operand = std::move(operand);
                lval = std::make_unique<ASTNode>(std::move(de), ln);
            }
        }
        else
        {
            // Simple lvalue: var, arr[i], obj.field
            lval = parsePrimary();
            // Handle postfix: arr[i], obj.field, ->member
            while (true)
            {
                if (check(TokenType::LBRACKET))
                {
                    consume();
                    auto idx = parseExpr();
                    expect(TokenType::RBRACKET, "Expected ']'");
                    IndexExpr ie;
                    ie.object = std::move(lval);
                    ie.index = std::move(idx);
                    lval = std::make_unique<ASTNode>(std::move(ie), ln);
                }
                else if (check(TokenType::DOT))
                {
                    consume();
                    auto member = expect(TokenType::IDENTIFIER, "Expected member name").value;
                    lval = std::make_unique<ASTNode>(MemberExpr{std::move(lval), member}, ln);
                }
                else if (check(TokenType::ARROW))
                {
                    consume();
                    auto member = expect(TokenType::IDENTIFIER, "Expected member name").value;
                    lval = std::make_unique<ASTNode>(ArrowExpr{std::move(lval), member}, ln);
                }
                else
                    break;
            }
        }

        InputStmt is;
        is.prompt = nullptr;
        if (lval->is<Identifier>())
            is.target = lval->as<Identifier>().name;
        else
            is.lvalueTarget = std::move(lval);

        block.statements.push_back(std::make_unique<ASTNode>(std::move(is), ln));
    }
    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
        consume();

    if (block.statements.size() == 1)
        return std::move(block.statements[0]);
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

    // C comma expression: "a += 3, b = a" — execute left-to-right as multiple stmts
    if (check(TokenType::COMMA))
    {
        BlockStmt block;
        block.statements.push_back(std::make_unique<ASTNode>(ExprStmt{std::move(expr)}, ln));
        while (match(TokenType::COMMA))
        {
            skipNewlines();
            if (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON) || atEnd())
                break;
            int eln = current().line;
            auto next = parseExpr();
            block.statements.push_back(std::make_unique<ASTNode>(ExprStmt{std::move(next)}, eln));
        }
        while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
            consume();
        return std::make_unique<ASTNode>(std::move(block), ln);
    }

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
        check(TokenType::SLASH_ASSIGN) || check(TokenType::AND_ASSIGN) ||
        check(TokenType::OR_ASSIGN) || check(TokenType::XOR_ASSIGN) ||
        check(TokenType::MOD_ASSIGN))
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
    // Unary + is a no-op: +1 == 1, +x == x
    if (check(TokenType::PLUS))
    {
        consume();
        return parseUnary(); // discard the +
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
    // C-style address-of: &var → AddressOfExpr
    if (check(TokenType::BIT_AND))
    {
        consume();
        int ln2 = current().line;
        auto operand = parseUnary();
        return std::make_unique<ASTNode>(AddressOfExpr{std::move(operand)}, ln2);
    }
    // C-style dereference: *ptr → DerefExpr
    if (check(TokenType::STAR))
    {
        consume();
        int ln2 = current().line;
        auto operand = parseUnary();
        return std::make_unique<ASTNode>(DerefExpr{std::move(operand)}, ln2);
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

        int ln = current().line; // declared here so it's in scope for all branches below

        if (!check(TokenType::DOT) && !check(TokenType::ARROW) && !check(TokenType::LBRACKET) && !check(TokenType::LPAREN) && !check(TokenType::PLUS_PLUS) && !check(TokenType::MINUS_MINUS))
        {
            // Check for C++ template call: identifier<Type>(args) e.g. make_unique<int[]>(n)
            // Only attempt if current expr is a simple identifier and next token is LESS
            if (check(TokenType::LT) && expr->is<Identifier>())
            {
                // Speculatively consume <...> — if followed by '(' it's a template call
                size_t preTpl = pos;
                consume(); // eat '<'
                int tdepth = 1;
                bool ok = false;
                while (!atEnd() && tdepth > 0)
                {
                    if (check(TokenType::LT))
                        tdepth++;
                    else if (check(TokenType::GT))
                    {
                        tdepth--;
                        if (tdepth == 0)
                        {
                            consume();
                            ok = true;
                            break;
                        }
                    }
                    else if (check(TokenType::RSHIFT))
                    {
                        tdepth -= 2;
                        consume();
                        ok = (tdepth <= 0);
                        break;
                    }
                    else if (check(TokenType::LPAREN) || check(TokenType::SEMICOLON) || check(TokenType::NEWLINE) || check(TokenType::EOF_TOKEN))
                        break;
                    consume();
                }
                if (ok && check(TokenType::LPAREN))
                {
                    // It's a template call — parse args and build CallExpr
                    auto args = parseArgList();
                    expr = std::make_unique<ASTNode>(CallExpr{std::move(expr), std::move(args)}, ln);
                    continue;
                }
                // Not a template call — restore position and break
                pos = preTpl;
            }
            pos = savedPos; // restore so newline terminates the statement
            break;
        }

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
        else if (check(TokenType::ARROW))
        {
            consume(); // eat ->
            std::string mem;
            if (check(TokenType::IDENTIFIER))
                mem = consume().value;
            else if (isCTypeKeyword(current().type))
                mem = consume().value;
            else
                mem = expect(TokenType::IDENTIFIER, "Expected member name after ->").value;
            if (check(TokenType::LPAREN))
            {
                // ptr->method(args) — treat as method call on dereferenced object
                auto deref = std::make_unique<ASTNode>(DerefExpr{std::move(expr)}, ln);
                auto memExpr = std::make_unique<ASTNode>(MemberExpr{std::move(deref), mem}, ln);
                auto args = parseArgList();
                expr = std::make_unique<ASTNode>(CallExpr{std::move(memExpr), std::move(args)}, ln);
            }
            else
            {
                expr = std::make_unique<ASTNode>(ArrowExpr{std::move(expr), mem}, ln);
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

    // new int(100) / new ClassName(args) / new int[n]
    if (tok.type == TokenType::NEW)
    {
        consume();
        // Accept both identifier class names AND C++ primitive type keywords
        std::string name;
        if (check(TokenType::IDENTIFIER))
            name = consume().value;
        else if (isCTypeKeyword(current().type))
            name = consume().value;
        else
            throw ParseError("Expected type name after 'new'", current().line, current().col);

        // new float[n] — array allocation: allocate array of n default-zero elements
        if (check(TokenType::LBRACKET))
        {
            consume(); // eat '['
            auto sizeNode = parseExpr();
            expect(TokenType::RBRACKET, "Expected ']'");
            NewExpr ne;
            ne.typeName = name;
            ne.isArray = true;
            ne.sizeExpr = std::move(sizeNode);
            return std::make_unique<ASTNode>(std::move(ne), ln);
        }

        // new int(100) / new ClassName(args) — heap allocation, always returns a pointer
        auto argNodes = parseArgList();
        NewExpr ne;
        ne.typeName = name;
        for (auto &a : argNodes)
            ne.args.push_back(std::move(a));
        return std::make_unique<ASTNode>(std::move(ne), ln);
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

        // ── C-style type cast: (int)x  (unsigned)time(NULL)  (char)c ───────
        // Detected when the expression inside parens was a single C type keyword
        // (parsed as an Identifier) AND the next token is NOT ')' — meaning the
        // ')' already closed the cast and the operand follows outside.
        // We simply discard the cast type and parse+return the operand.
        if (check(TokenType::RPAREN) && expr->is<Identifier>())
        {
            const std::string &idName = expr->as<Identifier>().name;
            // Known C type names used in casts (all lowercase — distinct from compound literals)
            static const std::unordered_set<std::string> cCastTypes = {
                "int", "unsigned", "long", "short", "char", "float", "double",
                "bool", "void", "size_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
                "int8_t", "int16_t", "int32_t", "int64_t", "uintptr_t", "ptrdiff_t"};
            if (cCastTypes.count(idName))
            {
                // Peek: if after ')' there is a value-producing token (not end/newline/;),
                // this is a cast — eat ')' and parse the cast operand.
                size_t peekPos = pos + 1;
                while (peekPos < tokens.size() && tokens[peekPos].type == TokenType::NEWLINE)
                    ++peekPos;
                if (peekPos < tokens.size() &&
                    tokens[peekPos].type != TokenType::SEMICOLON &&
                    tokens[peekPos].type != TokenType::EOF_TOKEN &&
                    tokens[peekPos].type != TokenType::RBRACE &&
                    tokens[peekPos].type != TokenType::RBRACKET &&
                    tokens[peekPos].type != TokenType::RPAREN &&
                    tokens[peekPos].type != TokenType::COMMA)
                {
                    consume(); // eat ')'
                    skipNewlines();
                    // Parse the operand (unary precedence — handles *ptr, &var, -x, etc.)
                    return parseUnary();
                }
            }
        }

        // ── C compound literal: (TypeName){field, field, ...} ──────────────
        // Only fire when the identifier looks like a type name (starts uppercase,
        // e.g. Room, Entity, Point). This prevents mis-firing on normal control-flow
        // conditions like while(cond) { } or if(flag) { }.
        if (check(TokenType::RPAREN) && expr->is<Identifier>())
        {
            const std::string &idName = expr->as<Identifier>().name;
            bool looksLikeType = !idName.empty() && std::isupper((unsigned char)idName[0]);

            if (looksLikeType)
            {
                // peek: is the token after ')' a '{'?
                size_t peekPos = pos + 1;
                while (peekPos < tokens.size() && tokens[peekPos].type == TokenType::NEWLINE)
                    ++peekPos;
                if (peekPos < tokens.size() && tokens[peekPos].type == TokenType::LBRACE)
                {
                    consume(); // eat ')'  — discard the cast type
                    skipNewlines();
                    int bln = current().line;
                    expect(TokenType::LBRACE, "Expected '{' in compound literal");
                    skipNewlines();
                    ArrayLiteral arr;
                    while (!check(TokenType::RBRACE) && !atEnd())
                    {
                        arr.elements.push_back(parseExpr());
                        skipNewlines();
                        if (!match(TokenType::COMMA))
                            break;
                        skipNewlines();
                    }
                    expect(TokenType::RBRACE, "Expected '}' in compound literal");
                    return std::make_unique<ASTNode>(std::move(arr), bln);
                }
            }
        }

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
    case TokenType::COUT:
    case TokenType::CIN:
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
    auto params = parseParamList(nullptr);
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

std::vector<std::string> Parser::parseParamList(std::vector<bool> *outIsRef)
{
    expect(TokenType::LPAREN, "Expected '('");
    std::vector<std::string> params;
    while (!check(TokenType::RPAREN) && !atEnd())
    {
        // C++ style: "const" modifier before type
        if (check(TokenType::CONST))
            consume(); // eat const

        // C-style: "int x" — type keyword before name
        if (isCTypeKeyword(current().type))
        {
            consume();
            while (isCTypeKeyword(current().type))
                consume(); // multi-word types
        }

        // C++ style: identifier type before name (e.g. "string name", "Entity *m", "Room &r")
        // Detect: IDENTIFIER followed by (BIT_AND or STAR or IDENTIFIER) — means it's a type name
        if (check(TokenType::IDENTIFIER))
        {
            // Peek ahead past any * and & qualifiers to find the actual name token
            size_t la = pos + 1;
            // Skip template arguments <...> in lookahead
            if (la < tokens.size() && tokens[la].type == TokenType::LT)
            {
                // Skip over template: unique_ptr<int[]>, shared_ptr<Foo>, etc.
                int tdepth = 0;
                while (la < tokens.size())
                {
                    if (tokens[la].type == TokenType::LT)
                        tdepth++;
                    else if (tokens[la].type == TokenType::GT)
                    {
                        tdepth--;
                        la++;
                        break;
                    }
                    else if (tokens[la].type == TokenType::RSHIFT)
                    {
                        tdepth -= 2;
                        la++;
                        break;
                    }
                    la++;
                }
            }
            while (la < tokens.size() &&
                   (tokens[la].type == TokenType::BIT_AND ||
                    tokens[la].type == TokenType::STAR ||
                    tokens[la].type == TokenType::CONST))
                la++;
            if (la < tokens.size() && tokens[la].type == TokenType::IDENTIFIER)
            {
                consume(); // eat type name (e.g. "Entity", "Room", "Cell", "string", "unique_ptr")
                // Skip template arguments: unique_ptr<int[]>, shared_ptr<Foo>, etc.
                if (check(TokenType::LT))
                {
                    consume(); // eat '<'
                    int tdepth = 1;
                    while (!atEnd() && tdepth > 0)
                    {
                        if (check(TokenType::LT))
                            tdepth++;
                        else if (check(TokenType::GT))
                            tdepth--;
                        else if (check(TokenType::RSHIFT))
                        {
                            tdepth -= 2;
                            consume();
                            continue;
                        }
                        consume();
                    }
                }
            }
        }

        // Detect whether this param is a pointer (*) or reference (&)
        bool isRef = false;
        while (check(TokenType::BIT_AND) || check(TokenType::STAR))
        {
            if (check(TokenType::BIT_AND))
                isRef = true; // & = pass-by-reference
            consume();
        }

        if (check(TokenType::IDENTIFIER))
        {
            params.push_back(consume().value);
            if (outIsRef)
                outIsRef->push_back(isRef);
        }
        else if (check(TokenType::THIS))
        {
            params.push_back(consume().value);
            if (outIsRef)
                outIsRef->push_back(false);
        }
        else if (check(TokenType::INPUT) || check(TokenType::PRINT) ||
                 check(TokenType::COUT) || check(TokenType::CIN))
        {
            // keyword tokens used as param names: e.g. void foo(int input, int* cout)
            params.push_back(consume().value);
            if (outIsRef)
                outIsRef->push_back(isRef);
        }
        else if (check(TokenType::COMMA) || check(TokenType::RPAREN))
        {
            // Unnamed parameter: e.g. void foo(int*, int) — just skip, no name to bind
            // Generate a placeholder name so param count stays consistent
            params.push_back("__unnamed_" + std::to_string(params.size()));
            if (outIsRef)
                outIsRef->push_back(isRef);
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
    // Consume any pointer stars and const qualifiers between type and name:
    // int* p  /  int *p  /  int* const p  /  const int* const p
    bool isPointer = false;
    while (check(TokenType::STAR) || check(TokenType::CONST))
    {
        if (check(TokenType::STAR))
            isPointer = true;
        consume();
    }
    // Accept IDENTIFIER or keyword tokens used as variable/param names
    Token nameToken = check(TokenType::IDENTIFIER) ? consume() : (check(TokenType::INPUT) || check(TokenType::PRINT)) ? consume()
                                                                                                                      : expect(TokenType::IDENTIFIER, "Expected variable name after type");
    // Skip C array dimension brackets: char map[ROWS][COLS], int arr[N], etc.
    while (check(TokenType::LBRACKET))
    {
        consume(); // eat '['
        int bdepth = 1;
        while (!atEnd() && bdepth > 0)
        {
            if (check(TokenType::LBRACKET))
                bdepth++;
            else if (check(TokenType::RBRACKET))
                bdepth--;
            consume();
        }
    }
    ASTNodePtr init;
    if (match(TokenType::ASSIGN))
        init = parseExpr();
    while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
        consume();
    auto decl = VarDecl{false, nameToken.value, std::move(init), typeHint};
    decl.isPointer = isPointer;
    auto node = std::make_unique<ASTNode>(std::move(decl), ln);
    // Consume trailing semicolon/newline only when NOT in a comma list
    // (the caller handles termination for multi-var declarations)
    // We stop here so the caller can check for ',' before consuming.
    // But if there's a semicolon right now and no comma coming, eat it.
    if (!check(TokenType::COMMA))
    {
        while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
            consume();
    }
    return node;
}