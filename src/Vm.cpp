#include "VM.h"
#include "Error.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <functional>
#include <regex>
#include <random>
#include <chrono>
#include <limits>
#include <cassert>
#include <unordered_set>

// Defined in main.cpp — true during --test runs so input() returns ""
// immediately instead of blocking on stdin.
extern bool g_testMode;

// Defined in Compiler.cpp — maps QuantumNative* chunk-holders to their Chunk
extern std::unordered_map<QuantumNative *, std::shared_ptr<Chunk>> g_chunkRegistry;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif

// ─── Iterator state tag stored inside a QuantumNative ────────────────────────
// We encode iterators as a QuantumNative whose fn() never gets called;
// the VM identifies them by name prefix "__iter__" and stores an IterState
// keyed by raw pointer.

// ─── Constructor ─────────────────────────────────────────────────────────────

VM::VM()
{
    globals = std::make_shared<Environment>();
    registerNatives();
}

// ─── Run ─────────────────────────────────────────────────────────────────────

void VM::run(std::shared_ptr<Chunk> chunk)
{
    stepCount_ = 0;
    pendingInstances_.clear();
    stack_.clear();
    frames_.clear();
    handlers_.clear();

    // Create a top-level closure
    auto closure = std::make_shared<Closure>(chunk);
    frames_.push_back({closure, 0, 0});
    runFrame(0);
}

// ─── Stack helpers ────────────────────────────────────────────────────────────

void VM::push(QuantumValue v)
{
    stack_.push_back(std::move(v));
}

QuantumValue VM::pop()
{
    if (stack_.empty())
        throw RuntimeError("VM stack underflow");
    QuantumValue v = std::move(stack_.back());
    stack_.pop_back();
    return v;
}

QuantumValue &VM::peek(int offset)
{
    return stack_[stack_.size() - 1 - offset];
}

void VM::runtimeError(const std::string &msg, int line)
{
    throw RuntimeError(msg, line);
}

double VM::toNumber(const QuantumValue &v, const std::string &ctx, int line)
{
    if (v.isNumber())
        return v.asNumber();
    if (v.isString())
    {
        try
        {
            return std::stod(v.asString());
        }
        catch (...)
        {
        }
    }
    throw TypeError("Expected number in " + ctx + ", got " + v.typeName(), line);
}

// ─── Value equality ───────────────────────────────────────────────────────────

bool VM::valuesEqual(const QuantumValue &a, const QuantumValue &b)
{
    if (a.isNil() && b.isNil())
        return true;
    if (a.isBool() && b.isBool())
        return a.asBool() == b.asBool();
    if (a.isNumber() && b.isNumber())
        return a.asNumber() == b.asNumber();
    if (a.isString() && b.isString())
        return a.asString() == b.asString();
    if (a.isArray() && b.isArray())
        return a.asArray() == b.asArray(); // ptr eq
    return false;
}

// ─── Binary / unary execution ────────────────────────────────────────────────

QuantumValue VM::execBinary(Op op, const QuantumValue &L, const QuantumValue &R, int line)
{
    // String concatenation
    if (op == Op::ADD && (L.isString() || R.isString()))
        return QuantumValue(L.toString() + R.toString());

    // Array concatenation
    if (op == Op::ADD && L.isArray() && R.isArray())
    {
        auto arr = std::make_shared<Array>(*L.asArray());
        for (auto &v : *R.asArray())
            arr->push_back(v);
        return QuantumValue(arr);
    }

    // Comparison operators — allow mixed types
    if (op == Op::EQ)
        return QuantumValue(valuesEqual(L, R));
    if (op == Op::NEQ)
        return QuantumValue(!valuesEqual(L, R));

    // Numeric arithmetic
    double l = 0, r = 0;
    bool hasNum = L.isNumber() || R.isNumber();

    if (L.isNumber())
        l = L.asNumber();
    else if (L.isString())
    {
        try
        {
            l = std::stod(L.asString());
        }
        catch (...)
        {
            l = 0;
        }
    }
    else if (L.isBool())
        l = L.asBool() ? 1.0 : 0.0;

    if (R.isNumber())
        r = R.asNumber();
    else if (R.isString())
    {
        try
        {
            r = std::stod(R.asString());
        }
        catch (...)
        {
            r = 0;
        }
    }
    else if (R.isBool())
        r = R.asBool() ? 1.0 : 0.0;

    switch (op)
    {
    case Op::ADD:
        return QuantumValue(l + r);
    case Op::SUB:
        return QuantumValue(l - r);
    case Op::MUL:
        // String repeat: "abc" * 3
        if (L.isString() && R.isNumber())
        {
            std::string s;
            for (int i = 0; i < (int)r; i++)
                s += L.asString();
            return QuantumValue(s);
        }
        return QuantumValue(l * r);
    case Op::DIV:
        if (r == 0)
            throw RuntimeError("Division by zero", line);
        return QuantumValue(l / r);
    case Op::MOD:
        if (r == 0)
            throw RuntimeError("Modulo by zero", line);
        return QuantumValue(std::fmod(l, r));
    case Op::FLOOR_DIV:
        if (r == 0)
            throw RuntimeError("Division by zero", line);
        return QuantumValue(std::floor(l / r));
    case Op::POW:
        return QuantumValue(std::pow(l, r));
    case Op::LT:
        return QuantumValue(l < r);
    case Op::LTE:
        return QuantumValue(l <= r);
    case Op::GT:
        return QuantumValue(l > r);
    case Op::GTE:
        return QuantumValue(l >= r);
    case Op::BIT_AND:
        return QuantumValue((double)((long long)l & (long long)r));
    case Op::BIT_OR:
        return QuantumValue((double)((long long)l | (long long)r));
    case Op::BIT_XOR:
        return QuantumValue((double)((long long)l ^ (long long)r));
    case Op::LSHIFT:
        return QuantumValue((double)((long long)l << (int)r));
    case Op::RSHIFT:
        return QuantumValue((double)((long long)l >> (int)r));
    default:
        throw RuntimeError("Unknown binary op", line);
    }
    (void)hasNum;
}

QuantumValue VM::execUnary(Op op, const QuantumValue &v, int line)
{
    switch (op)
    {
    case Op::NEG:
        if (v.isNumber())
            return QuantumValue(-v.asNumber());
        throw TypeError("Unary - on " + v.typeName(), line);
    case Op::NOT:
        return QuantumValue(!v.isTruthy());
    case Op::BIT_NOT:
        if (v.isNumber())
            return QuantumValue((double)(~(long long)v.asNumber()));
        throw TypeError("Bitwise ~ on " + v.typeName(), line);
    default:
        throw RuntimeError("Unknown unary op", line);
    }
}

// ─── Upvalue management ───────────────────────────────────────────────────────

std::shared_ptr<Upvalue> VM::captureUpvalue(size_t stackIdx)
{
    // Check if we already have an open upvalue for this slot
    for (auto &uv : openUpvalues_)
        if (uv->cell.get() == &stack_[stackIdx])
            return uv;

    // Create a new open upvalue pointing directly into the stack
    // We use a shared_ptr alias to avoid copying
    auto cell = std::shared_ptr<QuantumValue>(
        std::shared_ptr<QuantumValue>(), &stack_[stackIdx]);
    auto uv = std::make_shared<Upvalue>(cell);
    openUpvalues_.push_back(uv);
    return uv;
}

void VM::closeUpvalues(size_t fromIdx)
{
    for (auto it = openUpvalues_.begin(); it != openUpvalues_.end();)
    {
        auto &uv = *it;
        // If the cell points into stack at or above fromIdx, close it
        if (uv->cell.get() >= &stack_[fromIdx])
        {
            uv->closed = *uv->cell;
            uv->cell = std::shared_ptr<QuantumValue>(
                std::shared_ptr<QuantumValue>(), &uv->closed);
            it = openUpvalues_.erase(it);
        }
        else
            ++it;
    }
}

// ─── Call helpers ─────────────────────────────────────────────────────────────

void VM::callValue(QuantumValue callee, int argCount, int line)
{
    if (callee.isNative())
    {
        callNativeFn(callee.asNative(), argCount, line);
        return;
    }
    if (callee.isClass())
    {
        callClass(callee.asClass(), argCount, line);
        return;
    }
    if (callee.isFunction())
    {
        // QuantumFunction wrapping a Chunk (MAKE_FUNCTION/CLOSURE result)
        // The VM stores closures as QuantumNative with name "__closure__"
        throw RuntimeError("Direct QuantumFunction call not supported — use Closure", line);
    }

    // Check for closure wrapper
    auto *nat = std::get_if<std::shared_ptr<QuantumNative>>(&callee.data);
    if (nat && (*nat)->name.substr(0, 11) == "__closure__")
    {
        // Extract closure from fn capture
        // (The closure is stored as a raw ptr in a separate map keyed by native ptr)
        throw RuntimeError("Closure map not yet wired — see VM::callClosure", line);
    }

    throw TypeError("Cannot call value of type " + callee.typeName(), line);
}

// ─── Chunk-in-native extraction helper ───────────────────────────────────────
// We store Closures in a side-table indexed by QuantumNative pointer.
static std::unordered_map<QuantumNative *, std::shared_ptr<Closure>> s_closureTable;

// Bound method table: maps bound-method native ptr -> {closure, self value}
struct BoundMethod
{
    std::shared_ptr<Closure> closure;
    QuantumValue self;
};
static std::unordered_map<QuantumNative *, BoundMethod> s_boundTable;

void VM::callClosure(std::shared_ptr<Closure> closure, int argCount, int line)
{
    auto &ch = *closure->chunk;

    if (argCount < (int)ch.params.size())
    {
        // Fill missing args with nil (default arg logic simplified)
        while ((int)stack_.size() - argCount < (int)ch.params.size())
        {
            push(QuantumValue());
            argCount++;
        }
    }

    size_t stackBase = stack_.size() - argCount;
    frames_.push_back({closure, 0, stackBase});
}

void VM::callNativeFn(std::shared_ptr<QuantumNative> fn, int argCount, int line)
{
    std::vector<QuantumValue> args;
    args.reserve(argCount);
    for (int i = 0; i < argCount; ++i)
        args.push_back(stack_[stack_.size() - argCount + i]);

    for (int i = 0; i < argCount; ++i)
        stack_.pop_back();

    QuantumValue result;
    try
    {
        result = fn->fn(args);
    }
    catch (QuantumError &)
    {
        throw;
    }
    catch (std::exception &e)
    {
        throw RuntimeError(e.what(), line);
    }

    push(std::move(result));
}

void VM::callClass(std::shared_ptr<QuantumClass> klass, int argCount, int line)
{
    auto inst = std::make_shared<QuantumInstance>();
    inst->klass = klass;
    inst->env = std::make_shared<Environment>(globals);

    // Look for __init__ / init
    auto *k = klass.get();
    std::shared_ptr<QuantumFunction> initFn;
    while (k)
    {
        auto it = k->methods.find("__init__");
        if (it != k->methods.end())
        {
            initFn = it->second;
            break;
        }
        it = k->methods.find("init");
        if (it != k->methods.end())
        {
            initFn = it->second;
            break;
        }
        k = k->base.get();
    }

    QuantumValue instVal(inst);

    if (initFn)
    {
        // Find closure for this function
        auto *fn = initFn.get();
        auto cit = s_closureTable.find(reinterpret_cast<QuantumNative *>(fn));
        if (cit != s_closureTable.end())
        {
            // Push self as first arg
            std::vector<QuantumValue> newArgs = {instVal};
            for (int i = argCount - 1; i >= 0; --i)
                newArgs.push_back(stack_[stack_.size() - argCount + i]);
            for (int i = 0; i < argCount; ++i)
                stack_.pop_back();
            for (auto &a : newArgs)
                push(a);
            callClosure(cit->second, (int)newArgs.size(), line);
            return;
        }
    }

    // No init — pop args, push instance
    for (int i = 0; i < argCount; ++i)
        stack_.pop_back();
    push(instVal);
}

// ─── Built-in method dispatch ─────────────────────────────────────────────────

QuantumValue VM::callBuiltinMethod(QuantumValue &obj, const std::string &method,
                                   std::vector<QuantumValue> args, int line)
{
    if (obj.isArray())
        return callArrayMethod(obj.asArray(), method, args);
    if (obj.isString())
        return callStringMethod(obj.asString(), method, args);
    if (obj.isDict())
        return callDictMethod(obj.asDict(), method, args);
    if (obj.isInstance())
    {
        auto inst = obj.asInstance();
        // Find method in class hierarchy
        auto *k = inst->klass.get();
        while (k)
        {
            auto it = k->methods.find(method);
            if (it != k->methods.end())
            {
                auto fn = it->second;
                auto fnPtr = reinterpret_cast<QuantumNative *>(fn.get());
                auto cit = s_closureTable.find(fnPtr);
                if (cit != s_closureTable.end())
                {
                    // Push self + args
                    push(obj);
                    for (auto &a : args)
                        push(a);
                    callClosure(cit->second, (int)args.size() + 1, line);
                    runFrame();
                    return pop();
                }
                break;
            }
            k = k->base.get();
        }
        // Check fields
        try
        {
            return inst->getField(method);
        }
        catch (...)
        {
        }
    }
    throw TypeError("No method '" + method + "' on " + obj.typeName(), line);
}

// ─── Main execution loop ──────────────────────────────────────────────────────

void VM::runFrame(size_t stopDepth)
{
    while (frames_.size() > stopDepth)
    {
        CallFrame &frame = frames_.back();
        auto &code = frame.closure->chunk->code;
        auto &consts = frame.closure->chunk->constants;

        if (frame.ip >= code.size())
        {
            // Function fell off the end
            size_t base = frame.stackBase;
            frames_.pop_back();
            // Trim stack back to base, push nil
            while (stack_.size() > base)
                stack_.pop_back();
            push(QuantumValue());
            continue;
        }

        if (++stepCount_ > MAX_STEPS)
            throw RuntimeError("Execution step limit exceeded (possible infinite loop)");

        const Instruction &instr = code[frame.ip++];
        int line = instr.line;

        switch (instr.op)
        {
        // ── Constants & stack ──────────────────────────────────────────────
        case Op::LOAD_CONST:
            push(consts[instr.operand]);
            break;
        case Op::LOAD_NIL:
            push(QuantumValue());
            break;
        case Op::LOAD_TRUE:
            push(QuantumValue(true));
            break;
        case Op::LOAD_FALSE:
            push(QuantumValue(false));
            break;
        case Op::POP:
            pop();
            break;
        case Op::DUP:
            push(peek(0));
            break;
        case Op::SWAP:
        {
            QuantumValue a = pop(), b = pop();
            push(a);
            push(b);
            break;
        }
        case Op::NOP:
            break;

        // ── Globals ───────────────────────────────────────────────────────
        case Op::DEFINE_GLOBAL:
        {
            const std::string &name = consts[instr.operand].asString();
            globals->define(name, pop());
            break;
        }
        case Op::DEFINE_CONST:
        {
            const std::string &name = consts[instr.operand].asString();
            globals->define(name, pop(), true);
            break;
        }
        case Op::LOAD_GLOBAL:
        {
            const std::string &name = consts[instr.operand].asString();
            try
            {
                push(globals->get(name));
            }
            catch (NameError &)
            {
                push(QuantumValue());
            } // return nil for missing
            break;
        }
        case Op::STORE_GLOBAL:
        {
            const std::string &name = consts[instr.operand].asString();
            if (globals->has(name))
                globals->set(name, peek(0));
            else
                globals->define(name, peek(0));
            break;
        }

        // ── Locals ────────────────────────────────────────────────────────
        case Op::DEFINE_LOCAL:
            // Value already on stack; slot is stack_[stackBase + operand]
            // Nothing to do — the value stays at its position.
            break;
        case Op::LOAD_LOCAL:
        {
            size_t idx = frame.stackBase + instr.operand;
            if (idx < stack_.size())
                push(stack_[idx]);
            else
                push(QuantumValue());
            break;
        }
        case Op::STORE_LOCAL:
        {
            size_t idx = frame.stackBase + instr.operand;
            while (stack_.size() <= idx)
                stack_.push_back(QuantumValue());
            stack_[idx] = peek(0);
            break;
        }

        // ── Upvalues ──────────────────────────────────────────────────────
        case Op::LOAD_UPVALUE:
        {
            auto &uv = frame.closure->upvalues[instr.operand];
            push(uv->get());
            break;
        }
        case Op::STORE_UPVALUE:
        {
            auto &uv = frame.closure->upvalues[instr.operand];
            uv->set(peek(0));
            break;
        }
        case Op::CLOSE_UPVALUE:
        {
            closeUpvalues(stack_.size() - 1);
            pop();
            break;
        }

        // ── Arithmetic ────────────────────────────────────────────────────
        case Op::ADD:
        case Op::SUB:
        case Op::MUL:
        case Op::DIV:
        case Op::MOD:
        case Op::FLOOR_DIV:
        case Op::POW:
        case Op::BIT_AND:
        case Op::BIT_OR:
        case Op::BIT_XOR:
        case Op::LSHIFT:
        case Op::RSHIFT:
        case Op::EQ:
        case Op::NEQ:
        case Op::LT:
        case Op::LTE:
        case Op::GT:
        case Op::GTE:
        {
            QuantumValue R = pop(), L = pop();
            push(execBinary(instr.op, L, R, line));
            break;
        }

        case Op::NEG:
        case Op::NOT:
        case Op::BIT_NOT:
        {
            QuantumValue v = pop();
            push(execUnary(instr.op, v, line));
            break;
        }

        // ── Control flow ──────────────────────────────────────────────────
        case Op::JUMP:
            frame.ip += instr.operand;
            break;
        case Op::JUMP_IF_FALSE:
            if (!peek(0).isTruthy())
                frame.ip += instr.operand;
            break;
        case Op::JUMP_IF_TRUE:
            if (peek(0).isTruthy())
                frame.ip += instr.operand;
            break;
        case Op::LOOP:
            frame.ip -= instr.operand;
            break;
        case Op::JUMP_ABSOLUTE:
            frame.ip = instr.operand;
            break;

        // ── Functions ─────────────────────────────────────────────────────
        case Op::MAKE_FUNCTION:
        case Op::MAKE_CLOSURE:
        {
            // Top of stack is a QuantumNative holding the chunk
            QuantumValue top = pop();
            if (!top.isNative())
                throw RuntimeError("MAKE_FUNCTION expects chunk holder on stack", line);
            auto holder = top.asNative();
            // The chunk is captured in the native's lambda; we need to extract it.
            // Convention: we store the Chunk* in a side-table keyed by native name.
            // Find the chunk in the table:
            static std::unordered_map<std::string, std::shared_ptr<Chunk>> s_chunkTable;
            auto &chunkName = holder->name; // "__chunk__funcname"
            // Retrieve chunk via the lambda capture: call fn with a sentinel
            // Actually we stored it using a global side-table during compilation.
            // We use a trick: the lambda captures a shared_ptr<Chunk>.
            // We recover it by calling the lambda with a special "get_chunk" arg.
            // This is fragile — better design: store as a variant or custom type.
            // Retrieve chunk from global registry
            auto cit = g_chunkRegistry.find(holder.get());
            if (cit == g_chunkRegistry.end())
                throw RuntimeError("Chunk not found in registry for " + chunkName, line);

            auto closure = std::make_shared<Closure>(cit->second);
            closure->name = cit->second->name;

            // Capture upvalues if MAKE_CLOSURE
            if (instr.op == Op::MAKE_CLOSURE && cit->second->upvalueCount > 0)
            {
                auto &constants = cit->second->constants;
                // Last constant is the upvalue descriptor array
                if (!constants.empty() && constants.back().isArray())
                {
                    auto &descs = *constants.back().asArray();
                    for (auto &desc : descs)
                    {
                        if (!desc.isArray())
                            continue;
                        auto &d = *desc.asArray();
                        bool isLocal = d[0].asNumber() != 0.0;
                        int idx2 = (int)d[1].asNumber();
                        if (isLocal)
                            closure->upvalues.push_back(
                                captureUpvalue(frame.stackBase + idx2));
                        else if (idx2 < (int)frame.closure->upvalues.size())
                            closure->upvalues.push_back(
                                frame.closure->upvalues[idx2]);
                    }
                }
            }

            // Register closure in the closure table
            // Register under BOTH holder and callableNative so all lookup paths work
            s_closureTable[holder.get()] = closure;

            auto callableNative = std::make_shared<QuantumNative>();
            callableNative->name = "__callable__" + closure->name;
            // Register callableNative too — this is the key CALL/INSTANCE_NEW will find
            s_closureTable[callableNative.get()] = closure;

            // Keep a fallback fn (used only if s_closureTable lookup fails)
            auto closureCapture = closure;
            auto vmPtr = this;
            callableNative->fn = [vmPtr, closureCapture](std::vector<QuantumValue> args) -> QuantumValue
            {
                // Run this closure as a sub-call: push frame, run until it returns
                for (auto &a : args)
                    vmPtr->push(a);
                size_t base = vmPtr->stack_.size() - args.size();
                size_t savedDepth = vmPtr->frames_.size();
                vmPtr->frames_.push_back({closureCapture, 0, base});
                // Run only until this new frame completes
                while (vmPtr->frames_.size() > savedDepth)
                {
                    CallFrame &cf = vmPtr->frames_.back();
                    auto &co = cf.closure->chunk->code;
                    if (cf.ip >= co.size())
                    {
                        size_t b = cf.stackBase;
                        vmPtr->frames_.pop_back();
                        while (vmPtr->stack_.size() > b)
                            vmPtr->stack_.pop_back();
                        vmPtr->push(QuantumValue());
                        break;
                    }
                    if (++vmPtr->stepCount_ > VM::MAX_STEPS)
                        throw RuntimeError("Step limit exceeded");
                    // Single-step the frame by running the full loop once
                    // (We call runFrame which runs ALL remaining frames — depth check prevents overshoot)
                    vmPtr->runFrame(savedDepth);
                    break; // runFrame ran only our frame
                }
                if (!vmPtr->stack_.empty())
                    return vmPtr->pop();
                return QuantumValue();
            };

            push(QuantumValue(callableNative));
            break;
        }

        case Op::CALL:
        {
            int argCount = instr.operand;
            // Stack: [callee, arg0, arg1, ..., argN]  (callee is below args)
            QuantumValue callee = stack_[stack_.size() - argCount - 1];

            // Instance method call shortcut: if callee is a native callable,
            // just invoke it.
            if (callee.isNative())
            {
                auto fn = callee.asNative();
                std::string &nm = fn->name;

                if (nm.substr(0, 12) == "__callable__")
                {
                    auto cit = s_closureTable.find(fn.get());
                    if (cit != s_closureTable.end())
                    {
                        std::vector<QuantumValue> cargs;
                        cargs.reserve(argCount);
                        for (int i = 0; i < argCount; ++i)
                            cargs.push_back(stack_[stack_.size() - argCount + i]);
                        for (int i = 0; i <= argCount; ++i)
                            stack_.pop_back();
                        for (auto &a : cargs)
                            push(a);
                        size_t newBase = stack_.size() - argCount;
                        frames_.push_back({cit->second, 0, newBase});
                        break;
                    }
                    // Fallback (should not reach here)
                    std::vector<QuantumValue> args2;
                    for (int i = 0; i < argCount; ++i)
                        args2.push_back(stack_[stack_.size() - argCount + i]);
                    for (int i = 0; i <= argCount; ++i)
                        stack_.pop_back();
                    push(fn->fn(args2));
                    break;
                }
                if (nm.substr(0, 9) == "__bound__")
                {
                    // Bound method: look up {closure, self} in s_boundTable
                    auto bit = s_boundTable.find(fn.get());
                    if (bit != s_boundTable.end())
                    {
                        auto &bm = bit->second;
                        // Collect explicit args
                        std::vector<QuantumValue> cargs;
                        cargs.reserve(argCount);
                        for (int i = 0; i < argCount; ++i)
                            cargs.push_back(stack_[stack_.size() - argCount + i]);
                        for (int i = 0; i <= argCount; ++i)
                            stack_.pop_back(); // pop callee+args
                        // Push self first, then explicit args
                        push(bm.self);
                        for (auto &a : cargs)
                            push(a);
                        size_t newBase = stack_.size() - argCount - 1; // includes self
                        frames_.push_back({bm.closure, 0, newBase});
                        break;
                    }
                    // Fallback: call fn->fn(args) directly
                    std::vector<QuantumValue> args2;
                    for (int i = 0; i < argCount; ++i)
                        args2.push_back(stack_[stack_.size() - argCount + i]);
                    for (int i = 0; i <= argCount; ++i)
                        stack_.pop_back();
                    push(fn->fn(args2));
                    break;
                }

                // Regular native
                std::vector<QuantumValue> args;
                for (int i = 0; i < argCount; ++i)
                    args.push_back(stack_[stack_.size() - argCount + i]);
                for (int i = 0; i <= argCount; ++i)
                    stack_.pop_back();
                QuantumValue res;
                try
                {
                    res = fn->fn(args);
                }
                catch (QuantumError &)
                {
                    throw;
                }
                catch (std::exception &e)
                {
                    throw RuntimeError(e.what(), line);
                }
                push(res);
                break;
            }

            if (callee.isClass())
            {
                std::vector<QuantumValue> args;
                for (int i = 0; i < argCount; ++i)
                    args.push_back(stack_[stack_.size() - argCount + i]);
                for (int i = 0; i <= argCount; ++i)
                    stack_.pop_back();

                auto klass = callee.asClass();
                auto inst = std::make_shared<QuantumInstance>();
                inst->klass = klass;
                inst->env = std::make_shared<Environment>(globals);

                auto *k = klass.get();
                QuantumValue instVal(inst);

                // Look for __init__
                while (k)
                {
                    auto it = k->methods.find("__init__");
                    if (it == k->methods.end())
                        it = k->methods.find("init");
                    if (it != k->methods.end())
                    {
                        auto fn2 = it->second;
                        auto *fnPtr = reinterpret_cast<QuantumNative *>(fn2.get());
                        auto cit = s_closureTable.find(fnPtr);
                        if (cit != s_closureTable.end())
                        {
                            push(instVal);
                            for (auto &a : args)
                                push(a);
                            callClosure(cit->second, (int)args.size() + 1, line);
                            break;
                        }
                    }
                    k = k->base.get();
                }

                if (frames_.back().closure->chunk != frame.closure->chunk ||
                    frames_.back().ip != frame.ip)
                    break; // we pushed a new frame; let the loop continue

                push(instVal);
                break;
            }

            if (callee.isInstance())
            {
                // instance() — call __call__ method
                auto inst = callee.asInstance();
                try
                {
                    auto callMethod = inst->getField("__call__");
                    // Replace callee with __call__
                    stack_[stack_.size() - argCount - 1] = callMethod;
                    frame.ip--; // re-execute CALL
                    break;
                }
                catch (...)
                {
                }
            }

            throw TypeError("Cannot call value of type " + callee.typeName(), line);
        }

        case Op::RETURN:
        {
            QuantumValue result = pop();
            size_t base = frame.stackBase;
            closeUpvalues(base);
            frames_.pop_back();
            while (stack_.size() > base)
                stack_.pop_back();
            push(std::move(result));
            break;
        }
        case Op::RETURN_NIL:
        {
            size_t base = frame.stackBase;
            closeUpvalues(base);
            frames_.pop_back();
            while (stack_.size() > base)
                stack_.pop_back();
            if (!pendingInstances_.empty() &&
                frames_.size() == pendingInstances_.back().second)
            {
                push(pendingInstances_.back().first);
                pendingInstances_.pop_back();
            }
            else
                push(QuantumValue());
            break;
        }

        // ── Collections ───────────────────────────────────────────────────
        case Op::MAKE_ARRAY:
        {
            int n = instr.operand;
            auto arr = std::make_shared<Array>(n);
            for (int i = n - 1; i >= 0; --i)
                (*arr)[i] = pop();
            push(QuantumValue(arr));
            break;
        }
        case Op::MAKE_DICT:
        {
            int n = instr.operand; // number of pairs
            auto dict = std::make_shared<Dict>();
            // Stack has n*2 values: k0,v0,k1,v1,...
            std::vector<std::pair<QuantumValue, QuantumValue>> pairs(n);
            for (int i = n - 1; i >= 0; --i)
            {
                pairs[i].second = pop();
                pairs[i].first = pop();
            }
            for (auto &[k, v] : pairs)
                (*dict)[k.toString()] = v;
            push(QuantumValue(dict));
            break;
        }
        case Op::MAKE_TUPLE:
        {
            int n = instr.operand;
            auto arr = std::make_shared<Array>(n);
            for (int i = n - 1; i >= 0; --i)
                (*arr)[i] = pop();
            push(QuantumValue(arr)); // tuples stored as arrays
            break;
        }

        // ── Index / member access ─────────────────────────────────────────
        case Op::GET_INDEX:
        {
            QuantumValue idx = pop();
            QuantumValue obj = pop();

            if (obj.isArray())
            {
                auto &arr = *obj.asArray();
                int i = (int)toNumber(idx, "index", line);
                if (i < 0)
                    i += (int)arr.size();
                if (i < 0 || i >= (int)arr.size())
                    throw IndexError("Array index out of range: " + std::to_string(i), line);
                push(arr[i]);
            }
            else if (obj.isString())
            {
                const std::string &s = obj.asString();
                int i = (int)toNumber(idx, "index", line);
                if (i < 0)
                    i += (int)s.size();
                if (i < 0 || i >= (int)s.size())
                    throw IndexError("String index out of range", line);
                push(QuantumValue(std::string(1, s[i])));
            }
            else if (obj.isDict())
            {
                auto &d = *obj.asDict();
                auto it = d.find(idx.toString());
                push(it != d.end() ? it->second : QuantumValue());
            }
            else
                throw TypeError("Cannot index into " + obj.typeName(), line);
            break;
        }

        case Op::SET_INDEX:
        {
            // Stack: ... value, obj, key  (key on top)
            QuantumValue key = pop();
            QuantumValue obj = pop();
            QuantumValue val = pop();

            if (obj.isArray())
            {
                int i = (int)toNumber(key, "index", line);
                auto &arr = *obj.asArray();
                if (i < 0)
                    i += (int)arr.size();
                if (i < 0 || i >= (int)arr.size())
                    throw IndexError("Array index out of range", line);
                arr[i] = val;
            }
            else if (obj.isDict())
                (*obj.asDict())[key.toString()] = val;
            else
                throw TypeError("Cannot index-assign " + obj.typeName(), line);

            push(val); // assignment is an expression
            break;
        }

        case Op::GET_MEMBER:
        {
            const std::string &name = consts[instr.operand].asString();
            QuantumValue obj = pop();

            if (obj.isInstance())
            {
                auto inst = obj.asInstance();
                // 1. Instance fields
                auto fit = inst->fields.find(name);
                if (fit != inst->fields.end())
                {
                    push(fit->second);
                    break;
                }

                // 2. Class methods: create a bound method registered in s_boundTable
                auto *k = inst->klass.get();
                bool found = false;
                while (k && !found)
                {
                    auto mit = k->staticFields.find("__m__" + name);
                    if (mit != k->staticFields.end() && mit->second.isNative())
                    {
                        auto callable = mit->second.asNative();
                        auto cit = s_closureTable.find(callable.get());
                        if (cit != s_closureTable.end())
                        {
                            auto nat = std::make_shared<QuantumNative>();
                            nat->name = "__bound__" + name;
                            nat->fn = [](std::vector<QuantumValue>) -> QuantumValue
                            { return QuantumValue(); };
                            s_boundTable[nat.get()] = {cit->second, obj};
                            push(QuantumValue(nat));
                            found = true;
                        }
                    }
                    if (!found)
                        k = k->base.get();
                }
                if (found)
                    break;
            }

            if (obj.isClass())
            {
                auto klass = obj.asClass();
                auto cm = klass->staticFields.find("__m__" + name);
                if (cm != klass->staticFields.end())
                {
                    push(cm->second);
                    break;
                }
                auto fi = klass->staticFields.find(name);
                if (fi != klass->staticFields.end())
                {
                    push(fi->second);
                    break;
                }
            }

            // Built-in method (array/string/dict methods)
            {
                auto native = std::make_shared<QuantumNative>();
                native->name = "__method__" + name;
                auto objCap = obj;
                auto vmCap = this;
                auto nameCap = name;
                native->fn = [vmCap, objCap, nameCap](std::vector<QuantumValue> args) mutable -> QuantumValue
                {
                    return vmCap->callBuiltinMethod(objCap, nameCap, args, 0);
                };
                push(QuantumValue(native));
                break;
            }
        }

        case Op::SET_MEMBER:
        {
            const std::string &name = consts[instr.operand].asString();
            QuantumValue val = pop();
            QuantumValue obj = peek(0);

            if (obj.isInstance())
                obj.asInstance()->setField(name, val);
            else if (obj.isClass())
                obj.asClass()->staticFields[name] = val;
            else
                throw TypeError("Cannot set member on " + obj.typeName(), line);
            break;
        }

        // ── Iteration ─────────────────────────────────────────────────────
        case Op::MAKE_ITER:
        {
            QuantumValue iterable = pop();
            std::shared_ptr<Array> src;

            if (iterable.isArray())
                src = iterable.asArray();
            else if (iterable.isString())
            {
                src = std::make_shared<Array>();
                for (char c : iterable.asString())
                    src->push_back(QuantumValue(std::string(1, c)));
            }
            else if (iterable.isDict())
            {
                src = std::make_shared<Array>();
                for (auto &[k, v] : *iterable.asDict())
                    src->push_back(QuantumValue(k));
            }
            else
                throw TypeError("Value is not iterable: " + iterable.typeName(), line);

            // Store iterator state INSIDE the native closure — no external map needed
            auto idx = std::make_shared<size_t>(0);
            auto cap_src = src;
            auto cap_idx = idx;

            auto state = std::make_shared<QuantumNative>();
            state->name = "__iter__";
            state->fn = [cap_src, cap_idx](std::vector<QuantumValue> args) -> QuantumValue
            {
                // Called with no args: return {done, value} pair
                if (cap_idx && *cap_idx < cap_src->size())
                {
                    QuantumValue v = (*cap_src)[(*cap_idx)++];
                    return v; // return next element
                }
                return QuantumValue(); // exhausted: nil signals done
            };
            push(QuantumValue(state));
            break;
        }

        case Op::FOR_ITER:
        {
            // Peek at iterator (top of stack = iterator native)
            QuantumValue &iter = peek(0);
            if (!iter.isNative())
            {
                frame.ip += instr.operand;
                break;
            }
            auto nat = iter.asNative();
            if (nat->name != "__iter__")
            {
                frame.ip += instr.operand;
                break;
            }
            // Call fn() with no args to get next value
            QuantumValue next = nat->fn({});
            if (next.isNil())
            {
                // Exhausted — jump past loop
                frame.ip += instr.operand;
                break;
            }
            push(next); // push loop variable value
            break;
        }

        // ── Classes ───────────────────────────────────────────────────────
        case Op::MAKE_CLASS:
        {
            // Operand 0: class name was pushed as LOAD_CONST just before
            QuantumValue namePushed = pop();
            std::string className = namePushed.isString() ? namePushed.asString() : "Class";
            auto klass = std::make_shared<QuantumClass>();
            klass->name = className;
            push(QuantumValue(klass));
            break;
        }

        case Op::INHERIT:
        {
            QuantumValue base = pop();
            if (!base.isClass())
                throw TypeError("Base must be a class", line);
            peek(0).asClass()->base = base.asClass();
            break;
        }

        case Op::BIND_METHOD:
        {
            const std::string &methodName = consts[instr.operand].asString();
            QuantumValue fn = pop();
            QuantumValue &classVal = peek(0);
            if (!classVal.isClass())
                throw RuntimeError("BIND_METHOD: top is not a class", line);

            // Store a dummy QuantumFunction for compatibility
            auto qfn = std::make_shared<QuantumFunction>();
            qfn->name = methodName;
            classVal.asClass()->methods[methodName] = qfn;

            // Store the actual callable native in staticFields under "__m__name"
            // so GET_MEMBER can retrieve it directly for both class and instance calls
            classVal.asClass()->staticFields["__m__" + methodName] = fn;
            break;
        }

        case Op::INSTANCE_NEW:
        {
            int argCount = instr.operand;
            QuantumValue callee = stack_[stack_.size() - argCount - 1];
            if (!callee.isClass())
                throw TypeError("new: expected class, got " + callee.typeName(), line);

            // Pop args and callee
            std::vector<QuantumValue> args;
            for (int i = 0; i < argCount; ++i)
                args.push_back(stack_[stack_.size() - argCount + i]);
            for (int i = 0; i <= argCount; ++i)
                stack_.pop_back();

            auto klass = callee.asClass();
            auto inst = std::make_shared<QuantumInstance>();
            inst->klass = klass;
            inst->env = std::make_shared<Environment>(globals);
            QuantumValue instVal(inst);

            // Find __init__ or init
            auto *k = klass.get();
            bool initFound = false;
            while (k && !initFound)
            {
                for (const char *initName : {"__init__", "init"})
                {
                    auto mit = k->staticFields.find(std::string("__m__") + initName);
                    if (mit != k->staticFields.end() && mit->second.isNative())
                    {
                        auto initNat = mit->second.asNative();
                        auto cit = s_closureTable.find(initNat.get());
                        if (cit != s_closureTable.end())
                        {
                            // Push self + constructor args onto stack
                            push(instVal);
                            for (auto &a : args)
                                push(a);
                            size_t newBase = stack_.size() - (int)args.size() - 1;
                            // Set pendingInstance_ so RETURN/RETURN_NIL pushes the
                            // instance instead of nil when __init__ returns
                            // Record current depth: when init returns, frames will be back to this size
                            size_t targetDepth = frames_.size();
                            pendingInstances_.push_back({instVal, targetDepth});
                            frames_.push_back({cit->second, 0, newBase});
                            initFound = true;
                        }
                        break;
                    }
                }
                k = k->base.get();
            }

            // If no __init__, push the instance directly
            if (!initFound)
                push(instVal);
            break;
        }
        case Op::GET_SUPER:
        {
            const std::string &method = consts[instr.operand].asString();
            QuantumValue selfVal = pop();
            if (!selfVal.isInstance())
                throw RuntimeError("super: self is not an instance", line);

            auto base = selfVal.asInstance()->klass->base;
            if (!base)
                throw RuntimeError("super: class has no base class", line);

            // Normalise method name (parser converts __init__->init, toString->__str__ etc)
            std::string lookupMethod = method;
            if (method == "__init__")
                lookupMethod = "init";
            else if (method == "__str__" || method == "toString")
                lookupMethod = "__str__";

            // Search base class hierarchy for the method
            auto *k = base.get();
            bool superFound = false;
            while (k && !superFound)
            {
                auto mit = k->staticFields.find("__m__" + lookupMethod);
                if (mit == k->staticFields.end())
                    mit = k->staticFields.find("__m__" + method); // try original name too
                if (mit != k->staticFields.end() && mit->second.isNative())
                {
                    auto callable = mit->second.asNative();
                    auto cit = s_closureTable.find(callable.get());
                    if (cit != s_closureTable.end())
                    {
                        auto nat = std::make_shared<QuantumNative>();
                        nat->name = "__bound__" + method;
                        nat->fn = [](std::vector<QuantumValue>) -> QuantumValue
                        { return QuantumValue(); };
                        s_boundTable[nat.get()] = {cit->second, selfVal};
                        push(QuantumValue(nat));
                        superFound = true;
                    }
                }
                if (!superFound)
                    k = k->base.get();
            }
            if (!superFound)
                push(QuantumValue()); // nil fallback
            break;
        }

        // ── Exceptions ────────────────────────────────────────────────────
        case Op::PUSH_HANDLER:
        {
            int32_t catchIp = static_cast<int32_t>(frame.ip) + instr.operand;
            handlers_.push_back({catchIp, frames_.size(), stack_.size()});
            break;
        }
        case Op::POP_HANDLER:
            if (!handlers_.empty())
                handlers_.pop_back();
            break;
        case Op::RAISE:
        {
            QuantumValue val = pop();
            if (handlers_.empty())
            {
                std::string msg = val.isString() ? val.asString() : val.toString();
                throw RuntimeError(msg, line);
            }
            ExceptionHandler h = handlers_.back();
            handlers_.pop_back();
            while (frames_.size() > h.frameDepth)
                frames_.pop_back();
            while (stack_.size() > h.stackDepth)
                stack_.pop_back();
            push(val);
            if (!frames_.empty())
                frames_.back().ip = h.catchIp;
            break;
        }
        case Op::RERAISE:
        {
            if (!handlers_.empty())
            {
                ExceptionHandler h = handlers_.back();
                handlers_.pop_back();
                if (!frames_.empty())
                    frames_.back().ip = h.catchIp;
            }
            break;
        }

        // ── Pointers ──────────────────────────────────────────────────────
        case Op::ADDRESS_OF:
        {
            // For named variables this creates a pointer cell;
            // simplified: wrap in QuantumPointer
            QuantumValue v = pop();
            auto cell = std::make_shared<QuantumValue>(v);
            auto ptr = std::make_shared<QuantumPointer>();
            ptr->cell = cell;
            push(QuantumValue(ptr));
            break;
        }
        case Op::DEREF:
        {
            QuantumValue v = pop();
            if (!v.isPointer())
                throw TypeError("Cannot dereference non-pointer", line);
            push(v.asPointer()->deref());
            break;
        }
        case Op::ARROW:
        {
            // ptr->member: handled by compileArrow as DEREF + GET_MEMBER
            break;
        }

        // ── Print ─────────────────────────────────────────────────────────
        case Op::PRINT:
        {
            int n = instr.operand;
            // Stack: arg0..argN-1, sep, end   (end on top)
            QuantumValue endStr = pop();
            QuantumValue sepStr = pop();
            std::string sep = sepStr.isString() ? sepStr.asString() : " ";
            std::string end = endStr.isString() ? endStr.asString() : "\n";

            std::vector<QuantumValue> args(n);
            for (int i = n - 1; i >= 0; --i)
                args[i] = pop();

            for (int i = 0; i < n; ++i)
            {
                if (i > 0)
                    std::cout << sep;
                std::cout << args[i].toString();
            }
            std::cout << end;
            std::cout.flush();
            break;
        }

        // ── Unhandled ─────────────────────────────────────────────────────
        default:
            throw RuntimeError("VM: unhandled opcode " +
                                   std::to_string(static_cast<int>(instr.op)),
                               line);
        } // switch
    } // while frames
}

// ─── Register natives (same set as the old Interpreter) ──────────────────────

static double toNum2(const QuantumValue &v, const std::string &ctx)
{
    if (v.isNumber())
        return v.asNumber();
    throw TypeError("Expected number in " + ctx + ", got " + v.typeName());
}

void VM::registerNatives()
{
    auto reg = [&](const std::string &name, QuantumNativeFunc fn)
    {
        auto nat = std::make_shared<QuantumNative>();
        nat->name = name;
        nat->fn = std::move(fn);
        globals->define(name, QuantumValue(nat));
    };

    // ── I/O ───────────────────────────────────────────────────────────────
    reg("__input__", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (g_testMode) return QuantumValue(std::string(""));
        if (!args.empty()) std::cout << args[0].toString();
        std::string line;
        std::getline(std::cin, line);
        return QuantumValue(line); });
    reg("input", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (g_testMode) return QuantumValue(std::string(""));
        if (!args.empty()) std::cout << args[0].toString();
        std::string line;
        std::getline(std::cin, line);
        return QuantumValue(line); });

    // ── Type conversion ───────────────────────────────────────────────────
    reg("num", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("num() requires 1 argument");
        if (args[0].isNumber()) return args[0];
        if (args[0].isString()) {
            try { return QuantumValue(std::stod(args[0].asString())); }
            catch (...) { throw TypeError("Cannot convert to number"); }
        }
        if (args[0].isBool()) return QuantumValue(args[0].asBool() ? 1.0 : 0.0);
        throw TypeError("Cannot convert to number"); });
    reg("int", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) return QuantumValue(0.0);
        if (args[0].isNumber()) return QuantumValue(std::floor(args[0].asNumber()));
        if (args[0].isString()) {
            try { return QuantumValue(std::floor(std::stod(args[0].asString()))); }
            catch (...) { return QuantumValue(0.0); }
        }
        if (args[0].isBool()) return QuantumValue(args[0].asBool() ? 1.0 : 0.0);
        return QuantumValue(0.0); });
    reg("float", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) return QuantumValue(0.0);
        if (args[0].isNumber()) return args[0];
        if (args[0].isString()) {
            try { return QuantumValue(std::stod(args[0].asString())); }
            catch (...) { return QuantumValue(0.0); }
        }
        return QuantumValue(0.0); });
    reg("str", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) return QuantumValue(std::string(""));
        return QuantumValue(args[0].toString()); });
    reg("bool", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) return QuantumValue(false);
        return QuantumValue(args[0].isTruthy()); });
    reg("chr", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("chr() requires 1 argument");
        int code = (int)args[0].asNumber();
        return QuantumValue(std::string(1, (char)code)); });
    reg("ord", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty() || !args[0].isString()) throw RuntimeError("ord() requires a string");
        return QuantumValue((double)(unsigned char)args[0].asString()[0]); });

    // ── Math ──────────────────────────────────────────────────────────────
    reg("abs", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::abs(toNum2(a[0], "abs"))); });
    reg("sqrt", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::sqrt(toNum2(a[0], "sqrt"))); });
    reg("floor", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::floor(toNum2(a[0], "floor"))); });
    reg("ceil", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::ceil(toNum2(a[0], "ceil"))); });
    reg("round", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::round(toNum2(a[0], "round"))); });
    reg("pow", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::pow(toNum2(a[0], "pow"), toNum2(a[1], "pow"))); });
    reg("log", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::log(toNum2(a[0], "log"))); });
    reg("log2", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::log2(toNum2(a[0], "log2"))); });
    reg("log10", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::log10(toNum2(a[0], "log10"))); });
    reg("sin", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::sin(toNum2(a[0], "sin"))); });
    reg("cos", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::cos(toNum2(a[0], "cos"))); });
    reg("tan", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::tan(toNum2(a[0], "tan"))); });
    reg("asin", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::asin(toNum2(a[0], "asin"))); });
    reg("acos", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::acos(toNum2(a[0], "acos"))); });
    reg("atan", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::atan(toNum2(a[0], "atan"))); });
    reg("atan2", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::atan2(toNum2(a[0], "atan2"), toNum2(a[1], "atan2"))); });
    reg("min", [](std::vector<QuantumValue> a) -> QuantumValue
        {
        if (a.empty()) throw RuntimeError("min() expected args");
        if (a.size()==1 && a[0].isArray()) {
            auto &arr=*a[0].asArray(); if(arr.empty()) throw RuntimeError("min(): empty");
            double m=toNum2(arr[0],"min"); for(size_t i=1;i<arr.size();i++) m=std::min(m,toNum2(arr[i],"min")); return QuantumValue(m);
        }
        double m=toNum2(a[0],"min"); for(size_t i=1;i<a.size();i++) m=std::min(m,toNum2(a[i],"min")); return QuantumValue(m); });
    reg("max", [](std::vector<QuantumValue> a) -> QuantumValue
        {
        if (a.empty()) throw RuntimeError("max() expected args");
        if (a.size()==1 && a[0].isArray()) {
            auto &arr=*a[0].asArray(); if(arr.empty()) throw RuntimeError("max(): empty");
            double m=toNum2(arr[0],"max"); for(size_t i=1;i<arr.size();i++) m=std::max(m,toNum2(arr[i],"max")); return QuantumValue(m);
        }
        double m=toNum2(a[0],"max"); for(size_t i=1;i<a.size();i++) m=std::max(m,toNum2(a[i],"max")); return QuantumValue(m); });

    // ── Utility ───────────────────────────────────────────────────────────
    reg("len", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("len() requires 1 argument");
        if (args[0].isString()) return QuantumValue((double)args[0].asString().size());
        if (args[0].isArray())  return QuantumValue((double)args[0].asArray()->size());
        if (args[0].isDict())   return QuantumValue((double)args[0].asDict()->size());
        throw TypeError("len() unsupported for " + args[0].typeName()); });
    reg("type", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("type() requires 1 argument");
        return QuantumValue(args[0].typeName()); });
    reg("range", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("range() requires arguments");
        double start=0, end_, step=1;
        if (args.size()==1) end_=toNum2(args[0],"range");
        else if (args.size()==2) { start=toNum2(args[0],"range"); end_=toNum2(args[1],"range"); }
        else { start=toNum2(args[0],"range"); end_=toNum2(args[1],"range"); step=toNum2(args[2],"range"); }
        auto arr=std::make_shared<Array>();
        if(step>0) for(double i=start;i<end_;i+=step) arr->push_back(QuantumValue(i));
        else for(double i=start;i>end_;i+=step) arr->push_back(QuantumValue(i));
        return QuantumValue(arr); });
    reg("print", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        for (size_t i=0;i<args.size();i++) { if(i) std::cout<<" "; std::cout<<args[i].toString(); }
        std::cout<<"\n";
        return QuantumValue(); });
    reg("enumerate", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("enumerate() requires 1 argument");
        auto arr=std::make_shared<Array>();
        int start=0;
        if(args.size()>1) start=(int)args[1].asNumber();
        if(args[0].isArray()) {
            for(auto &v:*args[0].asArray()) {
                auto pair=std::make_shared<Array>();
                pair->push_back(QuantumValue((double)start++));
                pair->push_back(v);
                arr->push_back(QuantumValue(pair));
            }
        }
        return QuantumValue(arr); });
    reg("zip", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        auto arr=std::make_shared<Array>();
        if(args.empty()) return QuantumValue(arr);
        size_t minLen=SIZE_MAX;
        for(auto &a:args) if(a.isArray()) minLen=std::min(minLen,a.asArray()->size());
        if(minLen==SIZE_MAX) minLen=0;
        for(size_t i=0;i<minLen;i++) {
            auto tuple=std::make_shared<Array>();
            for(auto &a:args) if(a.isArray()) tuple->push_back((*a.asArray())[i]);
            arr->push_back(QuantumValue(tuple));
        }
        return QuantumValue(arr); });
    reg("map", [this](std::vector<QuantumValue> args) -> QuantumValue
        {
        if(args.size()<2) throw RuntimeError("map() requires 2 args");
        auto fn=args[0]; auto &iterable=args[1];
        if(!iterable.isArray()) throw TypeError("map() requires iterable");
        auto arr=std::make_shared<Array>();
        auto *nat=std::get_if<std::shared_ptr<QuantumNative>>(&fn.data);
        if(!nat) return QuantumValue(arr);
        for(auto &v:*iterable.asArray()) {
            QuantumValue result = (*nat)->fn({v});
            arr->push_back(result);
        }
        return QuantumValue(arr); });
    reg("filter", [this](std::vector<QuantumValue> args) -> QuantumValue
        {
        if(args.size()<2) throw RuntimeError("filter() requires 2 args");
        auto fn=args[0]; auto &iterable=args[1];
        if(!iterable.isArray()) throw TypeError("filter() requires iterable");
        auto arr=std::make_shared<Array>();
        auto *nat=std::get_if<std::shared_ptr<QuantumNative>>(&fn.data);
        if(!nat) return QuantumValue(arr);
        for(auto &v:*iterable.asArray()) {
            if((*nat)->fn({v}).isTruthy()) arr->push_back(v);
        }
        return QuantumValue(arr); });
    reg("sorted", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if(args.empty()||!args[0].isArray()) throw RuntimeError("sorted() requires array");
        auto copy=std::make_shared<Array>(*args[0].asArray());
        bool rev=args.size()>1&&args[1].isTruthy();
        std::sort(copy->begin(),copy->end(),[rev](const QuantumValue &a,const QuantumValue &b){
            bool lt = a.isNumber()&&b.isNumber() ? a.asNumber()<b.asNumber() : a.toString()<b.toString();
            return rev ? !lt : lt;
        });
        return QuantumValue(copy); });
    reg("reversed", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if(args.empty()||!args[0].isArray()) throw RuntimeError("reversed() requires array");
        auto copy=std::make_shared<Array>(*args[0].asArray());
        std::reverse(copy->begin(),copy->end());
        return QuantumValue(copy); });
    reg("sum", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if(args.empty()||!args[0].isArray()) throw RuntimeError("sum() requires array");
        double s=0; for(auto &v:*args[0].asArray()) s+=toNum2(v,"sum");
        return QuantumValue(s); });
    reg("isinstance", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if(args.size()<2) throw RuntimeError("isinstance() requires 2 args");
        std::string t;
        if(args[1].isString()) t=args[1].asString();
        else t=args[1].typeName();
        auto matches=[&](const std::string &s)->bool{
            if(s=="int"||s=="float"||s=="number"||s=="double") return args[0].isNumber();
            if(s=="str"||s=="string") return args[0].isString();
            if(s=="bool") return args[0].isBool();
            if(s=="list"||s=="array") return args[0].isArray();
            if(s=="dict") return args[0].isDict();
            if(s=="NoneType"||s=="nil") return args[0].isNil();
            return args[0].typeName()==s;
        };
        return QuantumValue(matches(t)); });

    // ── List-comp push helper ─────────────────────────────────────────────
    // __listcomp_push__(arr, val) — push val onto arr, return arr
    reg("__listcomp_push__", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if(args.size()<2||!args[0].isArray()) return QuantumValue();
        args[0].asArray()->push_back(args[1]);
        return args[0]; });

    // ── Slice helper ──────────────────────────────────────────────────────
    reg("__slice__", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if(args.empty()) return QuantumValue();
        auto &obj=args[0];
        int start=0,stop=-1,step=1;
        if(args.size()>1&&!args[1].isNil()) start=(int)args[1].asNumber();
        if(args.size()>2&&!args[2].isNil()) stop=(int)args[2].asNumber();
        if(args.size()>3&&!args[3].isNil()) step=(int)args[3].asNumber();
        if(step==0) throw RuntimeError("slice step cannot be zero");

        if(obj.isString()) {
            const std::string &s=obj.asString();
            int len=(int)s.size();
            if(stop<0||stop>len) stop=len;
            if(start<0) start=std::max(0,len+start);
            std::string r;
            if(step>0) for(int i=start;i<stop;i+=step) if(i<len) r+=s[i];
            else for(int i=start;i>stop;i+=step) if(i>=0&&i<len) r+=s[i];
            return QuantumValue(r);
        }
        if(obj.isArray()) {
            auto &arr=*obj.asArray();
            int len=(int)arr.size();
            if(stop<0||stop>len) stop=len;
            if(start<0) start=std::max(0,len+start);
            auto r=std::make_shared<Array>();
            if(step>0) for(int i=start;i<stop;i+=step) if(i<len) r->push_back(arr[i]);
            else for(int i=start;i>stop;i+=step) if(i>=0&&i<len) r->push_back(arr[i]);
            return QuantumValue(r);
        }
        return QuantumValue(); });

    // ── Constants ─────────────────────────────────────────────────────────
    globals->define("PI", QuantumValue(M_PI));
    globals->define("E", QuantumValue(M_E));
    globals->define("INF", QuantumValue(std::numeric_limits<double>::infinity()));
    globals->define("NaN", QuantumValue(std::numeric_limits<double>::quiet_NaN()));
    globals->define("null", QuantumValue());
    globals->define("undefined", QuantumValue());
    globals->define("__name__", QuantumValue(std::string("__main__")));
    globals->define("__file__", QuantumValue(std::string("")));
    globals->define("true", QuantumValue(true));
    globals->define("false", QuantumValue(false));
    globals->define("nil", QuantumValue());
    globals->define("None", QuantumValue());
}

// ─── Array methods ────────────────────────────────────────────────────────────

QuantumValue VM::callArrayMethod(std::shared_ptr<Array> arr, const std::string &m,
                                 std::vector<QuantumValue> args)
{
    if (m == "push" || m == "append")
    {
        arr->push_back(args.empty() ? QuantumValue() : args[0]);
        return QuantumValue(arr);
    }
    if (m == "pop")
    {
        if (arr->empty())
            throw RuntimeError("pop() on empty array");
        QuantumValue v = arr->back();
        arr->pop_back();
        return v;
    }
    if (m == "length" || m == "size")
        return QuantumValue((double)arr->size());
    if (m == "shift")
    {
        if (arr->empty())
            return QuantumValue();
        QuantumValue v = arr->front();
        arr->erase(arr->begin());
        return v;
    }
    if (m == "unshift")
    {
        if (!args.empty())
            arr->insert(arr->begin(), args[0]);
        return QuantumValue((double)arr->size());
    }
    if (m == "reverse")
    {
        std::reverse(arr->begin(), arr->end());
        return QuantumValue(arr);
    }
    if (m == "sort")
    {
        std::sort(arr->begin(), arr->end(), [](const QuantumValue &a, const QuantumValue &b)
                  { return a.isNumber() && b.isNumber() ? a.asNumber() < b.asNumber() : a.toString() < b.toString(); });
        return QuantumValue(arr);
    }
    if (m == "join")
    {
        std::string sep = args.empty() ? "," : args[0].toString();
        std::string s;
        for (size_t i = 0; i < arr->size(); i++)
        {
            if (i)
                s += sep;
            s += (*arr)[i].toString();
        }
        return QuantumValue(s);
    }
    if (m == "includes" || m == "contains")
    {
        if (args.empty())
            return QuantumValue(false);
        for (auto &v : *arr)
            if (VM::valuesEqual(v, args[0]))
                return QuantumValue(true);
        return QuantumValue(false);
    }
    if (m == "indexOf")
    {
        if (args.empty())
            return QuantumValue(-1.0);
        for (size_t i = 0; i < arr->size(); i++)
            if (VM::valuesEqual((*arr)[i], args[0]))
                return QuantumValue((double)i);
        return QuantumValue(-1.0);
    }
    if (m == "slice")
    {
        int start = args.empty() ? 0 : (int)args[0].asNumber();
        int stop = args.size() > 1 ? (int)args[1].asNumber() : (int)arr->size();
        int len = (int)arr->size();
        if (start < 0)
            start = std::max(0, len + start);
        if (stop < 0)
            stop = std::max(0, len + stop);
        stop = std::min(stop, len);
        auto r = std::make_shared<Array>(arr->begin() + start, arr->begin() + stop);
        return QuantumValue(r);
    }
    if (m == "splice")
    {
        if (args.empty())
            return QuantumValue(std::make_shared<Array>());
        int idx = (int)args[0].asNumber();
        int deleteCount = args.size() > 1 ? (int)args[1].asNumber() : (int)arr->size() - idx;
        if (idx < 0)
            idx = std::max(0, (int)arr->size() + idx);
        idx = std::min(idx, (int)arr->size());
        deleteCount = std::max(0, std::min(deleteCount, (int)arr->size() - idx));
        auto removed = std::make_shared<Array>(arr->begin() + idx, arr->begin() + idx + deleteCount);
        arr->erase(arr->begin() + idx, arr->begin() + idx + deleteCount);
        for (size_t i = 2; i < args.size(); i++)
            arr->insert(arr->begin() + idx + (i - 2), args[i]);
        return QuantumValue(removed);
    }
    if (m == "concat")
    {
        auto r = std::make_shared<Array>(*arr);
        for (auto &a : args)
            if (a.isArray())
                for (auto &v : *a.asArray())
                    r->push_back(v);
        return QuantumValue(r);
    }
    if (m == "flat" || m == "flatten")
    {
        auto r = std::make_shared<Array>();
        for (auto &v : *arr)
        {
            if (v.isArray())
                for (auto &inner : *v.asArray())
                    r->push_back(inner);
            else
                r->push_back(v);
        }
        return QuantumValue(r);
    }
    if (m == "fill")
    {
        if (args.empty())
            return QuantumValue(arr);
        QuantumValue val = args[0];
        for (auto &v : *arr)
            v = val;
        return QuantumValue(arr);
    }
    if (m == "count")
    {
        if (args.empty())
            return QuantumValue((double)arr->size());
        int c = 0;
        for (auto &v : *arr)
            if (VM::valuesEqual(v, args[0]))
                c++;
        return QuantumValue((double)c);
    }
    if (m == "insert")
    {
        if (args.size() >= 2)
        {
            int idx = (int)args[0].asNumber();
            if (idx < 0)
                idx = std::max(0, (int)arr->size() + idx);
            idx = std::min(idx, (int)arr->size());
            arr->insert(arr->begin() + idx, args[1]);
        }
        return QuantumValue();
    }
    if (m == "remove")
    {
        if (!args.empty())
        {
            for (auto it = arr->begin(); it != arr->end(); ++it)
                if (VM::valuesEqual(*it, args[0]))
                {
                    arr->erase(it);
                    break;
                }
        }
        return QuantumValue();
    }
    if (m == "clear")
    {
        arr->clear();
        return QuantumValue();
    }
    if (m == "copy")
    {
        return QuantumValue(std::make_shared<Array>(*arr));
    }
    if (m == "extend")
    {
        if (!args.empty() && args[0].isArray())
            for (auto &v : *args[0].asArray())
                arr->push_back(v);
        return QuantumValue();
    }
    throw TypeError("Array has no method '" + m + "'");
}

// ─── String methods ───────────────────────────────────────────────────────────

QuantumValue VM::callStringMethod(const std::string &str, const std::string &m,
                                  std::vector<QuantumValue> args)
{
    if (m == "length" || m == "size")
        return QuantumValue((double)str.size());
    if (m == "toUpperCase" || m == "upper")
    {
        std::string r = str;
        std::transform(r.begin(), r.end(), r.begin(), ::toupper);
        return QuantumValue(r);
    }
    if (m == "toLowerCase" || m == "lower")
    {
        std::string r = str;
        std::transform(r.begin(), r.end(), r.begin(), ::tolower);
        return QuantumValue(r);
    }
    if (m == "trim" || m == "strip")
    {
        std::string r = str;
        while (!r.empty() && std::isspace((unsigned char)r.front()))
            r.erase(r.begin());
        while (!r.empty() && std::isspace((unsigned char)r.back()))
            r.pop_back();
        return QuantumValue(r);
    }
    if (m == "startsWith" || m == "startswith")
    {
        if (args.empty())
            return QuantumValue(false);
        return QuantumValue(str.substr(0, std::min(str.size(), args[0].toString().size())) == args[0].toString());
    }
    if (m == "endsWith" || m == "endswith")
    {
        if (args.empty())
            return QuantumValue(false);
        std::string s = args[0].toString();
        return QuantumValue(str.size() >= s.size() && str.substr(str.size() - s.size()) == s);
    }
    if (m == "includes" || m == "contains")
    {
        if (args.empty())
            return QuantumValue(false);
        return QuantumValue(str.find(args[0].toString()) != std::string::npos);
    }
    if (m == "indexOf")
    {
        if (args.empty())
            return QuantumValue(-1.0);
        auto pos = str.find(args[0].toString());
        return QuantumValue(pos == std::string::npos ? -1.0 : (double)pos);
    }
    if (m == "split")
    {
        std::string sep = args.empty() ? "" : (args[0].isNil() ? "" : args[0].toString());
        auto arr = std::make_shared<Array>();
        if (sep.empty())
        {
            for (char c : str)
                arr->push_back(QuantumValue(std::string(1, c)));
        }
        else
        {
            size_t p = 0, f;
            while ((f = str.find(sep, p)) != std::string::npos)
            {
                arr->push_back(QuantumValue(str.substr(p, f - p)));
                p = f + sep.size();
            }
            arr->push_back(QuantumValue(str.substr(p)));
        }
        return QuantumValue(arr);
    }
    if (m == "replace")
    {
        if (args.size() < 2)
            return QuantumValue(str);
        std::string s = str, from = args[0].toString(), to = args[1].toString();
        size_t p = s.find(from);
        if (p != std::string::npos)
            s = s.substr(0, p) + to + s.substr(p + from.size());
        return QuantumValue(s);
    }
    if (m == "replaceAll")
    {
        if (args.size() < 2)
            return QuantumValue(str);
        std::string s = str, from = args[0].toString(), to = args[1].toString();
        size_t p = 0;
        while ((p = s.find(from, p)) != std::string::npos)
        {
            s = s.substr(0, p) + to + s.substr(p + from.size());
            p += to.size();
        }
        return QuantumValue(s);
    }
    if (m == "substring" || m == "substr")
    {
        int start = args.empty() ? 0 : (int)args[0].asNumber();
        int len2 = args.size() > 1 ? (int)args[1].asNumber() : (int)str.size() - start;
        if (start < 0)
            start = 0;
        return QuantumValue(str.substr(std::min((size_t)start, str.size()), std::max(0, len2)));
    }
    if (m == "charAt")
    {
        if (args.empty())
            return QuantumValue(std::string(""));
        int i = (int)args[0].asNumber();
        if (i < 0 || i >= (int)str.size())
            return QuantumValue(std::string(""));
        return QuantumValue(std::string(1, str[i]));
    }
    if (m == "charCodeAt")
    {
        int i = args.empty() ? 0 : (int)args[0].asNumber();
        if (i < 0 || i >= (int)str.size())
            return QuantumValue(std::numeric_limits<double>::quiet_NaN());
        return QuantumValue((double)(unsigned char)str[i]);
    }
    if (m == "repeat")
    {
        int n = args.empty() ? 0 : (int)args[0].asNumber();
        std::string r;
        for (int i = 0; i < n; i++)
            r += str;
        return QuantumValue(r);
    }
    if (m == "padStart")
    {
        int n = args.empty() ? 0 : (int)args[0].asNumber();
        std::string p = args.size() > 1 ? args[1].toString() : " ";
        std::string r = str;
        while ((int)r.size() < n)
            r = p + r;
        return QuantumValue(r.substr(r.size() - std::max((size_t)n, str.size())));
    }
    if (m == "padEnd")
    {
        int n = args.empty() ? 0 : (int)args[0].asNumber();
        std::string p = args.size() > 1 ? args[1].toString() : " ";
        std::string r = str;
        while ((int)r.size() < n)
            r += p;
        return QuantumValue(r.substr(0, std::max((size_t)n, str.size())));
    }
    if (m == "isdigit")
    {
        for (char c : str)
            if (!std::isdigit((unsigned char)c))
                return QuantumValue(false);
        return QuantumValue(!str.empty());
    }
    if (m == "isalpha")
    {
        for (char c : str)
            if (!std::isalpha((unsigned char)c))
                return QuantumValue(false);
        return QuantumValue(!str.empty());
    }
    if (m == "isupper")
    {
        for (char c : str)
            if (std::isalpha((unsigned char)c) && !std::isupper((unsigned char)c))
                return QuantumValue(false);
        return QuantumValue(!str.empty());
    }
    if (m == "islower")
    {
        for (char c : str)
            if (std::isalpha((unsigned char)c) && !std::islower((unsigned char)c))
                return QuantumValue(false);
        return QuantumValue(!str.empty());
    }
    if (m == "format")
    {
        // Simple format: replace {} placeholders
        std::string result = str;
        size_t idx = 0;
        size_t p;
        while ((p = result.find("{}")) != std::string::npos && idx < args.size())
        {
            result = result.substr(0, p) + args[idx++].toString() + result.substr(p + 2);
        }
        return QuantumValue(result);
    }
    if (m == "count")
    {
        if (args.empty())
            return QuantumValue((double)str.size());
        std::string sub = args[0].toString();
        if (sub.empty())
            return QuantumValue((double)str.size());
        int cnt = 0;
        size_t p = 0;
        while ((p = str.find(sub, p)) != std::string::npos)
        {
            cnt++;
            p += sub.size();
        }
        return QuantumValue((double)cnt);
    }
    throw TypeError("String has no method '" + m + "'");
}

// ─── Dict methods ─────────────────────────────────────────────────────────────

QuantumValue VM::callDictMethod(std::shared_ptr<Dict> dict, const std::string &m,
                                std::vector<QuantumValue> args)
{
    if (m == "keys")
    {
        auto arr = std::make_shared<Array>();
        for (auto &[k, v] : *dict)
            arr->push_back(QuantumValue(k));
        return QuantumValue(arr);
    }
    if (m == "values")
    {
        auto arr = std::make_shared<Array>();
        for (auto &[k, v] : *dict)
            arr->push_back(v);
        return QuantumValue(arr);
    }
    if (m == "items" || m == "entries")
    {
        auto arr = std::make_shared<Array>();
        for (auto &[k, v] : *dict)
        {
            auto pair = std::make_shared<Array>();
            pair->push_back(QuantumValue(k));
            pair->push_back(v);
            arr->push_back(QuantumValue(pair));
        }
        return QuantumValue(arr);
    }
    if (m == "has" || m == "contains" || m == "hasOwnProperty")
    {
        if (args.empty())
            return QuantumValue(false);
        return QuantumValue(dict->count(args[0].toString()) > 0);
    }
    if (m == "get")
    {
        if (args.empty())
            return QuantumValue();
        auto it = dict->find(args[0].toString());
        return it != dict->end() ? it->second : (args.size() > 1 ? args[1] : QuantumValue());
    }
    if (m == "set")
    {
        if (args.size() >= 2)
            (*dict)[args[0].toString()] = args[1];
        return QuantumValue(dict);
    }
    if (m == "delete")
    {
        if (!args.empty())
            dict->erase(args[0].toString());
        return QuantumValue(true);
    }
    if (m == "clear")
    {
        dict->clear();
        return QuantumValue();
    }
    if (m == "size" || m == "length")
        return QuantumValue((double)dict->size());
    throw TypeError("Dict has no method '" + m + "'");
}