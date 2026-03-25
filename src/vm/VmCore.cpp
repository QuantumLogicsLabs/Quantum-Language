#include "Vm.h"
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
#include <thread>
#include <limits>
#include <cassert>
#include <unordered_set>

// #define DEBUG_TRACE_EXECUTION

#include "Disassembler.h"
#include <iomanip>

// Defined in main.cpp — true during --test runs so input() returns ""

// immediately instead of blocking on stdin.
extern bool g_testMode;

// Defined in Compiler.cpp — maps QuantumNative* chunk-holders to their Chunk

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

    // Create a top-level closure and push it to stack as a dummy callee
    auto closure = std::make_shared<Closure>(chunk);
    push(QuantumValue(closure));
    frames_.push_back({closure, 0, 1}); // locals start at stack index 1
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
        callClosure(callee.asFunction(), argCount, line);
        return;
    }
    if (callee.isBoundMethod())
    {
        auto bm = callee.asBoundMethod();
        std::vector<QuantumValue> args;
        for (int i = 0; i < argCount; ++i)
            args.push_back(stack_[stack_.size() - argCount + i]);
        for (int i = 0; i < argCount; ++i)
            stack_.pop_back();
        // The callee itself was on the stack before args, pop it!
        // But what if the compiler didn't push the callee? wait, Op::CALL expects callee on stack underneath args!
        // It's checked during CALL instruction: QuantumValue callee = stack_[stack_.size() - argCount - 1];
        // However callValue modifies the stack natively? No, callValue doesn't pop the callee.
        // Wait, callNativeFn pops args but NOT callee.
        // Op::CALL pops args AND callee afterwards? Let's check callClosure.
        // callClosure does not pop args or callee. It just sets stackBase = stack_.size() - argCount.
        // The callee is left on the stack at slot 0 of the new frame!
        // So for BoundMethod, we need to overwrite the callee slot with 'self', then push args.
        // Actually, frame's slot 0 should be 'self' instead of the BoundMethod object.
        stack_[stack_.size() - argCount - 1] = bm->self;
        callClosure(bm->method, argCount + 1, line);
        return;
    }
    throw TypeError("Cannot call value of type " + callee.typeName(), line);
}

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
    std::shared_ptr<Closure> initFn;
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
        std::vector<QuantumValue> newArgs = {instVal};
        for (int i = argCount - 1; i >= 0; --i)
            newArgs.push_back(stack_[stack_.size() - argCount + i]);
        for (int i = 0; i < argCount; ++i)
            stack_.pop_back();
        // Replace callee with 'self' is not needed here as we are doing manual frame push? No, we just need to replace it on the stack.
        // Actually, pop callee.
        stack_.pop_back();
        for (auto &a : newArgs)
            push(a);
        callClosure(initFn, (int)newArgs.size(), line);
        return;
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
                // Pop the args and the object from stack? No, callBuiltinMethod is normally called natively.
                // It receives obj and args as parameters.
                push(obj);
                for (auto &a : args)
                    push(a);
                callClosure(fn, (int)args.size() + 1, line);
                // Execute the frame!
                size_t savedDepth = frames_.size() - 1;
                runFrame(savedDepth);
                return pop();
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

