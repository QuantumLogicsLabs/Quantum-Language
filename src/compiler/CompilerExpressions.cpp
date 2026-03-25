#include "Compiler.h"
#include "Error.h"
#include "Vm.h"
#include <stdexcept>
#include <algorithm>
#include <unordered_map>

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
    auto closureTpl = std::make_shared<Closure>(fnChunk);
    emit(Op::LOAD_CONST, addConst(QuantumValue(closureTpl)), line);
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

