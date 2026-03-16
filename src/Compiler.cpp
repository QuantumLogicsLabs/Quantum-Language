#include "Compiler.h"
#include "Error.h"
#include <stdexcept>
#include <algorithm>
#include <unordered_map>

// ─── Global chunk registry (defined here, extern'd in VM.cpp) ────────────────
std::unordered_map<QuantumNative *, std::shared_ptr<Chunk>> g_chunkRegistry;

Compiler::Compiler() : current_(nullptr) {}

std::shared_ptr<Chunk> Compiler::compile(ASTNode &root)
{
    CompilerState top("<script>");
    current_ = &top;
    if (root.is<BlockStmt>())
        compileBlock(root.as<BlockStmt>());
    else
        compileNode(root);
    emit(Op::RETURN_NIL, 0, 0);
    return top.chunk;
}

void Compiler::beginScope() { current_->scopeDepth++; }

void Compiler::endScope(int line)
{
    current_->scopeDepth--;
    while (!current_->locals.empty() &&
           current_->locals.back().depth > current_->scopeDepth)
    {
        if (current_->locals.back().isCaptured)
            emit(Op::CLOSE_UPVALUE, 0, line);
        else
            emit(Op::POP, 0, line);
        current_->locals.pop_back();
    }
}

int Compiler::resolveLocal(CompilerState *state, const std::string &name)
{
    for (int i = static_cast<int>(state->locals.size()) - 1; i >= 0; --i)
        if (state->locals[i].name == name)
            return i;
    return -1;
}

int Compiler::addUpvalue(CompilerState *state, int index, bool isLocal)
{
    for (int i = 0; i < static_cast<int>(state->upvalues.size()); ++i)
        if (state->upvalues[i].index == index &&
            state->upvalues[i].isLocal == isLocal)
            return i;
    state->upvalues.push_back({isLocal, index});
    state->chunk->upvalueCount++;
    return static_cast<int>(state->upvalues.size()) - 1;
}

int Compiler::resolveUpvalue(CompilerState *state, const std::string &name)
{
    if (!state->enclosing)
        return -1;
    int local = resolveLocal(state->enclosing, name);
    if (local != -1)
    {
        state->enclosing->locals[local].isCaptured = true;
        return addUpvalue(state, local, true);
    }
    int upvalue = resolveUpvalue(state->enclosing, name);
    if (upvalue != -1)
        return addUpvalue(state, upvalue, false);
    return -1;
}

void Compiler::declareLocal(const std::string &name, int)
{
    if (current_->scopeDepth == 0)
        return;
    current_->locals.push_back({name, current_->scopeDepth, false});
}

void Compiler::emitLoad(const std::string &name, int line)
{
    int local = resolveLocal(current_, name);
    if (local != -1)
    {
        emit(Op::LOAD_LOCAL, local, line);
        return;
    }
    int uv = resolveUpvalue(current_, name);
    if (uv != -1)
    {
        emit(Op::LOAD_UPVALUE, uv, line);
        return;
    }
    emit(Op::LOAD_GLOBAL, addStr(name), line);
}

void Compiler::emitStore(const std::string &name, int line)
{
    int local = resolveLocal(current_, name);
    if (local != -1)
    {
        emit(Op::STORE_LOCAL, local, line);
        return;
    }
    int uv = resolveUpvalue(current_, name);
    if (uv != -1)
    {
        emit(Op::STORE_UPVALUE, uv, line);
        return;
    }
    emit(Op::STORE_GLOBAL, addStr(name), line);
}

void Compiler::beginLoop(int startIp)
{
    loops_.push_back({});
    loops_.back().loopStart = startIp;
}

void Compiler::emitBreak(int line)
{
    loops_.back().breakJumps.push_back(emitJump(Op::JUMP, line));
}

void Compiler::emitContinue(int line)
{
    loops_.back().continueJumps.push_back(emitJump(Op::JUMP, line));
}

void Compiler::endLoop()
{
    size_t after = chunk().code.size();
    for (size_t idx : loops_.back().breakJumps)
        chunk().patch(idx, static_cast<int32_t>(after) - static_cast<int32_t>(idx) - 1);
    loops_.pop_back();
}

// ─── Chunk-holder factory: registers chunk in g_chunkRegistry ────────────────
static std::shared_ptr<QuantumNative> makeChunkHolder(
    const std::string &fnName, std::shared_ptr<Chunk> ch)
{
    auto holder = std::make_shared<QuantumNative>();
    holder->name = "__chunk__" + fnName;
    auto cap = ch;
    holder->fn = [cap](std::vector<QuantumValue>) -> QuantumValue
    {
        (void)cap;
        return QuantumValue();
    };
    g_chunkRegistry[holder.get()] = ch;
    return holder;
}

// ─── Node dispatch ────────────────────────────────────────────────────────────

void Compiler::compileNode(ASTNode &node)
{
    int ln = node.line;
    std::visit([&](auto &n)
               {
        using T = std::decay_t<decltype(n)>;
        if constexpr (std::is_same_v<T, BlockStmt>)      compileBlock(n);
        else if constexpr (std::is_same_v<T, VarDecl>)        compileVarDecl(n, ln);
        else if constexpr (std::is_same_v<T, FunctionDecl>)   compileFunctionDecl(n, ln);
        else if constexpr (std::is_same_v<T, ClassDecl>)      compileClassDecl(n, ln);
        else if constexpr (std::is_same_v<T, IfStmt>)         compileIf(n, ln);
        else if constexpr (std::is_same_v<T, WhileStmt>)      compileWhile(n, ln);
        else if constexpr (std::is_same_v<T, ForStmt>)        compileFor(n, ln);
        else if constexpr (std::is_same_v<T, ReturnStmt>)     compileReturn(n, ln);
        else if constexpr (std::is_same_v<T, PrintStmt>)      compilePrint(n, ln);
        else if constexpr (std::is_same_v<T, InputStmt>)      compileInput(n, ln);
        else if constexpr (std::is_same_v<T, TryStmt>)        compileTry(n, ln);
        else if constexpr (std::is_same_v<T, RaiseStmt>)      compileRaise(n, ln);
        else if constexpr (std::is_same_v<T, BreakStmt>)      emitBreak(ln);
        else if constexpr (std::is_same_v<T, ContinueStmt>)   emitContinue(ln);
        else if constexpr (std::is_same_v<T, ImportStmt>)     { /* natives handle imports */ }
        else if constexpr (std::is_same_v<T, ExprStmt>)
        {
            compileExpr(*n.expr);
            emit(Op::POP, 0, ln);
        }
        else { compileExpr(node); emit(Op::POP, 0, ln); } }, node.node);
}

void Compiler::compileBlock(BlockStmt &b)
{
    for (auto &stmt : b.statements)
        compileNode(*stmt);
}

void Compiler::compileExpr(ASTNode &node)
{
    int ln = node.line;
    std::visit([&](auto &n)
               {
        using T = std::decay_t<decltype(n)>;
        if constexpr (std::is_same_v<T, NumberLiteral>)
            emit(Op::LOAD_CONST, addConst(QuantumValue(n.value)), ln);
        else if constexpr (std::is_same_v<T, StringLiteral>)
            emit(Op::LOAD_CONST, addConst(QuantumValue(n.value)), ln);
        else if constexpr (std::is_same_v<T, BoolLiteral>)
            emit(n.value ? Op::LOAD_TRUE : Op::LOAD_FALSE, 0, ln);
        else if constexpr (std::is_same_v<T, NilLiteral>)
            emit(Op::LOAD_NIL, 0, ln);
        else if constexpr (std::is_same_v<T, Identifier>)    compileIdentifier(n, ln);
        else if constexpr (std::is_same_v<T, BinaryExpr>)    compileBinary(n, ln);
        else if constexpr (std::is_same_v<T, UnaryExpr>)     compileUnary(n, ln);
        else if constexpr (std::is_same_v<T, AssignExpr>)    compileAssign(n, ln);
        else if constexpr (std::is_same_v<T, CallExpr>)      compileCall(n, ln);
        else if constexpr (std::is_same_v<T, IndexExpr>)     compileIndex(n, ln);
        else if constexpr (std::is_same_v<T, SliceExpr>)     compileSlice(n, ln);
        else if constexpr (std::is_same_v<T, MemberExpr>)    compileMember(n, ln);
        else if constexpr (std::is_same_v<T, ArrayLiteral>)  compileArray(n, ln);
        else if constexpr (std::is_same_v<T, DictLiteral>)   compileDict(n, ln);
        else if constexpr (std::is_same_v<T, TupleLiteral>)  compileTuple(n, ln);
        else if constexpr (std::is_same_v<T, LambdaExpr>)    compileLambda(n, ln);
        else if constexpr (std::is_same_v<T, TernaryExpr>)   compileTernary(n, ln);
        else if constexpr (std::is_same_v<T, ListComp>)      compileListComp(n, ln);
        else if constexpr (std::is_same_v<T, SuperExpr>)     compileSuper(n, ln);
        else if constexpr (std::is_same_v<T, NewExpr>)       compileNew(n, ln);
        else if constexpr (std::is_same_v<T, AddressOfExpr>) compileAddressOf(n, ln);
        else if constexpr (std::is_same_v<T, DerefExpr>)     compileDeref(n, ln);
        else if constexpr (std::is_same_v<T, ArrowExpr>)     compileArrow(n, ln);
        else throw std::runtime_error("Compiler: unhandled expression node"); }, node.node);
}

// ─── Statements ──────────────────────────────────────────────────────────────

void Compiler::compileVarDecl(VarDecl &s, int line)
{
    if (s.initializer)
        compileExpr(*s.initializer);
    else
        emit(Op::LOAD_NIL, 0, line);

    if (current_->scopeDepth == 0)
    {
        emit(s.isConst ? Op::DEFINE_CONST : Op::DEFINE_GLOBAL, addStr(s.name), line);
    }
    else
    {
        declareLocal(s.name, line);
        emit(Op::DEFINE_LOCAL, static_cast<int>(current_->locals.size()) - 1, line);
    }
}

void Compiler::compileFunctionDecl(FunctionDecl &s, int line)
{
    auto fnChunk = compileFunction(s.name, s.params, s.paramIsRef, s.defaultArgs, s.body.get(), line);
    auto holder = makeChunkHolder(s.name, fnChunk);
    emit(Op::LOAD_CONST, addConst(QuantumValue(holder)), line);
    emit(fnChunk->upvalueCount > 0 ? Op::MAKE_CLOSURE : Op::MAKE_FUNCTION, 0, line);
    if (current_->scopeDepth == 0)
    {
        emit(Op::DEFINE_GLOBAL, addStr(s.name), line);
    }
    else
    {
        declareLocal(s.name, line);
        emit(Op::DEFINE_LOCAL, static_cast<int>(current_->locals.size()) - 1, line);
    }
}

void Compiler::compileClassDecl(ClassDecl &s, int line)
{
    emit(Op::LOAD_CONST, addConst(QuantumValue(s.name)), line);
    emit(Op::MAKE_CLASS, 0, line);

    if (!s.base.empty())
    {
        emitLoad(s.base, line);
        emit(Op::INHERIT, 0, line);
    }

    for (auto &method : s.methods)
    {
        if (!method->is<FunctionDecl>())
            continue;
        auto &fd = method->as<FunctionDecl>();
        auto fnChunk = compileFunction(fd.name, fd.params, fd.paramIsRef, fd.defaultArgs, fd.body.get(), method->line);
        auto holder = makeChunkHolder(fd.name, fnChunk);
        emit(Op::LOAD_CONST, addConst(QuantumValue(holder)), method->line);
        emit(Op::MAKE_FUNCTION, 0, method->line);
        emit(Op::BIND_METHOD, addStr(fd.name), method->line);
    }

    if (current_->scopeDepth == 0)
    {
        emit(Op::DEFINE_GLOBAL, addStr(s.name), line);
    }
    else
    {
        declareLocal(s.name, line);
        emit(Op::DEFINE_LOCAL, static_cast<int>(current_->locals.size()) - 1, line);
    }
}

void Compiler::compileIf(IfStmt &s, int line)
{
    compileExpr(*s.condition);
    size_t thenJump = emitJump(Op::JUMP_IF_FALSE, line);
    emit(Op::POP, 0, line);
    compileNode(*s.thenBranch);
    size_t elseJump = emitJump(Op::JUMP, line);
    patchJump(thenJump);
    emit(Op::POP, 0, line);
    if (s.elseBranch)
        compileNode(*s.elseBranch);
    patchJump(elseJump);
}

void Compiler::compileWhile(WhileStmt &s, int line)
{
    int loopStart = static_cast<int>(chunk().code.size());
    beginLoop(loopStart);

    compileExpr(*s.condition);
    size_t exitJump = emitJump(Op::JUMP_IF_FALSE, line);
    emit(Op::POP, 0, line);

    beginScope();
    compileNode(*s.body);
    endScope(line);

    for (size_t ci : loops_.back().continueJumps)
        chunk().patch(ci, static_cast<int32_t>(chunk().code.size()) - static_cast<int32_t>(ci) - 1);

    emit(Op::LOOP, static_cast<int>(chunk().code.size()) - loopStart + 1, line);
    patchJump(exitJump);
    emit(Op::POP, 0, line);
    endLoop();
}

void Compiler::compileFor(ForStmt &s, int line)
{
    // Outer scope: holds the iterator as a hidden local (survives loop iterations)
    compileExpr(*s.iterable);
    emit(Op::MAKE_ITER, 0, line);

    beginScope(); // outer scope — iterator lives here
    declareLocal("__iter__", line);
    int iterSlot = static_cast<int>(current_->locals.size()) - 1;
    emit(Op::DEFINE_LOCAL, iterSlot, line);

    int loopStart = static_cast<int>(chunk().code.size());
    beginLoop(loopStart);

    // FOR_ITER: peeks iterator from stack top (it's the last outer-scope local).
    // If exhausted, jump past the loop. Otherwise push the next element.
    size_t exitJump = emitJump(Op::FOR_ITER, line);

    // Inner scope: holds the loop variable (popped at end of each iteration)
    beginScope();
    declareLocal(s.var, line);
    int varSlot = static_cast<int>(current_->locals.size()) - 1;
    emit(Op::DEFINE_LOCAL, varSlot, line);

    if (!s.var2.empty())
    {
        emit(Op::LOAD_LOCAL, varSlot, line);
        emit(Op::LOAD_CONST, addConst(QuantumValue(1.0)), line);
        emit(Op::GET_INDEX, 0, line);
        declareLocal(s.var2, line);
        emit(Op::DEFINE_LOCAL, static_cast<int>(current_->locals.size()) - 1, line);
        emit(Op::LOAD_LOCAL, varSlot, line);
        emit(Op::LOAD_CONST, addConst(QuantumValue(0.0)), line);
        emit(Op::GET_INDEX, 0, line);
        emit(Op::STORE_LOCAL, varSlot, line);
        emit(Op::POP, 0, line);
    }

    compileNode(*s.body);

    for (size_t ci : loops_.back().continueJumps)
        chunk().patch(ci, static_cast<int32_t>(chunk().code.size()) - static_cast<int32_t>(ci) - 1);

    // End inner scope: pops loop variable(s) only, leaving iterator on stack
    endScope(line);

    // Jump back to FOR_ITER (iterator is still on stack top)
    emit(Op::LOOP, static_cast<int>(chunk().code.size()) - loopStart + 1, line);

    patchJump(exitJump);
    // End outer scope: pops the iterator
    endScope(line);
    endLoop();
}

void Compiler::compileReturn(ReturnStmt &s, int line)
{
    if (s.value)
    {
        compileExpr(*s.value);
        emit(Op::RETURN, 0, line);
    }
    else
        emit(Op::RETURN_NIL, 0, line);
}

void Compiler::compilePrint(PrintStmt &s, int line)
{
    for (auto &arg : s.args)
        compileExpr(*arg);
    emit(Op::LOAD_CONST, addStr(s.sep), line);
    emit(Op::LOAD_CONST, addStr(s.end), line);
    emit(Op::PRINT, static_cast<int32_t>(s.args.size()), line);
}

void Compiler::compileInput(InputStmt &s, int line)
{
    emit(Op::LOAD_GLOBAL, addStr("__input__"), line);
    if (s.prompt)
        compileExpr(*s.prompt);
    else
        emit(Op::LOAD_CONST, addStr(""), line);
    emit(Op::CALL, 1, line);
    if (!s.target.empty())
    {
        emitStore(s.target, line);
        emit(Op::POP, 0, line);
    }
    else
        emit(Op::POP, 0, line);
}

void Compiler::compileTry(TryStmt &s, int line)
{
    size_t handlerJump = emitJump(Op::PUSH_HANDLER, line);
    if (s.body)
        compileNode(*s.body);
    emit(Op::POP_HANDLER, 0, line);
    size_t afterHandlers = emitJump(Op::JUMP, line);
    patchJump(handlerJump);

    for (auto &h : s.handlers)
    {
        beginScope();
        // alias = 'as e' syntax; errorType used as var name for 'except (e)' syntax
        std::string varName = h.alias.empty() ? h.errorType : h.alias;
        if (!varName.empty())
        {
            declareLocal(varName, line);
            emit(Op::DEFINE_LOCAL, static_cast<int>(current_->locals.size()) - 1, line);
        }
        else
        {
            emit(Op::POP, 0, line);
        }
        if (h.body)
            compileNode(*h.body);
        endScope(line);
    }

    patchJump(afterHandlers);
    if (s.finallyBody)
        compileNode(*s.finallyBody);
}

void Compiler::compileRaise(RaiseStmt &s, int line)
{
    if (s.value)
        compileExpr(*s.value);
    else
        emit(Op::LOAD_NIL, 0, line);
    emit(Op::RAISE, 0, line);
}

// ─── Expressions ─────────────────────────────────────────────────────────────

void Compiler::compileIdentifier(Identifier &e, int line) { emitLoad(e.name, line); }

void Compiler::compileBinary(BinaryExpr &e, int line)
{
    if (e.op == "and" || e.op == "&&")
    {
        compileExpr(*e.left);
        size_t sc = emitJump(Op::JUMP_IF_FALSE, line);
        emit(Op::POP, 0, line);
        compileExpr(*e.right);
        patchJump(sc);
        return;
    }
    if (e.op == "or" || e.op == "||")
    {
        compileExpr(*e.left);
        size_t sc = emitJump(Op::JUMP_IF_TRUE, line);
        emit(Op::POP, 0, line);
        compileExpr(*e.right);
        patchJump(sc);
        return;
    }

    compileExpr(*e.left);
    compileExpr(*e.right);

    static const std::unordered_map<std::string, Op> opMap = {
        {"+", Op::ADD},
        {"-", Op::SUB},
        {"*", Op::MUL},
        {"/", Op::DIV},
        {"%", Op::MOD},
        {"//", Op::FLOOR_DIV},
        {"**", Op::POW},
        {"==", Op::EQ},
        {"!=", Op::NEQ},
        {"===", Op::EQ},
        {"!==", Op::NEQ},
        {"<", Op::LT},
        {"<=", Op::LTE},
        {">", Op::GT},
        {">=", Op::GTE},
        {"&", Op::BIT_AND},
        {"|", Op::BIT_OR},
        {"^", Op::BIT_XOR},
        {"<<", Op::LSHIFT},
        {">>", Op::RSHIFT},
        {"is", Op::EQ},
    };
    auto it = opMap.find(e.op);
    if (it != opMap.end())
        emit(it->second, 0, line);
    else
        throw std::runtime_error("Compiler: unknown binary op '" + e.op + "'");
}

void Compiler::compileUnary(UnaryExpr &e, int line)
{
    compileExpr(*e.operand);
    if (e.op == "-")
        emit(Op::NEG, 0, line);
    else if (e.op == "!" ||
             e.op == "not")
        emit(Op::NOT, 0, line);
    else if (e.op == "~")
        emit(Op::BIT_NOT, 0, line);
    else if (e.op == "++" || e.op == "--")
    {
        emit(Op::LOAD_CONST, addConst(QuantumValue(1.0)), line);
        emit(e.op == "++" ? Op::ADD : Op::SUB, 0, line);
        if (e.operand->is<Identifier>())
        {
            emit(Op::DUP, 0, line);
            emitStore(e.operand->as<Identifier>().name, line);
            emit(Op::POP, 0, line);
        }
    }
    else
        throw std::runtime_error("Compiler: unknown unary op '" + e.op + "'");
}

void Compiler::compileAssign(AssignExpr &e, int line)
{
    bool compound = (e.op != "=");

    static const std::unordered_map<std::string, Op> cops = {
        {"+=", Op::ADD},
        {"-=", Op::SUB},
        {"*=", Op::MUL},
        {"/=", Op::DIV},
        {"%=", Op::MOD},
        {"&=", Op::BIT_AND},
        {"|=", Op::BIT_OR},
        {"^=", Op::BIT_XOR},
    };

    if (e.target->is<Identifier>())
    {
        const std::string &name = e.target->as<Identifier>().name;
        if (compound)
            emitLoad(name, line);
        compileExpr(*e.value);
        if (compound)
        {
            auto it = cops.find(e.op);
            if (it != cops.end())
                emit(it->second, 0, line);
        }
        emit(Op::DUP, 0, line);
        emitStore(name, line);
        emit(Op::POP, 0, line);
        return;
    }

    if (e.target->is<IndexExpr>())
    {
        auto &idx = e.target->as<IndexExpr>();
        // VM SET_INDEX expects stack: val (bottom), obj, key (top)
        compileExpr(*e.value);    // val  <- bottom
        compileExpr(*idx.object); // obj
        compileExpr(*idx.index);  // key  <- top
        emit(Op::SET_INDEX, 0, line);
        // SET_INDEX pushes val back as the expression result
        return;
    }

    if (e.target->is<MemberExpr>())
    {
        auto &mem = e.target->as<MemberExpr>();
        // VM SET_MEMBER: pops val (top), peeks obj (does not pop obj)
        // After SET_MEMBER: obj still on stack
        compileExpr(*mem.object); // stack: obj
        compileExpr(*e.value);    // stack: obj val
        emit(Op::SET_MEMBER, addStr(mem.member), line);
        // stack: obj  (val was popped by SET_MEMBER, obj remains)
        // For use as expression result: swap obj for a copy of val
        // Simplest: just leave obj on stack — caller (ExprStmt) pops it anyway
        // If used as expression, obj != val but that's an edge case
        return;
    }

    // Fallback: evaluate rhs and leave on stack
    compileExpr(*e.value);
}

void Compiler::compileCall(CallExpr &e, int line)
{
    // super.method(args) -- special case
    if (e.callee->is<MemberExpr>())
    {
        auto &mem = e.callee->as<MemberExpr>();
        if (mem.object->is<SuperExpr>())
        {
            // Load self (slot 0), GET_SUPER method, push args, CALL
            emitLoad("self", line);
            emit(Op::GET_SUPER, addStr(mem.member), line);
            for (auto &arg : e.args)
                compileExpr(*arg);
            emit(Op::CALL, static_cast<int32_t>(e.args.size()), line);
            return;
        }
        // Regular method call: obj.method(args)
        compileExpr(*mem.object);
        emit(Op::GET_MEMBER, addStr(mem.member), line);
        for (auto &arg : e.args)
            compileExpr(*arg);
        emit(Op::CALL, static_cast<int32_t>(e.args.size()), line);
        return;
    }
    // Regular call
    compileExpr(*e.callee);
    for (auto &arg : e.args)
        compileExpr(*arg);
    emit(Op::CALL, static_cast<int32_t>(e.args.size()), line);
}

void Compiler::compileIndex(IndexExpr &e, int line)
{
    compileExpr(*e.object);
    compileExpr(*e.index);
    emit(Op::GET_INDEX, 0, line);
}

void Compiler::compileSlice(SliceExpr &e, int line)
{
    emit(Op::LOAD_GLOBAL, addStr("__slice__"), line);
    compileExpr(*e.object);
    if (e.start)
        compileExpr(*e.start);
    else
        emit(Op::LOAD_NIL, 0, line);
    if (e.stop)
        compileExpr(*e.stop);
    else
        emit(Op::LOAD_NIL, 0, line);
    if (e.step)
        compileExpr(*e.step);
    else
        emit(Op::LOAD_NIL, 0, line);
    emit(Op::CALL, 4, line);
}

void Compiler::compileMember(MemberExpr &e, int line)
{
    compileExpr(*e.object);
    emit(Op::GET_MEMBER, addStr(e.member), line);
}

void Compiler::compileArray(ArrayLiteral &e, int line)
{
    for (auto &el : e.elements)
        compileExpr(*el);
    emit(Op::MAKE_ARRAY, static_cast<int32_t>(e.elements.size()), line);
}

void Compiler::compileDict(DictLiteral &e, int line)
{
    for (auto &[k, v] : e.pairs)
    {
        compileExpr(*k);
        compileExpr(*v);
    }
    emit(Op::MAKE_DICT, static_cast<int32_t>(e.pairs.size()), line);
}

void Compiler::compileTuple(TupleLiteral &e, int line)
{
    for (auto &el : e.elements)
        compileExpr(*el);
    emit(Op::MAKE_TUPLE, static_cast<int32_t>(e.elements.size()), line);
}

void Compiler::compileLambda(LambdaExpr &e, int line)
{
    std::vector<bool> noRef(e.params.size(), false);
    auto fnChunk = compileFunction("lambda", e.params, noRef, e.defaultArgs, e.body.get(), line);
    auto holder = makeChunkHolder("lambda", fnChunk);
    emit(Op::LOAD_CONST, addConst(QuantumValue(holder)), line);
    emit(fnChunk->upvalueCount > 0 ? Op::MAKE_CLOSURE : Op::MAKE_FUNCTION, 0, line);
}

void Compiler::compileTernary(TernaryExpr &e, int line)
{
    compileExpr(*e.condition);
    size_t elseJump = emitJump(Op::JUMP_IF_FALSE, line);
    emit(Op::POP, 0, line);
    compileExpr(*e.thenExpr);
    size_t endJump = emitJump(Op::JUMP, line);
    patchJump(elseJump);
    emit(Op::POP, 0, line);
    compileExpr(*e.elseExpr);
    patchJump(endJump);
}

void Compiler::compileListComp(ListComp &e, int line)
{
    // Build result array as a local variable so we can load it inside the loop
    beginScope();

    // slot 0: result array
    emit(Op::MAKE_ARRAY, 0, line);
    declareLocal("__result__", line);
    int resultSlot = static_cast<int>(current_->locals.size()) - 1;
    emit(Op::DEFINE_LOCAL, resultSlot, line);

    // Outer scope for iterator (slot 1)
    beginScope();
    compileExpr(*e.iterable);
    emit(Op::MAKE_ITER, 0, line);
    declareLocal("__iter__", line);
    int iterSlot = static_cast<int>(current_->locals.size()) - 1;
    emit(Op::DEFINE_LOCAL, iterSlot, line);

    int loopStart = static_cast<int>(chunk().code.size());
    beginLoop(loopStart);
    size_t exitJump = emitJump(Op::FOR_ITER, line);

    // Inner scope: loop variables
    beginScope();
    for (auto &v : e.vars)
    {
        declareLocal(v, line);
        emit(Op::DEFINE_LOCAL, static_cast<int>(current_->locals.size()) - 1, line);
    }

    // Helper: load result array, load value, call push
    auto pushToResult = [&]()
    {
        // val is on stack top. We need: push_fn (callee), val (arg).
        // GET_MEMBER on array creates a bound native capturing the array.
        // Sequence: val is on stack, load array, GET_MEMBER "push" -> push_fn on stack.
        // Stack now: val, push_fn. SWAP -> push_fn, val. CALL 1.
        emit(Op::LOAD_LOCAL, resultSlot, line);     // stack: ..., val, array
        emit(Op::GET_MEMBER, addStr("push"), line); // stack: ..., val, push_fn
        emit(Op::SWAP, 0, line);                    // stack: ..., push_fn, val
        emit(Op::CALL, 1, line);                    // calls push_fn(val)
        emit(Op::POP, 0, line);                     // discard return value
    };

    if (e.condition)
    {
        compileExpr(*e.condition);
        size_t skipJump = emitJump(Op::JUMP_IF_FALSE, line);
        emit(Op::POP, 0, line);
        compileExpr(*e.expr);
        pushToResult();
        size_t jmp = emitJump(Op::JUMP, line);
        patchJump(skipJump);
        emit(Op::POP, 0, line);
        patchJump(jmp);
    }
    else
    {
        compileExpr(*e.expr);
        pushToResult();
    }

    for (size_t ci : loops_.back().continueJumps)
        chunk().patch(ci, static_cast<int32_t>(chunk().code.size()) -
                              static_cast<int32_t>(ci) - 1);

    endScope(line); // pop loop vars
    emit(Op::LOOP, static_cast<int>(chunk().code.size()) - loopStart + 1, line);
    patchJump(exitJump);
    endScope(line); // pop iterator
    endLoop();

    // Load result array, then end outer scope (which would pop result,
    // but we want to leave it on stack). So load it, end scope (pops slot),
    // but result is already pushed above the scope. Use DUP before endScope.
    emit(Op::LOAD_LOCAL, resultSlot, line);
    endScope(line); // pops result local from stack, but we just loaded a copy
}

void Compiler::compileSuper(SuperExpr &e, int line)
{
    // Standalone super() or super.method access (not a call)
    // For super.method() calls, compileCall handles it directly.
    emitLoad("self", line);
    if (!e.method.empty())
        emit(Op::GET_SUPER, addStr(e.method), line);
}

void Compiler::compileNew(NewExpr &e, int line)
{
    emitLoad(e.typeName, line);
    for (auto &arg : e.args)
        compileExpr(*arg);
    emit(Op::INSTANCE_NEW, static_cast<int32_t>(e.args.size()), line);
}

void Compiler::compileAddressOf(AddressOfExpr &e, int line)
{
    compileExpr(*e.operand);
    emit(Op::ADDRESS_OF, 0, line);
}

void Compiler::compileDeref(DerefExpr &e, int line)
{
    compileExpr(*e.operand);
    emit(Op::DEREF, 0, line);
}

void Compiler::compileArrow(ArrowExpr &e, int line)
{
    compileExpr(*e.object);
    emit(Op::DEREF, 0, line);
    emit(Op::GET_MEMBER, addStr(e.member), line);
}

// ─── compileFunction ─────────────────────────────────────────────────────────

std::shared_ptr<Chunk> Compiler::compileFunction(
    const std::string &name,
    const std::vector<std::string> &params,
    const std::vector<bool> &paramIsRef,
    const std::vector<ASTNodePtr> &,
    ASTNode *body,
    int line)
{
    CompilerState fnState(name, current_);
    fnState.isFunction = true;
    CompilerState *prev = current_;
    current_ = &fnState;

    beginScope();
    for (auto &p : params)
        declareLocal(p, line);

    fnState.chunk->params = params;
    fnState.chunk->paramIsRef = paramIsRef.empty()
                                    ? std::vector<bool>(params.size(), false)
                                    : paramIsRef;

    if (body)
    {
        if (body->is<BlockStmt>())
            compileBlock(body->as<BlockStmt>());
        else
        {
            compileExpr(*body);
            emit(Op::RETURN, 0, line);
        }
    }
    emit(Op::RETURN_NIL, 0, line);
    endScope(line);

    auto result = fnState.chunk;
    result->upvalueCount = static_cast<int>(fnState.upvalues.size());

    // Pack upvalue descriptors as the last constant for MAKE_CLOSURE
    auto uvDescs = std::make_shared<Array>();
    for (auto &uv : fnState.upvalues)
    {
        auto desc = std::make_shared<Array>();
        desc->push_back(QuantumValue(uv.isLocal ? 1.0 : 0.0));
        desc->push_back(QuantumValue(static_cast<double>(uv.index)));
        uvDescs->push_back(QuantumValue(desc));
    }
    result->constants.push_back(QuantumValue(uvDescs));

    current_ = prev;
    return result;
}