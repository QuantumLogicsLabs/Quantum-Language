#include "../include/Interpreter.h"
#include "../include/Error.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <random>
#include <iomanip>
#include <functional>
#include <thread>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

// ─── Helpers ─────────────────────────────────────────────────────────────────

static double toNum(const QuantumValue &v, const std::string &ctx)
{
    if (v.isNumber())
        return v.asNumber();
    throw TypeError("Expected number in " + ctx + ", got " + v.typeName());
}

static long long toInt(const QuantumValue &v, const std::string &ctx)
{
    return (long long)toNum(v, ctx);
}

// ─── Format Engine ───────────────────────────────────────────────────────────
// Shared by printf(), format(), and sprintf()-style calls.
// Supports: %d %i %u %f %e %g %s %c %x %X %o %b %%
// Flags:    - (left-align)  + (force sign)  0 (zero-pad)  space
// Width:    %8d   %-10s
// Precision:%6.2f  %.5s (truncate string)

static std::string applyFormat(const std::string &fmt, const std::vector<QuantumValue> &args, size_t argStart = 1)
{
    std::string out;
    size_t argIdx = argStart;
    size_t i = 0;

    auto nextArg = [&]() -> QuantumValue
    {
        return argIdx < args.size() ? args[argIdx++] : QuantumValue();
    };

    while (i < fmt.size())
    {
        if (fmt[i] != '%')
        {
            out += fmt[i++];
            continue;
        }
        ++i;
        if (i >= fmt.size())
            break;
        if (fmt[i] == '%')
        {
            out += '%';
            ++i;
            continue;
        }

        // ── Collect flags ──────────────────────────────────────────────────
        bool flagMinus = false, flagPlus = false, flagSpace = false, flagZero = false, flagHash = false;
        while (i < fmt.size())
        {
            char f = fmt[i];
            if (f == '-')
            {
                flagMinus = true;
                ++i;
            }
            else if (f == '+')
            {
                flagPlus = true;
                ++i;
            }
            else if (f == ' ')
            {
                flagSpace = true;
                ++i;
            }
            else if (f == '0')
            {
                flagZero = true;
                ++i;
            }
            else if (f == '#')
            {
                flagHash = true;
                ++i;
            }
            else
                break;
        }

        // ── Width ──────────────────────────────────────────────────────────
        int width = 0;
        while (i < fmt.size() && std::isdigit(fmt[i]))
            width = width * 10 + (fmt[i++] - '0');

        // ── Precision ─────────────────────────────────────────────────────
        int prec = -1;
        if (i < fmt.size() && fmt[i] == '.')
        {
            ++i;
            prec = 0;
            while (i < fmt.size() && std::isdigit(fmt[i]))
                prec = prec * 10 + (fmt[i++] - '0');
        }

        if (i >= fmt.size())
            break;
        char conv = fmt[i++];
        QuantumValue arg = nextArg();

        // ── Pad helper ────────────────────────────────────────────────────
        auto pad = [&](std::string s, bool numericSign = false) -> std::string
        {
            if (width > 0 && (int)s.size() < width)
            {
                int pad = width - (int)s.size();
                if (flagMinus)
                    s += std::string(pad, ' ');
                else if (flagZero && numericSign)
                    s = std::string(pad, '0') + s;
                else
                    s = std::string(pad, ' ') + s;
            }
            return s;
        };

        char buf[256];
        switch (conv)
        {

        // ── Integer specifiers ────────────────────────────────────────────
        case 'd':
        case 'i':
        {
            long long n = arg.isNumber() ? (long long)arg.asNumber() : 0LL;
            std::string s;
            if (prec >= 0)
            {
                // precision on integers = minimum digits
                std::snprintf(buf, sizeof(buf), ("%0*lld"), prec, std::abs(n));
                s = buf;
                if (n < 0)
                    s = "-" + s;
                else if (flagPlus)
                    s = "+" + s;
                else if (flagSpace)
                    s = " " + s;
            }
            else
            {
                std::snprintf(buf, sizeof(buf), "%lld", n);
                s = buf;
                if (n >= 0 && flagPlus)
                    s = "+" + s;
                else if (n >= 0 && flagSpace)
                    s = " " + s;
            }
            out += pad(s, true);
            break;
        }
        case 'u':
        {
            unsigned long long n = arg.isNumber() ? (unsigned long long)(long long)arg.asNumber() : 0ULL;
            std::snprintf(buf, sizeof(buf), "%llu", n);
            out += pad(std::string(buf), true);
            break;
        }

        // ── Float specifiers ──────────────────────────────────────────────
        case 'f':
        case 'F':
        {
            double d = arg.isNumber() ? arg.asNumber() : 0.0;
            std::string spec = "%";
            if (flagPlus)
                spec += '+';
            else if (flagSpace)
                spec += ' ';
            if (prec >= 0)
                spec += "." + std::to_string(prec);
            spec += 'f';
            std::snprintf(buf, sizeof(buf), spec.c_str(), d);
            out += pad(std::string(buf), true);
            break;
        }
        case 'e':
        case 'E':
        {
            double d = arg.isNumber() ? arg.asNumber() : 0.0;
            std::string spec = "%";
            if (prec >= 0)
                spec += "." + std::to_string(prec);
            spec += conv;
            std::snprintf(buf, sizeof(buf), spec.c_str(), d);
            out += pad(std::string(buf), true);
            break;
        }
        case 'g':
        case 'G':
        {
            double d = arg.isNumber() ? arg.asNumber() : 0.0;
            std::string spec = "%";
            if (prec >= 0)
                spec += "." + std::to_string(prec);
            spec += conv;
            std::snprintf(buf, sizeof(buf), spec.c_str(), d);
            out += pad(std::string(buf), true);
            break;
        }

        // ── String specifier ─────────────────────────────────────────────
        case 's':
        {
            std::string s = arg.toString();
            if (prec >= 0 && (int)s.size() > prec)
                s = s.substr(0, prec); // truncate to precision
            out += pad(s, false);
            break;
        }

        // ── Char specifier ────────────────────────────────────────────────
        case 'c':
        {
            char c = arg.isString() && !arg.asString().empty()
                         ? arg.asString()[0]
                         : (char)(arg.isNumber() ? (int)arg.asNumber() : 0);
            out += pad(std::string(1, c), false);
            break;
        }

        // ── Hex specifiers ────────────────────────────────────────────────
        case 'x':
        case 'X':
        {
            unsigned long long n = (unsigned long long)(long long)(arg.isNumber() ? arg.asNumber() : 0.0);
            std::string spec = "%";
            if (flagHash)
                spec += '#';
            if (prec >= 0)
                spec += "." + std::to_string(prec);
            spec += (conv == 'x') ? "llx" : "llX";
            std::snprintf(buf, sizeof(buf), spec.c_str(), n);
            std::string s = buf;
            // prefix 0x/0X if not already there (flagHash adds it)
            if (flagHash && n != 0 && s.substr(0, 2) != "0x" && s.substr(0, 2) != "0X")
                s = (conv == 'x' ? "0x" : "0X") + s;
            out += pad(s, true);
            break;
        }

        // ── Octal specifier ───────────────────────────────────────────────
        case 'o':
        {
            unsigned long long n = (unsigned long long)(long long)(arg.isNumber() ? arg.asNumber() : 0.0);
            std::snprintf(buf, sizeof(buf), flagHash ? "%#llo" : "%llo", n);
            out += pad(std::string(buf), true);
            break;
        }

        // ── Binary specifier (non-standard, Quantum extension) ────────────
        case 'b':
        {
            long long n = arg.isNumber() ? (long long)arg.asNumber() : 0LL;
            if (n == 0)
            {
                out += pad(flagHash ? "0b0" : "0", true);
            }
            else
            {
                std::string bits;
                unsigned long long u = (unsigned long long)n;
                while (u)
                {
                    bits = (char)('0' + (u & 1)) + bits;
                    u >>= 1;
                }
                if (flagHash)
                    bits = "0b" + bits;
                out += pad(bits, true);
            }
            break;
        }

        // ── Boolean specifier (Quantum extension) ─────────────────────────
        case 'B':
        {
            out += pad(arg.isTruthy() ? "true" : "false", false);
            break;
        }

        // ── Type name specifier (Quantum extension) ───────────────────────
        case 't':
        {
            out += pad(arg.typeName(), false);
            break;
        }

        default:
            out += '%';
            out += conv;
            break;
        }
    }
    return out;
}

// ─── Constructor ─────────────────────────────────────────────────────────────

Interpreter::Interpreter()
{
    globals = std::make_shared<Environment>();
    env = globals;
    registerNatives();
}

void Interpreter::registerNatives()
{
    auto reg = [&](const std::string &name, QuantumNativeFunc fn)
    {
        auto nat = std::make_shared<QuantumNative>();
        nat->name = name;
        nat->fn = std::move(fn);
        globals->define(name, QuantumValue(nat));
    };

    // I/O
    reg("__input__", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (!args.empty()) std::cout << args[0].toString();
        std::string line;
        std::getline(std::cin, line);
        return QuantumValue(line); });

    reg("input", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (!args.empty()) std::cout << args[0].toString();
        std::string line;
        std::getline(std::cin, line);
        return QuantumValue(line); });

    reg("scanf", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        // args: format string, then pointer args (&var) to write values into
        if (args.empty()) throw RuntimeError("scanf() requires at least a format string");
        std::string fmt = args[0].isString() ? args[0].asString() : args[0].toString();

        // Collect pointer targets (skip format string)
        std::vector<std::shared_ptr<QuantumPointer>> targets;
        for (size_t i = 1; i < args.size(); i++) {
            if (args[i].isPointer()) {
                targets.push_back(args[i].asPointer());
            }
        }

        // Read one line of input
        std::string input;
        std::getline(std::cin, input);

        // Parse format specifiers and write values back through pointers
        size_t targetIdx = 0;
        std::istringstream iss(input);
        size_t fi = 0;
        while (fi < fmt.size() && targetIdx < targets.size()) {
            if (fmt[fi] == '%' && fi + 1 < fmt.size()) {
                char spec = fmt[fi + 1];
                fi += 2;
                auto& ptr = targets[targetIdx++];
                if (!ptr || ptr->isNull()) continue;
                if (spec == 'd' || spec == 'i') {
                    long long v = 0; iss >> v;
                    ptr->deref() = QuantumValue((double)v);
                } else if (spec == 'f' || spec == 'g' || spec == 'e') {
                    double v = 0; iss >> v;
                    ptr->deref() = QuantumValue(v);
                } else if (spec == 's') {
                    std::string v; iss >> v;
                    ptr->deref() = QuantumValue(v);
                } else if (spec == 'c') {
                    char c = 0; iss >> c;
                    ptr->deref() = QuantumValue(std::string(1, c));
                } else {
                    // Unknown specifier: just read a token
                    std::string v; iss >> v;
                    ptr->deref() = QuantumValue(v);
                }
            } else {
                fi++;
            }
        }

        // If no pointer args (legacy call), return the raw input string
        if (targets.empty()) return QuantumValue(input);
        return QuantumValue(); });

    // Type conversion
    reg("num", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("num() requires 1 argument");
        if (args[0].isNumber()) return args[0];
        if (args[0].isString()) {
            try { return QuantumValue(std::stod(args[0].asString())); }
            catch (...) { throw TypeError("Cannot convert '" + args[0].asString() + "' to number"); }
        }
        if (args[0].isBool()) return QuantumValue(args[0].asBool() ? 1.0 : 0.0);
        throw TypeError("Cannot convert to number"); });

    reg("str", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("str() requires 1 argument");
        return QuantumValue(args[0].toString()); });

    reg("bool", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("bool() requires 1 argument");
        return QuantumValue(args[0].isTruthy()); });

    // Math
    reg("abs", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::abs(toNum(a[0], "abs"))); });
    reg("sqrt", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::sqrt(toNum(a[0], "sqrt"))); });
    reg("floor", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::floor(toNum(a[0], "floor"))); });
    reg("ceil", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::ceil(toNum(a[0], "ceil"))); });
    reg("round", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::round(toNum(a[0], "round"))); });
    reg("pow", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::pow(toNum(a[0], "pow"), toNum(a[1], "pow"))); });
    reg("log", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::log(toNum(a[0], "log"))); });
    reg("log2", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::log2(toNum(a[0], "log2"))); });
    reg("sin", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::sin(toNum(a[0], "sin"))); });
    reg("cos", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::cos(toNum(a[0], "cos"))); });
    reg("tan", [](std::vector<QuantumValue> a) -> QuantumValue
        { return QuantumValue(std::tan(toNum(a[0], "tan"))); });
    reg("min", [](std::vector<QuantumValue> a) -> QuantumValue
        {
        if (a.empty()) throw RuntimeError("min() expected at least 1 argument");
        // min(array)  — single iterable argument
        if (a.size() == 1 && a[0].isArray()) {
            auto &arr = *a[0].asArray();
            if (arr.empty()) throw RuntimeError("min() arg is an empty sequence");
            double m = toNum(arr[0], "min");
            for (size_t i = 1; i < arr.size(); i++) m = std::min(m, toNum(arr[i], "min"));
            return QuantumValue(m);
        }
        // min(a, b, c, ...)  — multiple numeric arguments
        double m = toNum(a[0], "min");
        for (size_t i = 1; i < a.size(); i++) m = std::min(m, toNum(a[i], "min"));
        return QuantumValue(m); });
    reg("max", [](std::vector<QuantumValue> a) -> QuantumValue
        {
        if (a.empty()) throw RuntimeError("max() expected at least 1 argument");
        // max(array)  — single iterable argument
        if (a.size() == 1 && a[0].isArray()) {
            auto &arr = *a[0].asArray();
            if (arr.empty()) throw RuntimeError("max() arg is an empty sequence");
            double m = toNum(arr[0], "max");
            for (size_t i = 1; i < arr.size(); i++) m = std::max(m, toNum(arr[i], "max"));
            return QuantumValue(m);
        }
        // max(a, b, c, ...)  — multiple numeric arguments
        double m = toNum(a[0], "max");
        for (size_t i = 1; i < a.size(); i++) m = std::max(m, toNum(a[i], "max"));
        return QuantumValue(m); });

    // Constants
    globals->define("PI", QuantumValue(M_PI));
    globals->define("E", QuantumValue(M_E));
    globals->define("INF", QuantumValue(std::numeric_limits<double>::infinity()));
    // JavaScript compatibility aliases
    globals->define("null", QuantumValue());
    globals->define("undefined", QuantumValue());
    globals->define("NaN", QuantumValue(std::numeric_limits<double>::quiet_NaN()));
    // Python dunder globals
    globals->define("__name__", QuantumValue(std::string("__main__")));
    globals->define("__file__", QuantumValue(std::string("")));
    globals->define("__doc__", QuantumValue());
    globals->define("__package__", QuantumValue());
    globals->define("__spec__", QuantumValue());

    // Utility
    reg("len", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("len() requires 1 argument");
        if (args[0].isString()) return QuantumValue((double)args[0].asString().size());
        if (args[0].isArray())  return QuantumValue((double)args[0].asArray()->size());
        if (args[0].isDict())   return QuantumValue((double)args[0].asDict()->size());
        throw TypeError("len() not supported for type " + args[0].typeName()); });

    reg("type", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("type() requires 1 argument");
        return QuantumValue(args[0].typeName()); });

    // isinstance(val, type_name_str_or_type)
    // Supports Python-style: isinstance(x, int), isinstance(x, str), isinstance(x, list), etc.
    reg("isinstance", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.size() < 2) throw RuntimeError("isinstance() requires 2 arguments");
        auto &val = args[0];
        // Second arg: string type name, or a native/class constructor whose name we check
        std::string typeName;
        if (args[1].isString())
            typeName = args[1].asString();
        else if (std::holds_alternative<std::shared_ptr<QuantumNative>>(args[1].data))
            typeName = args[1].asNative()->name;
        else
            typeName = args[1].typeName();

        // Normalise Python type names → Quantum type names
        auto matches = [&](const std::string &t) -> bool
        {
            if (t == "int" || t == "float" || t == "number" || t == "double")
                return val.isNumber();
            if (t == "str" || t == "string")
                return val.isString();
            if (t == "bool")
                return val.isBool();
            if (t == "list" || t == "array" || t == "tuple")
                return val.isArray();
            if (t == "dict")
                return val.isDict();
            if (t == "NoneType" || t == "nil")
                return val.isNil();
            if (t == "function" || t == "callable")
                return val.isFunction();
            return val.typeName() == t;
        };
        return QuantumValue(matches(typeName)); });

    // id() — Python identity function (returns a fake unique number here)
    reg("id", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("id() requires 1 argument");
        // Return a stable pseudo-id based on pointer or value
        return QuantumValue(0.0); });

    reg("range", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("range() requires arguments");
        double start = 0, end_, step = 1;
        if (args.size() == 1) { end_ = toNum(args[0], "range"); }
        else if (args.size() == 2) { start = toNum(args[0],"range"); end_ = toNum(args[1],"range"); }
        else { start = toNum(args[0],"range"); end_ = toNum(args[1],"range"); step = toNum(args[2],"range"); }
        auto arr = std::make_shared<Array>();
        if (step > 0) for (double i = start; i < end_; i += step) arr->push_back(QuantumValue(i));
        else          for (double i = start; i > end_; i += step) arr->push_back(QuantumValue(i));
        return QuantumValue(arr); });

    // enumerate(iterable, start=0) → array of [index, value] pairs
    reg("enumerate", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("enumerate() requires an argument");
        double startIdx = args.size() > 1 ? toNum(args[1], "enumerate") : 0;
        auto result = std::make_shared<Array>();
        auto addPair = [&](double i, QuantumValue v) {
            auto pair = std::make_shared<Array>();
            pair->push_back(QuantumValue(i));
            pair->push_back(std::move(v));
            result->push_back(QuantumValue(pair));
        };
        if (args[0].isArray()) {
            double i = startIdx;
            for (auto &v : *args[0].asArray()) addPair(i++, v);
        } else if (args[0].isString()) {
            double i = startIdx;
            for (char c : args[0].asString()) addPair(i++, QuantumValue(std::string(1,c)));
        }
        return QuantumValue(result); });

    // sum(iterable, start=0)
    reg("sum", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("sum() requires an argument");
        double total = args.size() > 1 ? toNum(args[1], "sum") : 0.0;
        if (args[0].isArray())
            for (auto &v : *args[0].asArray()) total += toNum(v, "sum");
        return QuantumValue(total); });

    // any(iterable) / all(iterable)
    reg("any", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty() || !args[0].isArray()) return QuantumValue(false);
        for (auto &v : *args[0].asArray()) if (v.isTruthy()) return QuantumValue(true);
        return QuantumValue(false); });
    reg("all", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty() || !args[0].isArray()) return QuantumValue(true);
        for (auto &v : *args[0].asArray()) if (!v.isTruthy()) return QuantumValue(false);
        return QuantumValue(true); });

    // sorted(iterable, reverse=False)
    reg("sorted", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("sorted() requires an argument");
        std::shared_ptr<Array> arr;
        if (args[0].isArray()) arr = std::make_shared<Array>(*args[0].asArray());
        else { arr = std::make_shared<Array>(); }
        bool rev = args.size() > 1 && args[1].isTruthy();
        std::sort(arr->begin(), arr->end(), [&](const QuantumValue &a, const QuantumValue &b) {
            if (a.isNumber() && b.isNumber())
                return rev ? a.asNumber() > b.asNumber() : a.asNumber() < b.asNumber();
            return rev ? a.toString() > b.toString() : a.toString() < b.toString();
        });
        return QuantumValue(arr); });

    reg("rand", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        static std::mt19937 rng(std::random_device{}());
        if (args.size() >= 2) {
            double lo = toNum(args[0],"rand"), hi = toNum(args[1],"rand");
            std::uniform_real_distribution<double> dist(lo, hi);
            return QuantumValue(dist(rng));
        }
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return QuantumValue(dist(rng)); });

    reg("rand_int", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        static std::mt19937 rng(std::random_device{}());
        double lo = args.size()>=2 ? toNum(args[0],"rand_int") : 0;
        double hi = args.size()>=2 ? toNum(args[1],"rand_int") : toNum(args[0],"rand_int");
        std::uniform_int_distribution<long long> dist((long long)lo, (long long)hi);
        return QuantumValue((double)dist(rng)); });

    reg("time", [](std::vector<QuantumValue>) -> QuantumValue
        {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        return QuantumValue((double)std::chrono::duration_cast<std::chrono::milliseconds>(now).count() / 1000.0); });

    reg("sleep", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("sleep() requires seconds argument");
        long long ms = (long long)(toNum(args[0],"sleep") * 1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        return QuantumValue(); });

    reg("exit", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        int code = args.empty() ? 0 : (int)toNum(args[0],"exit");
        std::exit(code); });

    reg("assert", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty() || !args[0].isTruthy()) {
            std::string msg = args.size() > 1 ? args[1].toString() : "Assertion failed";
            throw RuntimeError(msg);
        }
        return QuantumValue(); });

    // String building
    reg("chr", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        int code = (int)toNum(args[0],"chr");
        return QuantumValue(std::string(1, (char)code)); });

    reg("ord", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (!args[0].isString() || args[0].asString().empty()) throw TypeError("ord() expects non-empty string");
        return QuantumValue((double)(unsigned char)args[0].asString()[0]); });

    reg("hex", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        std::ostringstream oss;
        oss << "0x" << std::hex << std::uppercase << (long long)toNum(args[0],"hex");
        return QuantumValue(oss.str()); });

    reg("bin", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        long long n = (long long)toNum(args[0],"bin");
        if (n == 0) return QuantumValue(std::string("0b0"));
        std::string res;
        long long tmp = n;
        while (tmp) { res = (char)('0' + (tmp & 1)) + res; tmp >>= 1; }
        return QuantumValue("0b" + res); });

    // Array
    reg("array", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        auto arr = std::make_shared<Array>();
        if (!args.empty()) arr->resize((size_t)toNum(args[0],"array"), args.size()>1 ? args[1] : QuantumValue());
        return QuantumValue(arr); });

    reg("keys", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (!args[0].isDict()) throw TypeError("keys() expects dict");
        auto arr = std::make_shared<Array>();
        for (auto& [k,v] : *args[0].asDict()) arr->push_back(QuantumValue(k));
        return QuantumValue(arr); });

    reg("values", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (!args[0].isDict()) throw TypeError("values() expects dict");
        auto arr = std::make_shared<Array>();
        for (auto& [k,v] : *args[0].asDict()) arr->push_back(v);
        return QuantumValue(arr); });

    // ─── Cybersecurity builtins (future expansion base) ────────────────────
    reg("xor_bytes", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString())
            throw TypeError("xor_bytes() expects two strings");
        const std::string& a = args[0].asString();
        const std::string& b = args[1].asString();
        std::string result(a.size(), '\0');
        for (size_t i = 0; i < a.size(); i++)
            result[i] = a[i] ^ b[i % b.size()];
        return QuantumValue(result); });

    reg("to_hex", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (!args[0].isString()) throw TypeError("to_hex() expects string");
        std::ostringstream oss;
        for (unsigned char c : args[0].asString())
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        return QuantumValue(oss.str()); });

    reg("from_hex", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (!args[0].isString()) throw TypeError("from_hex() expects string");
        const std::string& h = args[0].asString();
        std::string result;
        for (size_t i = 0; i + 1 < h.size(); i += 2)
            result += (char)std::stoi(h.substr(i, 2), nullptr, 16);
        return QuantumValue(result); });

    reg("rot13", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (!args[0].isString()) throw TypeError("rot13() expects string");
        std::string s = args[0].asString();
        for (char& c : s) {
            if (std::isalpha(c)) {
                char base = std::islower(c) ? 'a' : 'A';
                c = base + (c - base + 13) % 26;
            }
        }
        return QuantumValue(s); });

    reg("base64_encode", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (!args[0].isString()) throw TypeError("base64_encode() expects string");
        static const char* B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        const std::string& in = args[0].asString();
        std::string out;
        for (size_t i = 0; i < in.size(); i += 3) {
            unsigned char b0 = in[i], b1 = i+1<in.size()?in[i+1]:0, b2 = i+2<in.size()?in[i+2]:0;
            out += B64[b0 >> 2];
            out += B64[((b0 & 3) << 4) | (b1 >> 4)];
            out += i+1<in.size() ? B64[((b1 & 0xf) << 2) | (b2 >> 6)] : '=';
            out += i+2<in.size() ? B64[b2 & 0x3f] : '=';
        }
        return QuantumValue(out); });

    // ─── Formatted output / string building ───────────────────────────────
    // printf("fmt", args...)  — print formatted, no implicit newline
    // printf("fmt", args...) — C-style printf: resolves pointer args, prints formatted string
    reg("printf", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("printf() requires a format string");
        std::vector<QuantumValue> resolved;
        resolved.reserve(args.size());
        for (auto& a : args) {
            if (a.isPointer()) {
                auto ptr = a.asPointer();
                resolved.push_back(ptr && !ptr->isNull() ? ptr->deref() : QuantumValue());
            } else {
                resolved.push_back(a);
            }
        }
        std::cout << applyFormat(resolved[0].toString(), resolved, 1);
        std::cout.flush();
        return QuantumValue(); });

    reg("__printf__", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("printf() requires a format string");
        std::cout << applyFormat(args[0].toString(), args, 1);
        std::cout.flush();
        return QuantumValue(); });

    // format("fmt", args...)  — return formatted string (like sprintf)
    reg("format", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("format() requires a format string");
        return QuantumValue(applyFormat(args[0].toString(), args, 1)); });

    // sprintf("fmt", args...)  — alias for format()
    reg("sprintf", [](std::vector<QuantumValue> args) -> QuantumValue
        {
        if (args.empty()) throw RuntimeError("sprintf() requires a format string");
        return QuantumValue(applyFormat(args[0].toString(), args, 1)); });

    // ─── Math object (JavaScript compatibility) ──────────────────────────
    auto mathDict = std::make_shared<Dict>();

    auto mathNative = [&](const std::string &name, QuantumNativeFunc fn) -> QuantumValue
    {
        auto nat = std::make_shared<QuantumNative>();
        nat->name = name;
        nat->fn = std::move(fn);
        return QuantumValue(nat);
    };

    (*mathDict)["PI"] = QuantumValue(M_PI);
    (*mathDict)["E"] = QuantumValue(M_E);
    (*mathDict)["LN2"] = QuantumValue(std::log(2.0));
    (*mathDict)["LN10"] = QuantumValue(std::log(10.0));
    (*mathDict)["LOG2E"] = QuantumValue(std::log2(M_E));
    (*mathDict)["LOG10E"] = QuantumValue(std::log10(M_E));
    (*mathDict)["SQRT2"] = QuantumValue(std::sqrt(2.0));
    (*mathDict)["Infinity"] = QuantumValue(std::numeric_limits<double>::infinity());

    (*mathDict)["floor"] = mathNative("Math.floor", [](std::vector<QuantumValue> a) -> QuantumValue
                                      { return QuantumValue(std::floor(toNum(a[0], "Math.floor"))); });
    (*mathDict)["ceil"] = mathNative("Math.ceil", [](std::vector<QuantumValue> a) -> QuantumValue
                                     { return QuantumValue(std::ceil(toNum(a[0], "Math.ceil"))); });
    (*mathDict)["round"] = mathNative("Math.round", [](std::vector<QuantumValue> a) -> QuantumValue
                                      { return QuantumValue(std::round(toNum(a[0], "Math.round"))); });
    (*mathDict)["abs"] = mathNative("Math.abs", [](std::vector<QuantumValue> a) -> QuantumValue
                                    { return QuantumValue(std::abs(toNum(a[0], "Math.abs"))); });
    (*mathDict)["sqrt"] = mathNative("Math.sqrt", [](std::vector<QuantumValue> a) -> QuantumValue
                                     { return QuantumValue(std::sqrt(toNum(a[0], "Math.sqrt"))); });
    (*mathDict)["cbrt"] = mathNative("Math.cbrt", [](std::vector<QuantumValue> a) -> QuantumValue
                                     { return QuantumValue(std::cbrt(toNum(a[0], "Math.cbrt"))); });
    (*mathDict)["pow"] = mathNative("Math.pow", [](std::vector<QuantumValue> a) -> QuantumValue
                                    { return QuantumValue(std::pow(toNum(a[0], "Math.pow"), toNum(a[1], "Math.pow"))); });
    (*mathDict)["log"] = mathNative("Math.log", [](std::vector<QuantumValue> a) -> QuantumValue
                                    { return QuantumValue(std::log(toNum(a[0], "Math.log"))); });
    (*mathDict)["log2"] = mathNative("Math.log2", [](std::vector<QuantumValue> a) -> QuantumValue
                                     { return QuantumValue(std::log2(toNum(a[0], "Math.log2"))); });
    (*mathDict)["log10"] = mathNative("Math.log10", [](std::vector<QuantumValue> a) -> QuantumValue
                                      { return QuantumValue(std::log10(toNum(a[0], "Math.log10"))); });
    (*mathDict)["sin"] = mathNative("Math.sin", [](std::vector<QuantumValue> a) -> QuantumValue
                                    { return QuantumValue(std::sin(toNum(a[0], "Math.sin"))); });
    (*mathDict)["cos"] = mathNative("Math.cos", [](std::vector<QuantumValue> a) -> QuantumValue
                                    { return QuantumValue(std::cos(toNum(a[0], "Math.cos"))); });
    (*mathDict)["tan"] = mathNative("Math.tan", [](std::vector<QuantumValue> a) -> QuantumValue
                                    { return QuantumValue(std::tan(toNum(a[0], "Math.tan"))); });
    (*mathDict)["asin"] = mathNative("Math.asin", [](std::vector<QuantumValue> a) -> QuantumValue
                                     { return QuantumValue(std::asin(toNum(a[0], "Math.asin"))); });
    (*mathDict)["acos"] = mathNative("Math.acos", [](std::vector<QuantumValue> a) -> QuantumValue
                                     { return QuantumValue(std::acos(toNum(a[0], "Math.acos"))); });
    (*mathDict)["atan"] = mathNative("Math.atan", [](std::vector<QuantumValue> a) -> QuantumValue
                                     { return QuantumValue(std::atan(toNum(a[0], "Math.atan"))); });
    (*mathDict)["atan2"] = mathNative("Math.atan2", [](std::vector<QuantumValue> a) -> QuantumValue
                                      { return QuantumValue(std::atan2(toNum(a[0], "Math.atan2"), toNum(a[1], "Math.atan2"))); });
    (*mathDict)["exp"] = mathNative("Math.exp", [](std::vector<QuantumValue> a) -> QuantumValue
                                    { return QuantumValue(std::exp(toNum(a[0], "Math.exp"))); });
    (*mathDict)["sign"] = mathNative("Math.sign", [](std::vector<QuantumValue> a) -> QuantumValue
                                     { double v = toNum(a[0],"Math.sign"); return QuantumValue(v > 0 ? 1.0 : v < 0 ? -1.0 : 0.0); });
    (*mathDict)["trunc"] = mathNative("Math.trunc", [](std::vector<QuantumValue> a) -> QuantumValue
                                      { return QuantumValue(std::trunc(toNum(a[0], "Math.trunc"))); });
    (*mathDict)["hypot"] = mathNative("Math.hypot", [](std::vector<QuantumValue> a) -> QuantumValue
                                      {
        double s = 0; for (auto& x : a) { double v = toNum(x,"Math.hypot"); s += v*v; } return QuantumValue(std::sqrt(s)); });
    (*mathDict)["min"] = mathNative("Math.min", [](std::vector<QuantumValue> a) -> QuantumValue
                                    {
        if (a.empty()) return QuantumValue(std::numeric_limits<double>::infinity());
        double m = toNum(a[0],"Math.min"); for (size_t i=1;i<a.size();i++) m = std::min(m, toNum(a[i],"Math.min")); return QuantumValue(m); });
    (*mathDict)["max"] = mathNative("Math.max", [](std::vector<QuantumValue> a) -> QuantumValue
                                    {
        if (a.empty()) return QuantumValue(-std::numeric_limits<double>::infinity());
        double m = toNum(a[0],"Math.max"); for (size_t i=1;i<a.size();i++) m = std::max(m, toNum(a[i],"Math.max")); return QuantumValue(m); });
    (*mathDict)["random"] = mathNative("Math.random", [](std::vector<QuantumValue>) -> QuantumValue
                                       {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return QuantumValue(dist(rng)); });
    (*mathDict)["clamp"] = mathNative("Math.clamp", [](std::vector<QuantumValue> a) -> QuantumValue
                                      {
        double v = toNum(a[0],"Math.clamp"), lo = toNum(a[1],"Math.clamp"), hi = toNum(a[2],"Math.clamp");
        return QuantumValue(std::max(lo, std::min(hi, v))); });

    globals->define("Math", QuantumValue(mathDict));
    // console.log, console.warn, console.error all print space-joined args
    auto consolePrint = [](const std::string &prefix, std::ostream &out)
    {
        return [prefix, &out](std::vector<QuantumValue> args) -> QuantumValue
        {
            if (!prefix.empty())
                out << prefix;
            for (size_t i = 0; i < args.size(); i++)
            {
                if (i)
                    out << " ";
                out << args[i].toString();
            }
            out << "\n";
            out.flush();
            return QuantumValue();
        };
    };

    auto consoleDict = std::make_shared<Dict>();

    auto makeNative = [&](const std::string &name, QuantumNativeFunc fn) -> QuantumValue
    {
        auto nat = std::make_shared<QuantumNative>();
        nat->name = name;
        nat->fn = std::move(fn);
        return QuantumValue(nat);
    };

    (*consoleDict)["log"] = makeNative("console.log",
                                       [](std::vector<QuantumValue> args) -> QuantumValue
                                       {
            for (size_t i = 0; i < args.size(); i++) { if (i) std::cout << " "; std::cout << args[i].toString(); }
            std::cout << "\n"; std::cout.flush(); return QuantumValue(); });

    (*consoleDict)["warn"] = makeNative("console.warn",
                                        [](std::vector<QuantumValue> args) -> QuantumValue
                                        {
            std::cout << "[warn] ";
            for (size_t i = 0; i < args.size(); i++) { if (i) std::cout << " "; std::cout << args[i].toString(); }
            std::cout << "\n"; std::cout.flush(); return QuantumValue(); });

    (*consoleDict)["error"] = makeNative("console.error",
                                         [](std::vector<QuantumValue> args) -> QuantumValue
                                         {
            std::cerr << "[error] ";
            for (size_t i = 0; i < args.size(); i++) { if (i) std::cerr << " "; std::cerr << args[i].toString(); }
            std::cerr << "\n"; std::cerr.flush(); return QuantumValue(); });

    globals->define("console", QuantumValue(consoleDict));

    // ── Python-style exception constructors ────────────────────────────────
    // These allow  raise ValueError("msg")  to produce a meaningful string.
    // When used with raise/throw the interpreter converts to RuntimeError.
    auto makeExcClass = [&](const std::string &excName)
    {
        auto nat = std::make_shared<QuantumNative>();
        nat->name = excName;
        nat->fn = [excName](std::vector<QuantumValue> args) -> QuantumValue
        {
            std::string msg = excName;
            if (!args.empty())
                msg += ": " + args[0].toString();
            return QuantumValue(msg);
        };
        globals->define(excName, QuantumValue(nat));
    };
    makeExcClass("ValueError");
    makeExcClass("TypeError");
    makeExcClass("RuntimeError");
    makeExcClass("IndexError");
    makeExcClass("KeyError");
    makeExcClass("AttributeError");
    makeExcClass("NotImplementedError");
    makeExcClass("StopIteration");
    makeExcClass("OverflowError");
    makeExcClass("ZeroDivisionError");
    makeExcClass("IOError");
    makeExcClass("FileNotFoundError");
    makeExcClass("PermissionError");
    makeExcClass("Exception");
    makeExcClass("Error");          // JS alias
    makeExcClass("RangeError");     // JS alias
    makeExcClass("ReferenceError"); // JS alias

    // ── Python built-in type tokens as first-class values ─────────────────
    // Allows:  isinstance(x, int)  isinstance(x, str)  isinstance(x, list)
    // These reserved keywords (TYPE_INT etc.) become Identifier{"int"} in the
    // AST; the interpreter looks them up here rather than throwing NameError.
    auto makeTypeVal = [&](const std::string &name)
    {
        auto nat = std::make_shared<QuantumNative>();
        nat->name = name;
        // When called as a constructor/cast: int(x), str(x), list(x), etc.
        nat->fn = [name](std::vector<QuantumValue> args) -> QuantumValue
        {
            if (args.empty())
                return QuantumValue();
            auto &v = args[0];
            if (name == "int" || name == "long" || name == "short")
            {
                if (v.isNumber())
                    return QuantumValue(std::floor(v.asNumber()));
                if (v.isString())
                {
                    try
                    {
                        return QuantumValue((double)(long long)std::stod(v.asString()));
                    }
                    catch (...)
                    {
                    }
                }
                if (v.isBool())
                    return QuantumValue(v.asBool() ? 1.0 : 0.0);
                throw TypeError("int() cannot convert " + v.typeName());
            }
            if (name == "float" || name == "double")
            {
                if (v.isNumber())
                    return v;
                if (v.isString())
                {
                    try
                    {
                        return QuantumValue(std::stod(v.asString()));
                    }
                    catch (...)
                    {
                    }
                }
                if (v.isBool())
                    return QuantumValue(v.asBool() ? 1.0 : 0.0);
                throw TypeError("float() cannot convert " + v.typeName());
            }
            if (name == "str" || name == "string")
                return QuantumValue(v.toString());
            if (name == "bool")
                return QuantumValue(v.isTruthy());
            if (name == "list" || name == "tuple")
            {
                if (v.isArray())
                    return v;
                auto arr = std::make_shared<Array>();
                if (v.isString())
                {
                    for (char c : v.asString())
                        arr->push_back(QuantumValue(std::string(1, c)));
                }
                else
                    arr->push_back(v);
                return QuantumValue(arr);
            }
            if (name == "dict")
            {
                if (v.isDict())
                    return v;
                return QuantumValue(std::make_shared<Dict>());
            }
            return v;
        };
        globals->define(name, QuantumValue(nat));
    };
    makeTypeVal("int");
    makeTypeVal("float");
    makeTypeVal("double");
    makeTypeVal("str");
    makeTypeVal("bool");
    makeTypeVal("list");
    makeTypeVal("tuple");
    makeTypeVal("dict");
    makeTypeVal("long");
    makeTypeVal("short");
    makeTypeVal("char");

    // ── isinstance(obj, klass) ──────────────────────────────────────────────
    {
        auto nat = std::make_shared<QuantumNative>();
        nat->name = "isinstance";
        nat->fn = [](std::vector<QuantumValue> args) -> QuantumValue
        {
            if (args.size() < 2)
                throw RuntimeError("isinstance() requires 2 arguments");
            if (!args[0].isInstance())
                return QuantumValue(false);
            if (!args[1].isClass())
                return QuantumValue(false);
            auto inst = args[0].asInstance();
            auto targetKlass = args[1].asClass();
            // Walk the inheritance chain
            auto k = inst->klass.get();
            while (k)
            {
                if (k == targetKlass.get())
                    return QuantumValue(true);
                k = k->base.get();
            }
            return QuantumValue(false);
        };
        globals->define("isinstance", QuantumValue(nat));
    }

    // ── classname(obj) — get class name as string ───────────────────────────
    {
        auto nat = std::make_shared<QuantumNative>();
        nat->name = "classname";
        nat->fn = [](std::vector<QuantumValue> args) -> QuantumValue
        {
            if (args.empty())
                return QuantumValue(std::string("nil"));
            if (args[0].isInstance())
                return QuantumValue(args[0].asInstance()->klass->name);
            return QuantumValue(args[0].typeName());
        };
        globals->define("classname", QuantumValue(nat));
    }

    // ── __format__(obj, specifier) — internal f-string format helper ────────
    {
        auto nat = std::make_shared<QuantumNative>();
        nat->name = "__format__";
        nat->fn = [](std::vector<QuantumValue> args) -> QuantumValue
        {
            if (args.size() < 2)
                return QuantumValue(std::string(""));
            std::string valStr = args[0].toString();
            std::string spec = args[1].asString();

            // Basic handling for Python format specifiers like .2f
            if (spec.size() >= 3 && spec[0] == '.' && spec.back() == 'f' && args[0].isNumber())
            {
                int precision = std::stoi(spec.substr(1, spec.size() - 2));
                std::ostringstream out;
                out.precision(precision);
                out << std::fixed << args[0].asNumber();
                return QuantumValue(out.str());
            }

            // Basic handling for alignment like *^30 or >10
            if (spec.size() >= 2)
            {
                char fill = ' ';
                char align = spec[0];
                size_t widthIdx = 1;

                if (spec.size() >= 3 && (spec[1] == '<' || spec[1] == '>' || spec[1] == '^'))
                {
                    fill = spec[0];
                    align = spec[1];
                    widthIdx = 2;
                }

                if (align == '<' || align == '>' || align == '^')
                {
                    try
                    {
                        int width = std::stoi(spec.substr(widthIdx));
                        int len = valStr.size();
                        if (width > len)
                        {
                            if (align == '<')
                            {
                                valStr = valStr + std::string(width - len, fill);
                            }
                            else if (align == '>')
                            {
                                valStr = std::string(width - len, fill) + valStr;
                            }
                            else if (align == '^')
                            {
                                int padLeft = (width - len) / 2;
                                int padRight = width - len - padLeft;
                                valStr = std::string(padLeft, fill) + valStr + std::string(padRight, fill);
                            }
                        }
                    }
                    catch (...)
                    {
                    }
                }
            }

            return QuantumValue(valStr);
        };
        globals->define("__format__", QuantumValue(nat));
    }

    // ── nullptr and NULL constants ─────────────────────────────────────────
    {
        auto nullPtr = std::make_shared<QuantumPointer>();
        nullPtr->cell = nullptr;
        nullPtr->varName = "nullptr";
        nullPtr->offset = 0;
        globals->define("nullptr", QuantumValue(nullPtr));
        globals->define("NULL", QuantumValue(nullPtr));
    }
}

// ─── Execute ─────────────────────────────────────────────────────────────────

void Interpreter::execute(ASTNode &node)
{
    std::visit([&](auto &n)
               {
        using T = std::decay_t<decltype(n)>;
        if constexpr (std::is_same_v<T, BlockStmt>)      execBlock(n, env); // same scope — no new env
        else if constexpr (std::is_same_v<T, VarDecl>)       execVarDecl(n);
        else if constexpr (std::is_same_v<T, FunctionDecl>)  execFunctionDecl(n);
        else if constexpr (std::is_same_v<T, ClassDecl>)     execClassDecl(n);
        else if constexpr (std::is_same_v<T, IfStmt>)        execIf(n);
        else if constexpr (std::is_same_v<T, WhileStmt>)     execWhile(n);
        else if constexpr (std::is_same_v<T, ForStmt>)       execFor(n);
        else if constexpr (std::is_same_v<T, ReturnStmt>)    execReturn(n);
        else if constexpr (std::is_same_v<T, PrintStmt>)     execPrint(n);
        else if constexpr (std::is_same_v<T, InputStmt>)     execInput(n);
        else if constexpr (std::is_same_v<T, ImportStmt>)    execImport(n);
        else if constexpr (std::is_same_v<T, ExprStmt>)      execExprStmt(n);
        else if constexpr (std::is_same_v<T, BreakStmt>)     throw BreakSignal{};
        else if constexpr (std::is_same_v<T, ContinueStmt>)  throw ContinueSignal{};
        else if constexpr (std::is_same_v<T, RaiseStmt>) {
            std::string msg = "Exception raised";
            if (n.value) {
                auto val = evaluate(*n.value);
                msg = val.toString();
            }
            throw RuntimeError(msg, node.line);
        }
        else if constexpr (std::is_same_v<T, TryStmt>) {
            auto runFinally = [&]() {
                if (n.finallyBody) execute(*n.finallyBody);
            };
            try {
                execute(*n.body);
            }
            catch (ReturnSignal&)  { runFinally(); throw; }
            catch (BreakSignal&)   { runFinally(); throw; }
            catch (ContinueSignal&){ runFinally(); throw; }
            catch (QuantumError &e) {
                bool handled = false;
                for (auto &h : n.handlers) {
                    // Match if no type specified (bare except) or type name matches
                    if (h.errorType.empty() || h.errorType == e.kind ||
                        h.errorType == "Exception" || h.errorType == "Error") {
                        if (!h.alias.empty()) {
                            if (!env->has(h.alias)) env->define(h.alias, QuantumValue(std::string(e.what())));
                            else env->set(h.alias, QuantumValue(std::string(e.what())));
                        }
                        execute(*h.body);
                        handled = true;
                        break;
                    }
                }
                if (!handled) { runFinally(); throw; }
            }
            catch (std::exception &e) {
                bool handled = false;
                for (auto &h : n.handlers) {
                    if (h.errorType.empty() || h.errorType == "Exception" || h.errorType == "Error") {
                        if (!h.alias.empty()) {
                            if (!env->has(h.alias)) env->define(h.alias, QuantumValue(std::string(e.what())));
                            else env->set(h.alias, QuantumValue(std::string(e.what())));
                        }
                        execute(*h.body);
                        handled = true;
                        break;
                    }
                }
                if (!handled) { runFinally(); throw; }
            }
            runFinally();
        }
        else {
            // Could be an expression node used as statement
            evaluate(node);
        } }, node.node);
}

void Interpreter::execBlock(BlockStmt &s, std::shared_ptr<Environment> scope)
{
    auto prev = env;
    env = scope ? scope : std::make_shared<Environment>(prev);
    try
    {
        for (auto &stmt : s.statements)
            execute(*stmt);
    }
    catch (...)
    {
        env = prev;
        throw;
    }
    env = prev;
}

void Interpreter::execVarDecl(VarDecl &s)
{
    QuantumValue val;
    if (s.initializer)
        val = evaluate(*s.initializer);

    // ── C-style type coercion ────────────────────────────────────────────────
    if (!s.typeHint.empty())
    {
        const std::string &h = s.typeHint;
        // Integer types: int, short, long, unsigned, unsigned int, long long …
        if (h == "int" || h == "short" || h == "long" ||
            h == "long long" || h == "unsigned" || h == "unsigned int" ||
            h == "unsigned long" || h == "unsigned long long" || h == "unsigned short")
        {
            if (val.isNumber())
                val = QuantumValue((double)(long long)val.asNumber());
            else if (val.isString())
            {
                try
                {
                    val = QuantumValue((double)std::stoll(val.asString()));
                }
                catch (...)
                {
                    val = QuantumValue(0.0);
                }
            }
            else if (val.isBool())
                val = QuantumValue(val.asBool() ? 1.0 : 0.0);
            else if (val.isNil())
                val = QuantumValue(0.0);
        }
        // Floating point types: float, double, long double
        else if (h == "float" || h == "double" || h == "long double")
        {
            if (val.isNumber())
                ; // already double — keep it
            else if (val.isString())
            {
                try
                {
                    val = QuantumValue(std::stod(val.asString()));
                }
                catch (...)
                {
                    val = QuantumValue(0.0);
                }
            }
            else if (val.isBool())
                val = QuantumValue(val.asBool() ? 1.0 : 0.0);
            else if (val.isNil())
                val = QuantumValue(0.0);
        }
        // char — single character stored as string
        else if (h == "char")
        {
            if (val.isString())
                val = QuantumValue(val.asString().empty() ? std::string("") : std::string(1, val.asString()[0]));
            else if (val.isNumber())
            {
                char c = (char)(int)val.asNumber();
                val = QuantumValue(std::string(1, c));
            }
            else if (val.isNil())
                val = QuantumValue(std::string("\0", 1));
        }
        // string / std::string — convert anything to string
        else if (h == "string")
        {
            val = QuantumValue(val.toString());
        }
        // bool — truthy coercion
        else if (h == "bool")
        {
            val = QuantumValue(val.isTruthy());
        }
        // void — just nil; assigning void x = ... is unusual but handled gracefully
        else if (h == "void")
        {
            val = QuantumValue(); // nil
        }
    }

    // ── Pointer variable: int* p = new int(100) / int* p = &a ────────────────
    // If the VarDecl is declared as a pointer (int* p) and the initializer
    // produced a plain value (not already a pointer), wrap it in a heap cell.
    if (s.isPointer && !val.isPointer())
    {
        if (!val.isNil())
        {
            auto cell = std::make_shared<QuantumValue>(val);
            auto ptr = std::make_shared<QuantumPointer>();
            ptr->cell = cell;
            ptr->varName = s.name;
            val = QuantumValue(ptr);
        }
    }

    env->define(s.name, std::move(val), s.isConst);
}

void Interpreter::execFunctionDecl(FunctionDecl &s)
{
    auto fn = std::make_shared<QuantumFunction>();
    fn->name = s.name;
    fn->params = s.params;
    fn->paramIsRef = s.paramIsRef;
    fn->body = s.body.get();
    fn->closure = env;
    env->define(s.name, QuantumValue(fn));
}

void Interpreter::execClassDecl(ClassDecl &s)
{
    auto klass = std::make_shared<QuantumClass>();
    klass->name = s.name;

    // Inherit from base class if specified
    if (!s.base.empty())
    {
        try
        {
            auto baseVal = env->get(s.base);
            if (baseVal.isClass())
                klass->base = baseVal.asClass();
            // If base is a native stub (e.g. ABC from our import stubs), silently skip —
            // we don't need true abstract-class enforcement at runtime.
        }
        catch (NameError &)
        {
            // Base not defined at all — treat as a rootless class rather than crashing.
            // This mirrors Python's lenient behaviour when stubs are missing.
        }
    }

    // Compile instance methods
    for (auto &m : s.methods)
    {
        if (m->is<FunctionDecl>())
        {
            auto &fd = m->as<FunctionDecl>();
            auto fn = std::make_shared<QuantumFunction>();
            fn->name = fd.name;
            fn->params = fd.params;
            fn->body = fd.body.get();
            fn->closure = env;

            // Support method overloading: if name already exists, also store under name#argcount
            if (klass->methods.count(fd.name))
            {
                // Store the existing one under name#argcount if not already done
                auto existing = klass->methods[fd.name];
                size_t existingParamStart = (!existing->params.empty() &&
                                             (existing->params[0] == "self" || existing->params[0] == "this"))
                                                ? 1
                                                : 0;
                std::string existingKey = fd.name + "#" + std::to_string(existing->params.size() - existingParamStart);
                if (!klass->methods.count(existingKey))
                    klass->methods[existingKey] = existing;
            }
            klass->methods[fd.name] = fn;

            // Always store an overload-specific version
            size_t paramStart = (!fd.params.empty() &&
                                 (fd.params[0] == "self" || fd.params[0] == "this"))
                                    ? 1
                                    : 0;
            std::string overloadKey = fd.name + "#" + std::to_string(fd.params.size() - paramStart);
            klass->methods[overloadKey] = fn;
        }
    }

    // Compile static methods
    for (auto &m : s.staticMethods)
    {
        if (m->is<FunctionDecl>())
        {
            auto &fd = m->as<FunctionDecl>();
            auto fn = std::make_shared<QuantumFunction>();
            fn->name = fd.name;
            fn->params = fd.params;
            fn->body = fd.body.get();
            fn->closure = env;
            klass->staticMethods[fd.name] = fn;
        }
    }

    // Store the class as a first-class value
    env->define(s.name, QuantumValue(klass));
}

void Interpreter::execIf(IfStmt &s)
{
    if (evaluate(*s.condition).isTruthy())
        execute(*s.thenBranch);
    else if (s.elseBranch)
        execute(*s.elseBranch);
}

void Interpreter::execWhile(WhileStmt &s)
{
    while (evaluate(*s.condition).isTruthy())
    {
        try
        {
            execute(*s.body);
        }
        catch (BreakSignal &)
        {
            break;
        }
        catch (ContinueSignal &)
        {
            continue;
        }
    }
}

void Interpreter::execFor(ForStmt &s)
{
    auto iter = evaluate(*s.iterable);
    bool hasTuple = !s.var2.empty();

    auto loop = [&](QuantumValue item)
    {
        auto scope = std::make_shared<Environment>(env);
        if (hasTuple)
        {
            // Tuple unpacking: for k, v in dict.items() or array of pairs
            if (item.isArray() && item.asArray()->size() >= 2)
            {
                scope->define(s.var, (*item.asArray())[0]);
                scope->define(s.var2, (*item.asArray())[1]);
            }
            else
            {
                scope->define(s.var, item);
                scope->define(s.var2, QuantumValue());
            }
        }
        else
            scope->define(s.var, std::move(item));
        try
        {
            execBlock(s.body->as<BlockStmt>(), scope);
        }
        catch (BreakSignal &)
        {
            throw;
        }
        catch (ContinueSignal &)
        {
        }
    };

    if (iter.isArray())
    {
        for (auto &item : *iter.asArray())
        {
            try
            {
                loop(item);
            }
            catch (BreakSignal &)
            {
                break;
            }
        }
    }
    else if (iter.isString())
    {
        for (char c : iter.asString())
        {
            try
            {
                loop(QuantumValue(std::string(1, c)));
            }
            catch (BreakSignal &)
            {
                break;
            }
        }
    }
    else if (iter.isDict())
    {
        for (auto &[k, v] : *iter.asDict())
        {
            try
            {
                loop(QuantumValue(k));
            }
            catch (BreakSignal &)
            {
                break;
            }
        }
    }
    else
    {
        throw TypeError("Cannot iterate over " + iter.typeName());
    }
}

void Interpreter::execReturn(ReturnStmt &s)
{
    QuantumValue val;
    if (s.value)
        val = evaluate(*s.value);
    throw ReturnSignal(std::move(val));
}

void Interpreter::execPrint(PrintStmt &s)
{
    if (s.args.empty())
    {
        if (s.newline)
            std::cout << "\n";
        std::cout.flush();
        return;
    }

    // Evaluate all args first
    std::vector<QuantumValue> vals;
    vals.reserve(s.args.size());
    for (auto &a : s.args)
        vals.push_back(evaluate(*a));

    // Printf-mode: first arg is a string containing a % format specifier
    // e.g. printf("x = %d\n", x)  OR  print("%d items", count)
    bool isPrintf = false;
    if (vals.size() > 1 && vals[0].isString())
    {
        const std::string &fmt = vals[0].asString();
        // Look for a real % specifier (not %%)
        for (size_t i = 0; i + 1 < fmt.size(); ++i)
        {
            if (fmt[i] == '%' && fmt[i + 1] != '%')
            {
                isPrintf = true;
                break;
            }
        }
    }

    if (isPrintf)
    {
        // Printf-style: format the string using the existing applyFormat engine
        std::cout << applyFormat(vals[0].toString(), vals, 1);
    }
    else
    {
        // Python-style: space-join all args and print
        for (size_t i = 0; i < vals.size(); i++)
        {
            if (i)
                std::cout << " ";
            // Call __str__ on instances if defined
            if (vals[i].isInstance())
            {
                auto inst = vals[i].asInstance();
                auto k = inst->klass.get();
                bool found = false;
                while (k)
                {
                    auto it = k->methods.find("__str__");
                    if (it != k->methods.end())
                    {
                        auto result = callInstanceMethod(inst, it->second, {});
                        std::cout << result.toString();
                        found = true;
                        break;
                    }
                    k = k->base.get();
                }
                if (!found)
                    std::cout << vals[i].toString();
            }
            else
            {
                std::cout << vals[i].toString();
            }
        }
        if (s.newline)
            std::cout << "\n";
    }
    std::cout.flush();
}

void Interpreter::execInput(InputStmt &s)
{
    // Evaluate the prompt string exactly once
    std::string promptStr;
    if (s.prompt)
        promptStr = evaluate(*s.prompt).toString();

    // Extract the format specifier (if any) from the prompt string
    // e.g. "%d" -> 'd',  "Enter a number: " -> 0 (no spec)
    char spec = 0;
    if (!promptStr.empty())
    {
        for (size_t i = 0; i + 1 < promptStr.size(); ++i)
        {
            if (promptStr[i] == '%' && promptStr[i + 1] != '%')
            {
                size_t j = i + 1;
                while (j < promptStr.size() && (promptStr[j] == '-' || promptStr[j] == '+' || promptStr[j] == ' ' || promptStr[j] == '0' || promptStr[j] == '#'))
                    ++j;
                while (j < promptStr.size() && std::isdigit(promptStr[j]))
                    ++j;
                if (j < promptStr.size() && promptStr[j] == '.')
                {
                    ++j;
                    while (j < promptStr.size() && std::isdigit(promptStr[j]))
                        ++j;
                }
                if (j < promptStr.size())
                    spec = promptStr[j];
                break;
            }
        }
    }

    // Strip all %specifier sequences from the prompt and print only the
    // human-readable text. e.g. "Enter number: %d" -> prints "Enter number: "
    // and "%d" alone -> prints nothing.
    if (!promptStr.empty())
    {
        std::string display;
        for (size_t i = 0; i < promptStr.size();)
        {
            if (promptStr[i] == '%')
            {
                ++i;
                if (i < promptStr.size() && promptStr[i] == '%')
                {
                    display += '%';
                    ++i; // %% -> literal %
                    continue;
                }
                // skip flags
                while (i < promptStr.size() && (promptStr[i] == '-' || promptStr[i] == '+' || promptStr[i] == ' ' || promptStr[i] == '0' || promptStr[i] == '#'))
                    ++i;
                // skip width
                while (i < promptStr.size() && std::isdigit(promptStr[i]))
                    ++i;
                // skip precision
                if (i < promptStr.size() && promptStr[i] == '.')
                {
                    ++i;
                    while (i < promptStr.size() && std::isdigit(promptStr[i]))
                        ++i;
                }
                // skip conversion char — the specifier is fully consumed, print nothing
                if (i < promptStr.size())
                    ++i;
            }
            else
            {
                display += promptStr[i++];
            }
        }
        if (!display.empty())
        {
            std::cout << display;
            std::cout.flush();
        }
    }

    std::string line;
    std::getline(std::cin, line);

    if (s.target.empty())
        return;

    QuantumValue val;
    try
    {
        switch (spec)
        {
        case 'd':
        case 'i':
        case 'u':
        {
            // Force integer
            long long n = std::stoll(line);
            val = QuantumValue((double)n);
            break;
        }
        case 'f':
        case 'e':
        case 'g':
        case 'F':
        case 'E':
        case 'G':
        {
            // Force float
            val = QuantumValue(std::stod(line));
            break;
        }
        case 's':
        case 'c':
        {
            // Force string
            val = QuantumValue(line);
            break;
        }
        default:
        {
            // Auto-detect: try number first, fall back to string
            size_t idx = 0;
            double d = std::stod(line, &idx);
            val = (idx == line.size()) ? QuantumValue(d) : QuantumValue(line);
            break;
        }
        }
    }
    catch (...)
    {
        val = QuantumValue(line);
    }

    if (s.lvalueTarget)
    {
        // Complex lvalue: arr[i], obj.field, *ptr, etc.
        setLValue(*s.lvalueTarget, std::move(val), "=");
        return;
    }

    if (env->has(s.target))
        env->set(s.target, std::move(val));
    else
        env->define(s.target, std::move(val));
}

void Interpreter::execImport(ImportStmt &s)
{
    // ── Stub registry for common Python standard-library symbols ──────────────
    // Any symbol not listed here is silently injected as a no-op native stub.

    auto makeStubClass = [&](const std::string &name) -> QuantumValue
    {
        auto klass = std::make_shared<QuantumClass>();
        klass->name = name;
        return QuantumValue(klass);
    };

    auto makeStubNative = [&](const std::string &name) -> QuantumValue
    {
        auto fn = std::make_shared<QuantumNative>();
        fn->name = name;
        fn->fn = [](std::vector<QuantumValue> args) -> QuantumValue
        {
            return args.empty() ? QuantumValue() : args[0];
        };
        return QuantumValue(fn);
    };

    auto defGlobal = [&](const std::string &sym, QuantumValue val)
    {
        if (!globals->has(sym))
            globals->define(sym, std::move(val), false);
    };

    // Per-module registration helpers
    auto registerAbc = [&]()
    {
        defGlobal("ABC", makeStubClass("ABC"));
        defGlobal("abstractmethod", makeStubNative("abstractmethod"));
        defGlobal("ABCMeta", makeStubClass("ABCMeta"));
    };
    auto registerTyping = [&]()
    {
        for (auto &sym : {"List", "Dict", "Set", "Tuple", "Optional", "Union",
                          "Any", "Callable", "Type", "Iterable", "Iterator",
                          "Generator", "Sequence", "Mapping", "FrozenSet",
                          "ClassVar", "Final", "Literal", "TypeVar", "Generic",
                          "Protocol", "NamedTuple", "TypedDict", "overload",
                          "cast", "no_type_check", "get_type_hints"})
            defGlobal(sym, makeStubNative(sym));
    };
    auto registerCollections = [&]()
    {
        for (auto &sym : {"defaultdict", "OrderedDict", "Counter", "deque",
                          "namedtuple", "ChainMap"})
            defGlobal(sym, makeStubClass(sym));
    };
    auto registerDataclasses = [&]()
    {
        for (auto &sym : {"dataclass", "field", "fields", "asdict", "astuple",
                          "make_dataclass", "replace", "is_dataclass"})
            defGlobal(sym, makeStubNative(sym));
    };
    auto registerEnum = [&]()
    {
        for (auto &sym : {"Enum", "IntEnum", "Flag", "IntFlag", "auto", "unique"})
            defGlobal(sym, makeStubClass(sym));
    };
    auto registerFunctools = [&]()
    {
        for (auto &sym : {"reduce", "partial", "wraps", "lru_cache",
                          "cached_property", "total_ordering", "singledispatch"})
            defGlobal(sym, makeStubNative(sym));
    };
    auto registerItertools = [&]()
    {
        for (auto &sym : {"chain", "cycle", "repeat", "count", "accumulate",
                          "combinations", "permutations", "product", "groupby",
                          "islice", "starmap", "takewhile", "dropwhile", "zip_longest"})
            defGlobal(sym, makeStubNative(sym));
    };
    auto registerOs = [&]()
    {
        for (auto &sym : {"getcwd", "listdir", "path", "environ", "getenv",
                          "makedirs", "remove", "rename", "walk", "sep"})
            defGlobal(sym, makeStubNative(sym));
    };
    auto registerSys = [&]()
    {
        for (auto &sym : {"argv", "exit", "path", "version", "platform",
                          "stdin", "stdout", "stderr", "modules", "maxsize"})
            defGlobal(sym, makeStubNative(sym));
    };
    auto registerRe = [&]()
    {
        for (auto &sym : {"compile", "match", "search", "findall", "finditer",
                          "sub", "subn", "split", "fullmatch", "escape",
                          "IGNORECASE", "MULTILINE", "DOTALL", "VERBOSE"})
            defGlobal(sym, makeStubNative(sym));
    };
    auto registerJson = [&]()
    {
        for (auto &sym : {"dumps", "loads", "dump", "load",
                          "JSONDecodeError", "JSONEncoder", "JSONDecoder"})
            defGlobal(sym, makeStubNative(sym));
    };
    auto registerMath = [&]()
    {
        for (auto &sym : {"sqrt", "floor", "ceil", "log", "log2", "log10",
                          "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
                          "pow", "exp", "fabs", "factorial", "gcd", "lcm",
                          "pi", "e", "inf", "nan", "isnan", "isinf", "isfinite",
                          "degrees", "radians", "hypot", "trunc", "comb", "perm"})
            defGlobal(sym, makeStubNative(sym));
    };
    auto registerRandom = [&]()
    {
        for (auto &sym : {"random", "randint", "choice", "choices", "shuffle",
                          "sample", "uniform", "gauss", "seed", "randrange"})
            defGlobal(sym, makeStubNative(sym));
    };
    auto registerDatetime = [&]()
    {
        for (auto &sym : {"datetime", "date", "time", "timedelta", "timezone",
                          "MINYEAR", "MAXYEAR"})
            defGlobal(sym, makeStubClass(sym));
    };
    auto registerPathlib = [&]()
    {
        defGlobal("Path", makeStubClass("Path"));
        defGlobal("PurePath", makeStubClass("PurePath"));
    };
    auto registerIo = [&]()
    {
        for (auto &sym : {"StringIO", "BytesIO", "TextIOWrapper", "BufferedReader"})
            defGlobal(sym, makeStubClass(sym));
    };
    auto registerCopy = [&]()
    {
        defGlobal("copy", makeStubNative("copy"));
        defGlobal("deepcopy", makeStubNative("deepcopy"));
    };

    auto registerModule = [&](const std::string &name)
    {
        if (name == "abc")
            registerAbc();
        else if (name == "typing")
            registerTyping();
        else if (name == "collections")
            registerCollections();
        else if (name == "dataclasses")
            registerDataclasses();
        else if (name == "enum")
            registerEnum();
        else if (name == "functools")
            registerFunctools();
        else if (name == "itertools")
            registerItertools();
        else if (name == "os" || name == "os.path")
            registerOs();
        else if (name == "sys")
            registerSys();
        else if (name == "re")
            registerRe();
        else if (name == "json")
            registerJson();
        else if (name == "math")
            registerMath();
        else if (name == "random")
            registerRandom();
        else if (name == "datetime")
            registerDatetime();
        else if (name == "pathlib")
            registerPathlib();
        else if (name == "io")
            registerIo();
        else if (name == "copy")
            registerCopy();
        // Unknown module: silently ignored
    };

    if (!s.module.empty())
    {
        // "from X import a, b, c"
        registerModule(s.module);
        // Any symbol still undefined after module registration gets a generic stub
        for (auto &item : s.imports)
        {
            std::string sym = item.alias.empty() ? item.name : item.alias;
            if (!globals->has(sym) && !env->has(sym))
                globals->define(sym, makeStubNative(item.name), false);
        }
    }
    else
    {
        // "import X"  or  "import X as Y"
        for (auto &item : s.imports)
        {
            registerModule(item.name);
            std::string alias = item.alias.empty() ? item.name : item.alias;
            if (!globals->has(alias))
                globals->define(alias, makeStubNative(alias), false);
        }
    }
}

void Interpreter::execExprStmt(ExprStmt &s)
{
    evaluate(*s.expr);
}

// ─── Evaluate ────────────────────────────────────────────────────────────────

QuantumValue Interpreter::evaluate(ASTNode &node)
{
    return std::visit([&](auto &n) -> QuantumValue
                      {
        using T = std::decay_t<decltype(n)>;
        if constexpr (std::is_same_v<T, NumberLiteral>) return QuantumValue(n.value);
        else if constexpr (std::is_same_v<T, StringLiteral>) return QuantumValue(n.value);
        else if constexpr (std::is_same_v<T, BoolLiteral>)   return QuantumValue(n.value);
        else if constexpr (std::is_same_v<T, NilLiteral>)    return QuantumValue();
        else if constexpr (std::is_same_v<T, Identifier>)    return evalIdentifier(n);
        else if constexpr (std::is_same_v<T, BinaryExpr>)    return evalBinary(n);
        else if constexpr (std::is_same_v<T, UnaryExpr>)     return evalUnary(n);
        else if constexpr (std::is_same_v<T, AssignExpr>)    return evalAssign(n);
        else if constexpr (std::is_same_v<T, CallExpr>)      return evalCall(n);
        else if constexpr (std::is_same_v<T, IndexExpr>)     return evalIndex(n);
        else if constexpr (std::is_same_v<T, SliceExpr>)
        {
            auto obj = evaluate(*n.object);
            // Resolve start / stop / step with Python semantics
            auto applySlice = [&](int len) -> std::tuple<int,int,int>
            {
                int step  = n.step  ? (int)toNum(evaluate(*n.step),  "slice step")  : 1;
                int start, stop;
                if (!n.start) start = (step > 0) ? 0 : len - 1;
                else {
                    start = (int)toNum(evaluate(*n.start), "slice start");
                    if (start < 0) start += len;
                    if (start < 0) start = 0;
                    if (start > len) start = len;
                }
                if (!n.stop) stop = (step > 0) ? len : -1;
                else {
                    stop = (int)toNum(evaluate(*n.stop), "slice stop");
                    if (stop < 0) stop += len;
                    if (step > 0) { if (stop < 0) stop = 0; if (stop > len) stop = len; }
                    else          { if (stop < -1) stop = -1; }
                }
                return {start, stop, step};
            };

            if (obj.isString())
            {
                const auto &s = obj.asString();
                int len = (int)s.size();
                auto [start, stop, step] = applySlice(len);
                std::string result;
                if (step > 0) { for (int i = start; i < stop;  i += step) if (i >= 0 && i < len) result += s[i]; }
                else          { for (int i = start; i > stop;  i += step) if (i >= 0 && i < len) result += s[i]; }
                return QuantumValue(result);
            }
            if (obj.isArray())
            {
                auto &arr = *obj.asArray();
                int len = (int)arr.size();
                auto [start, stop, step] = applySlice(len);
                auto result = std::make_shared<Array>();
                if (step > 0) { for (int i = start; i < stop;  i += step) if (i >= 0 && i < len) result->push_back(arr[i]); }
                else          { for (int i = start; i > stop;  i += step) if (i >= 0 && i < len) result->push_back(arr[i]); }
                return QuantumValue(result);
            }
            throw TypeError("Cannot slice " + obj.typeName());
        }
        else if constexpr (std::is_same_v<T, MemberExpr>)    return evalMember(n);
        else if constexpr (std::is_same_v<T, ArrayLiteral>)  return evalArray(n);
        else if constexpr (std::is_same_v<T, DictLiteral>)   return evalDict(n);
        else if constexpr (std::is_same_v<T, LambdaExpr>)    return evalLambda(n);
        else if constexpr (std::is_same_v<T, ListComp>)      return evalListComp(n);
        else if constexpr (std::is_same_v<T, TupleLiteral>)
        {
            // Tuples are arrays at runtime
            auto arr = std::make_shared<Array>();
            for (auto &el : n.elements)
                arr->push_back(evaluate(*el));
            return QuantumValue(arr);
        }
        else if constexpr (std::is_same_v<T, TernaryExpr>)   {
            return evaluate(*n.condition).isTruthy()
                ? evaluate(*n.thenExpr)
                : evaluate(*n.elseExpr);
        }
        else if constexpr (std::is_same_v<T, SuperExpr>) {
            // SuperExpr is handled directly in evalCall for super() and super.method()
            // If we get here, it's a bare super reference
            throw RuntimeError("Cannot use 'super' outside of a method call");
        }
        else if constexpr (std::is_same_v<T, AddressOfExpr>) return evalAddressOf(n);
        else if constexpr (std::is_same_v<T, DerefExpr>)     return evalDeref(n);
        else if constexpr (std::is_same_v<T, ArrowExpr>)     return evalArrow(n);
        else if constexpr (std::is_same_v<T, NewExpr>)       return evalNewExpr(n);
        else {
            execute(node);
            return QuantumValue();
        } }, node.node);
}

QuantumValue Interpreter::evalIdentifier(Identifier &e)
{
    // Try regular env lookup first
    try
    {
        return env->get(e.name);
    }
    catch (NameError &)
    {
        // Fall back: if "self" is in scope and has this field or method, return it
        try
        {
            auto selfVal = env->get("self");
            if (selfVal.isInstance())
            {
                auto inst = selfVal.asInstance();
                // Check instance fields
                auto fit = inst->fields.find(e.name);
                if (fit != inst->fields.end())
                    return fit->second;
                // Check class methods (for C++ style bare method calls like defeatVillain())
                auto k = inst->klass.get();
                while (k)
                {
                    auto mit = k->methods.find(e.name);
                    if (mit != k->methods.end())
                    {
                        // Return as a bound-method-like function
                        // We wrap it so callFunction works, binding self
                        return QuantumValue(mit->second);
                    }
                    k = k->base.get();
                }
            }
        }
        catch (...)
        {
        }
        throw; // re-throw original NameError
    }
}

QuantumValue Interpreter::evalBinary(BinaryExpr &e)
{
    // Short-circuit for and/or
    if (e.op == "and")
    {
        auto lv = evaluate(*e.left);
        if (!lv.isTruthy())
            return lv;
        return evaluate(*e.right);
    }
    if (e.op == "or")
    {
        auto lv = evaluate(*e.left);
        if (lv.isTruthy())
            return lv;
        return evaluate(*e.right);
    }

    auto lv = evaluate(*e.left);
    auto rv = evaluate(*e.right);
    const std::string &op = e.op;

    // ── Pointer arithmetic (must be checked before generic +/-) ─────────────
    if (lv.isPointer() && rv.isNumber())
    {
        auto ptr = lv.asPointer();
        auto newPtr = std::make_shared<QuantumPointer>(*ptr);
        if (op == "+")
            newPtr->offset += (int)rv.asNumber();
        else if (op == "-")
            newPtr->offset -= (int)rv.asNumber();
        else
            throw RuntimeError("Unsupported pointer operator: " + op);
        return QuantumValue(newPtr);
    }
    if (rv.isPointer() && lv.isNumber() && op == "+")
    {
        auto ptr = rv.asPointer();
        auto newPtr = std::make_shared<QuantumPointer>(*ptr);
        newPtr->offset += (int)lv.asNumber();
        return QuantumValue(newPtr);
    }
    // Pointer difference
    if (lv.isPointer() && rv.isPointer() && op == "-")
        return QuantumValue((double)(lv.asPointer()->offset - rv.asPointer()->offset));
    // Pointer comparison (ptr == nullptr, ptr == ptr, ptr != nullptr)
    if (lv.isPointer() || rv.isPointer())
    {
        if (op == "==" || op == "!=")
        {
            bool eq = false;
            if (lv.isPointer() && rv.isNil())
                eq = lv.asPointer()->isNull();
            else if (rv.isPointer() && lv.isNil())
                eq = rv.asPointer()->isNull();
            else if (lv.isPointer() && rv.isPointer())
                eq = (lv.asPointer()->cell == rv.asPointer()->cell &&
                      lv.asPointer()->offset == rv.asPointer()->offset);
            return QuantumValue(op == "==" ? eq : !eq);
        }
    }

    if (op == "+")
    {
        if (lv.isString() || rv.isString())
            return QuantumValue(lv.toString() + rv.toString());
        if (lv.isNumber() && rv.isNumber())
            return QuantumValue(lv.asNumber() + rv.asNumber());
        if (lv.isArray() && rv.isArray())
        {
            auto arr = std::make_shared<Array>(*lv.asArray());
            for (auto &x : *rv.asArray())
                arr->push_back(x);
            return QuantumValue(arr);
        }
        throw TypeError("Cannot add " + lv.typeName() + " and " + rv.typeName());
    }
    if (op == "-")
        return QuantumValue(toNum(lv, "-") - toNum(rv, "-"));
    if (op == "*")
    {
        if (lv.isNumber() && rv.isNumber())
            return QuantumValue(lv.asNumber() * rv.asNumber());
        if (lv.isString() && rv.isNumber())
        {
            std::string s;
            int n = (int)rv.asNumber();
            for (int i = 0; i < n; i++)
                s += lv.asString();
            return QuantumValue(s);
        }
        throw TypeError("Cannot multiply " + lv.typeName() + " and " + rv.typeName());
    }
    if (op == "/")
    {
        double d = toNum(rv, "/");
        if (d == 0)
            throw RuntimeError("Division by zero");
        return QuantumValue(toNum(lv, "/") / d);
    }
    if (op == "//")
    {
        double d = toNum(rv, "//");
        if (d == 0)
            throw RuntimeError("Division by zero");
        return QuantumValue(std::floor(toNum(lv, "//") / d));
    }
    if (op == "%")
    {
        long long b = toInt(rv, "%");
        if (b == 0)
            throw RuntimeError("Modulo by zero");
        return QuantumValue((double)(toInt(lv, "%") % b));
    }
    if (op == "**")
        return QuantumValue(std::pow(toNum(lv, "**"), toNum(rv, "**")));
    if (op == "==")
    {
        if (lv.isNil() && rv.isNil())
            return QuantumValue(true);
        if (lv.typeName() != rv.typeName())
            return QuantumValue(false);
        if (lv.isNumber())
            return QuantumValue(lv.asNumber() == rv.asNumber());
        if (lv.isBool())
            return QuantumValue(lv.asBool() == rv.asBool());
        if (lv.isString())
            return QuantumValue(lv.asString() == rv.asString());
        return QuantumValue(false);
    }
    if (op == "!=")
    {
        // inline evaluate
        bool eq_res = false;
        if (lv.isNil() && rv.isNil())
            eq_res = true;
        else if (lv.typeName() == rv.typeName())
        {
            if (lv.isNumber())
                eq_res = lv.asNumber() == rv.asNumber();
            else if (lv.isBool())
                eq_res = lv.asBool() == rv.asBool();
            else if (lv.isString())
                eq_res = lv.asString() == rv.asString();
        }
        return QuantumValue(!eq_res);
    }
    // Numeric comparisons — coerce bool to number so chained comparisons
    // like (0 <= start) < n work correctly (True/False become 1/0).
    auto toNumOrBool = [](const QuantumValue &v, const std::string &ctx) -> double
    {
        if (v.isNumber())
            return v.asNumber();
        if (v.isBool())
            return v.asBool() ? 1.0 : 0.0;
        throw TypeError("Expected number in " + ctx + ", got " + v.typeName());
    };
    if (op == "<")
        return QuantumValue(toNumOrBool(lv, "<") < toNumOrBool(rv, "<"));
    if (op == ">")
        return QuantumValue(toNumOrBool(lv, ">") > toNumOrBool(rv, ">"));
    if (op == "<=")
        return QuantumValue(toNumOrBool(lv, "<=") <= toNumOrBool(rv, "<="));
    if (op == ">=")
        return QuantumValue(toNumOrBool(lv, ">=") >= toNumOrBool(rv, ">="));

    // Membership: x in collection,  x not in collection
    if (op == "in" || op == "not in")
    {
        bool found = false;
        if (rv.isArray())
        {
            for (auto &elem : *rv.asArray())
            {
                if (elem.isNumber() && lv.isNumber() && elem.asNumber() == lv.asNumber())
                {
                    found = true;
                    break;
                }
                if (elem.isString() && lv.isString() && elem.asString() == lv.asString())
                {
                    found = true;
                    break;
                }
                if (elem.isBool() && lv.isBool() && elem.asBool() == lv.asBool())
                {
                    found = true;
                    break;
                }
                if (elem.isNil() && lv.isNil())
                {
                    found = true;
                    break;
                }
            }
        }
        else if (rv.isString())
            found = rv.asString().find(lv.toString()) != std::string::npos;
        else if (rv.isDict())
            found = rv.asDict()->count(lv.toString()) > 0;
        return QuantumValue(op == "in" ? found : !found);
    }

    // Bitwise
    if (op == "&")
        return QuantumValue((double)(toInt(lv, "&") & toInt(rv, "&")));
    if (op == "|")
        return QuantumValue((double)(toInt(lv, "|") | toInt(rv, "|")));
    if (op == "^")
        return QuantumValue((double)(toInt(lv, "^") ^ toInt(rv, "^")));
    if (op == "<<")
        return QuantumValue((double)(toInt(lv, "<<") << toInt(rv, "<<")));
    if (op == ">>")
        return QuantumValue((double)(toInt(lv, ">>") >> toInt(rv, ">>")));

    throw RuntimeError("Unknown operator: " + op);
}

QuantumValue Interpreter::evalUnary(UnaryExpr &e)
{
    auto v = evaluate(*e.operand);
    if (e.op == "-")
        return QuantumValue(-toNum(v, "unary -"));
    if (e.op == "not")
        return QuantumValue(!v.isTruthy());
    if (e.op == "~")
        return QuantumValue((double)~toInt(v, "~"));
    // Pointer-aware increment/decrement (operand already evaluated above as 'v')
    // The orignal parsePostfix already converts p++ into AssignExpr(+=, p, 1)
    // so we only need to handle pointer arithmetic for any raw unary ++ that might arrive
    throw RuntimeError("Unknown unary op: " + e.op);
}

void Interpreter::setLValue(ASTNode &target, QuantumValue val, const std::string &op)
{
    auto applyOp = [&](QuantumValue old) -> QuantumValue
    {
        if (op == "=")
            return val;
        if (op == "+=")
        {
            if (old.isString())
                return QuantumValue(old.asString() + val.toString());
            return QuantumValue(toNum(old, op) + toNum(val, op));
        }
        if (op == "-=")
            return QuantumValue(toNum(old, op) - toNum(val, op));
        if (op == "*=")
            return QuantumValue(toNum(old, op) * toNum(val, op));
        if (op == "/=")
        {
            double d = toNum(val, op);
            if (!d)
                throw RuntimeError("Div by 0");
            return QuantumValue(toNum(old, op) / d);
        }
        return val;
    };

    if (target.is<Identifier>())
    {
        auto &name = target.as<Identifier>().name;
        if (op == "=")
        {
            // If assigning a plain value to an existing pointer variable (e.g. c = new int(300)),
            // wrap the value in a new heap cell so the pointer stays valid.
            if (env->has(name) && !val.isPointer() && !val.isNil())
            {
                try
                {
                    auto existing = env->get(name);
                    if (existing.isPointer())
                    {
                        auto cell = std::make_shared<QuantumValue>(val);
                        auto ptr = std::make_shared<QuantumPointer>();
                        ptr->cell = cell;
                        ptr->varName = name;
                        val = QuantumValue(ptr);
                    }
                }
                catch (...)
                {
                }
            }
            // Python-style: if variable doesn't exist anywhere, define it in current scope
            // But if we're inside a method (self exists) and variable not yet in any scope,
            // store as instance field (C++ style: heroName = value is equivalent to self.heroName = value)
            if (!env->has(name))
            {
                // Check if self exists — if so, store as instance field (C++ method style)
                try
                {
                    auto selfVal = env->get("self");
                    if (selfVal.isInstance())
                    {
                        selfVal.asInstance()->fields[name] = std::move(val);
                        return;
                    }
                }
                catch (...)
                {
                }
                env->define(name, std::move(val));
            }
            else
                env->set(name, std::move(val));
        }
        else
        {
            // Compound assignment: try env first, then self fields
            try
            {
                env->set(name, applyOp(env->get(name)));
            }
            catch (NameError &)
            {
                try
                {
                    auto selfVal = env->get("self");
                    if (selfVal.isInstance())
                    {
                        auto inst = selfVal.asInstance();
                        auto it = inst->fields.find(name);
                        QuantumValue old = (it != inst->fields.end()) ? it->second : QuantumValue();
                        inst->fields[name] = applyOp(old);
                        return;
                    }
                }
                catch (...)
                {
                }
                throw;
            }
        }
    }
    else if (target.is<IndexExpr>())
    {
        auto &ie = target.as<IndexExpr>();
        auto obj = evaluate(*ie.object);
        auto idx = evaluate(*ie.index);
        // Unwrap pointer-to-array: float* arr = new float[n]; arr[i] = v
        if (obj.isPointer())
        {
            auto ptr = obj.asPointer();
            if (!ptr->isNull() && ptr->cell->isArray())
                obj = *ptr->cell;
        }
        if (obj.isArray())
        {
            int i = (int)toNum(idx, "index");
            auto arr = obj.asArray();
            if (i < 0)
                i += arr->size();
            if (i < 0 || i >= (int)arr->size())
                throw IndexError("Array index out of range");
            (*arr)[i] = applyOp((*arr)[i]);
        }
        else if (obj.isDict())
        {
            std::string key = idx.toString();
            auto dict = obj.asDict();
            (*dict)[key] = applyOp(dict->count(key) ? (*dict)[key] : QuantumValue());
        }
    }
    else if (target.is<MemberExpr>())
    {
        auto &me = target.as<MemberExpr>();
        auto obj = evaluate(*me.object);
        if (obj.isInstance())
        {
            auto inst = obj.asInstance();
            auto cur = inst->fields.count(me.member) ? inst->fields[me.member] : QuantumValue();
            inst->setField(me.member, applyOp(cur));
        }
        else if (obj.isDict())
        {
            (*obj.asDict())[me.member] = applyOp(obj.asDict()->count(me.member) ? (*obj.asDict())[me.member] : QuantumValue());
        }
    }
    else if (target.is<DerefExpr>())
    {
        // *ptr = value  /  *(ptr + i) = value
        auto &de = target.as<DerefExpr>();
        auto ptrVal = evaluate(*de.operand);
        if (!ptrVal.isPointer())
            throw RuntimeError("Cannot dereference non-pointer");
        auto ptr = ptrVal.asPointer();
        if (ptr->isNull())
            throw RuntimeError("Null pointer dereference");
        // Array-pointer with offset: *(ptr + i) = value
        // Unwrap one level if cell itself holds a pointer (double-indirection)
        auto cell = ptr->cell;
        while (cell->isPointer())
            cell = cell->asPointer()->cell;
        if (cell->isArray())
        {
            auto arr = cell->asArray();
            int i = ptr->offset;
            if (i < 0)
                i += (int)arr->size();
            if (i < 0 || i >= (int)arr->size())
                throw IndexError("Pointer write out of bounds (offset " + std::to_string(ptr->offset) + ")");
            (*arr)[i] = applyOp((*arr)[i]);
        }
        else
        {
            *cell = applyOp(*cell);
        }
    }
    else if (target.is<ArrowExpr>())
    {
        // ptr->member = value
        auto &ae = target.as<ArrowExpr>();
        auto ptrVal = evaluate(*ae.object);
        if (!ptrVal.isPointer())
            throw RuntimeError("Arrow operator requires pointer");
        auto ptr = ptrVal.asPointer();
        if (ptr->isNull())
            throw RuntimeError("Null pointer dereference via ->");
        auto &cell = *ptr->cell;
        if (cell.isInstance())
        {
            auto inst = cell.asInstance();
            auto cur = inst->fields.count(ae.member) ? inst->fields[ae.member] : QuantumValue();
            inst->setField(ae.member, applyOp(cur));
        }
        else if (cell.isDict())
        {
            (*cell.asDict())[ae.member] = applyOp(cell.asDict()->count(ae.member) ? (*cell.asDict())[ae.member] : QuantumValue());
        }
        else
            throw RuntimeError("Cannot use -> on non-struct type");
    }
}

QuantumValue Interpreter::evalAssign(AssignExpr &e)
{
    // Tuple unpacking: a, b, c = someIterable
    if (e.op == "unpack")
    {
        auto val = evaluate(*e.value);
        auto &lhs = e.target->as<TupleLiteral>();
        // val should be array/tuple
        std::shared_ptr<Array> arr;
        if (val.isArray())
            arr = val.asArray();
        else
        {
            // wrap scalar in single-element array
            arr = std::make_shared<Array>();
            arr->push_back(val);
        }
        for (size_t i = 0; i < lhs.elements.size(); ++i)
        {
            QuantumValue item = (i < arr->size()) ? (*arr)[i] : QuantumValue();
            auto &target = *lhs.elements[i];
            if (target.is<Identifier>())
            {
                auto &name = target.as<Identifier>().name;
                if (!env->has(name))
                    env->define(name, item);
                else
                    env->set(name, item);
            }
        }
        return val;
    }
    auto val = evaluate(*e.value);
    setLValue(*e.target, val, e.op);
    return val;
}

QuantumValue Interpreter::evalCall(CallExpr &e)
{
    // Special: method call via MemberExpr callee
    if (e.callee->is<MemberExpr>())
    {
        auto &me = e.callee->as<MemberExpr>();
        auto obj = evaluate(*me.object);
        std::vector<QuantumValue> args;
        for (auto &a : e.args)
            args.push_back(evaluate(*a));
        return callMethod(obj, me.member, std::move(args));
    }

    // Super constructor call: super(args)
    if (e.callee->is<SuperExpr>())
    {
        auto &se = e.callee->as<SuperExpr>();
        std::vector<QuantumValue> args;
        for (auto &a : e.args)
            args.push_back(evaluate(*a));

        if (!se.method.empty())
        {
            // super.method(args) — find method in parent class and call with current self
            auto selfVal = env->get("self");
            auto inst = selfVal.asInstance();
            auto parentClass = inst->klass->base;
            if (!parentClass)
                throw RuntimeError("No parent class for super call");
            // Walk parent chain to find the method
            auto k = parentClass.get();
            while (k)
            {
                auto it = k->methods.find(se.method);
                if (it != k->methods.end())
                    return callInstanceMethod(inst, it->second, std::move(args));
                k = k->base.get();
            }
            throw RuntimeError("Method '" + se.method + "' not found in parent class");
        }
        else
        {
            // super(args) — call parent constructor
            auto selfVal = env->get("self");
            auto inst = selfVal.asInstance();
            auto parentClass = inst->klass->base;
            if (!parentClass)
                throw RuntimeError("No parent class for super() call");
            // Find init in parent chain
            auto k = parentClass.get();
            while (k)
            {
                auto it = k->methods.find("init");
                if (it != k->methods.end())
                    return callInstanceMethod(inst, it->second, std::move(args));
                k = k->base.get();
            }
            return QuantumValue(); // no parent init
        }
    }

    auto callee = evaluate(*e.callee);
    std::vector<QuantumValue> args;

    // For QuantumFunctions with reference params, pass live cells for ref args
    // so the callee can write back through them
    if (callee.isFunction() && std::holds_alternative<std::shared_ptr<QuantumFunction>>(callee.data))
    {
        auto fn = callee.asFunction();
        for (size_t i = 0; i < e.args.size(); i++)
        {
            bool isRef = (i < fn->paramIsRef.size()) && fn->paramIsRef[i];
            if (isRef && e.args[i]->is<Identifier>())
            {
                // Pass by reference: wrap in a pointer to the caller's live cell
                const std::string &varName = e.args[i]->as<Identifier>().name;
                auto cell = env->getCell(varName);
                if (cell)
                {
                    auto ptr = std::make_shared<QuantumPointer>();
                    ptr->cell = cell;
                    ptr->varName = varName;
                    args.push_back(QuantumValue(ptr));
                    continue;
                }
            }
            args.push_back(evaluate(*e.args[i]));
        }
    }
    else
    {
        for (auto &a : e.args)
            args.push_back(evaluate(*a));
    }

    // Class construction: ClassName(args)
    if (callee.isClass())
    {
        auto klass = callee.asClass();
        auto inst = std::make_shared<QuantumInstance>();
        inst->klass = klass;
        inst->env = std::make_shared<Environment>(env);
        QuantumValue instVal(inst);

        // Find init in class hierarchy — prefer exact arg-count match for overloading
        auto k = klass.get();
        std::shared_ptr<QuantumFunction> initFn;
        std::string overloadKey = "init#" + std::to_string(args.size());
        while (k && !initFn)
        {
            // Try exact overload match first
            auto oit = k->methods.find(overloadKey);
            if (oit != k->methods.end())
            {
                initFn = oit->second;
                break;
            }
            // Fall back to generic init
            auto it = k->methods.find("init");
            if (it != k->methods.end())
                initFn = it->second;
            k = k->base.get();
        }

        if (initFn)
        {
            auto scope = std::make_shared<Environment>(initFn->closure);
            scope->define("self", instVal);
            scope->define("this", instVal);
            // Bind params (skip leading "self"/"this" if present)
            size_t paramStart = (!initFn->params.empty() && (initFn->params[0] == "self" || initFn->params[0] == "this")) ? 1 : 0;
            for (size_t i = paramStart; i < initFn->params.size(); i++)
            {
                size_t ai = i - paramStart;
                scope->define(initFn->params[i], ai < args.size() ? args[ai] : QuantumValue());
            }
            try
            {
                execBlock(initFn->body->as<BlockStmt>(), scope);
            }
            catch (ReturnSignal &)
            {
            }
        }
        return instVal;
    }

    if (callee.isFunction())
    {
        if (std::holds_alternative<std::shared_ptr<QuantumFunction>>(callee.data))
        {
            // C++ style: if we're inside a method and called a bare function name,
            // and that function is a class method (not in regular env), call it as instance method
            if (e.callee->is<Identifier>())
            {
                const std::string &fname = e.callee->as<Identifier>().name;
                try
                {
                    env->get(fname); // if this succeeds, it's a regular function — call normally
                    return callFunction(callee.asFunction(), std::move(args));
                }
                catch (NameError &)
                {
                    // It came from self — call as instance method
                    try
                    {
                        auto selfVal = env->get("self");
                        if (selfVal.isInstance())
                            return callInstanceMethod(selfVal.asInstance(), callee.asFunction(), std::move(args));
                    }
                    catch (...)
                    {
                    }
                    return callFunction(callee.asFunction(), std::move(args));
                }
            }
            return callFunction(callee.asFunction(), std::move(args));
        }
        if (std::holds_alternative<std::shared_ptr<QuantumNative>>(callee.data))
            return callNative(callee.asNative(), std::move(args));
    }
    throw TypeError("Cannot call " + callee.typeName());
}

QuantumValue Interpreter::callFunction(std::shared_ptr<QuantumFunction> fn, std::vector<QuantumValue> args)
{
    auto scope = std::make_shared<Environment>(fn->closure);
    for (size_t i = 0; i < fn->params.size(); i++)
    {
        bool isRef = (i < fn->paramIsRef.size()) && fn->paramIsRef[i];
        QuantumValue v = i < args.size() ? args[i] : QuantumValue();

        if (isRef)
        {
            // Pass-by-reference: the arg must be a QuantumPointer (from &var or the var's cell).
            // If the caller already passed a pointer, use its cell directly in scope.
            // If the caller passed a plain value (e.g. cubeByRef(b) without explicit &),
            // we need the caller's env to have a live cell for 'b'. We wrap the value
            // in a new shared cell and bind it — then sync back after the call.
            if (v.isPointer())
            {
                // Already a pointer — bind param name to a local that shares the cell
                auto ptr = v.asPointer();
                if (!ptr->isNull())
                {
                    scope->defineRef(fn->params[i], ptr->cell);
                    continue;
                }
            }
            // Plain value passed for a ref param: create a cell, bind it, sync after call
            auto cell = std::make_shared<QuantumValue>(v);
            scope->defineRef(fn->params[i], cell);
            // After the call we'll write *cell back to the caller arg — but we can't
            // reach caller's env here. The cell IS shared though, so if the caller
            // passed via getCell the sync is automatic.
            continue;
        }

        scope->define(fn->params[i], std::move(v));
    }
    QuantumValue result;
    try
    {
        execBlock(fn->body->as<BlockStmt>(), scope);
    }
    catch (ReturnSignal &r)
    {
        result = std::move(r.value);
    }

    // C++ destructor simulation: call __del__ on any instances defined in this scope
    // that have a __del__ method (e.g. objects going out of scope)
    for (auto &[varName, val] : scope->getVars())
    {
        if (val.isInstance())
        {
            auto inst = val.asInstance();
            auto k = inst->klass.get();
            while (k)
            {
                auto it = k->methods.find("__del__");
                if (it != k->methods.end())
                {
                    try
                    {
                        callInstanceMethod(inst, it->second, {});
                    }
                    catch (...)
                    {
                    }
                    break;
                }
                k = k->base.get();
            }
        }
    }

    return result;
}

QuantumValue Interpreter::callNative(std::shared_ptr<QuantumNative> fn, std::vector<QuantumValue> args)
{
    return fn->fn(std::move(args));
}

QuantumValue Interpreter::callInstanceMethod(std::shared_ptr<QuantumInstance> inst, std::shared_ptr<QuantumFunction> fn, std::vector<QuantumValue> args)
{
    QuantumValue instVal(inst);
    auto scope = std::make_shared<Environment>(fn->closure);
    scope->define("self", instVal);
    scope->define("this", instVal);
    size_t paramStart = (!fn->params.empty() && (fn->params[0] == "self" || fn->params[0] == "this")) ? 1 : 0;
    for (size_t i = paramStart; i < fn->params.size(); i++)
    {
        size_t ai = i - paramStart;
        scope->define(fn->params[i], ai < args.size() ? args[ai] : QuantumValue());
    }
    try
    {
        execBlock(fn->body->as<BlockStmt>(), scope);
    }
    catch (ReturnSignal &r)
    {
        return r.value;
    }
    return QuantumValue();
}

QuantumValue Interpreter::evalIndex(IndexExpr &e)
{
    auto obj = evaluate(*e.object);
    auto idx = evaluate(*e.index);

    // Pointer-to-array indexing: float* arr = new float[n];  arr[i]
    if (obj.isPointer())
    {
        auto ptr = obj.asPointer();
        if (!ptr->isNull() && ptr->cell->isArray())
        {
            obj = *ptr->cell; // unwrap to the array
        }
        else
        {
            // Pointer arithmetic: ptr[0] == *ptr
            int i = idx.isNumber() ? (int)idx.asNumber() : 0;
            if (i == 0 && !ptr->isNull())
                return *ptr->cell;
            throw TypeError("Cannot index pointer with offset " + std::to_string(i));
        }
    }

    if (obj.isArray())
    {
        int i;
        if (idx.isNumber())
            i = (int)idx.asNumber();
        else if (idx.isString())
        {
            try
            {
                i = std::stoi(idx.asString());
            }
            catch (...)
            {
                throw TypeError("Expected number in index, got " + idx.typeName());
            }
        }
        else
            throw TypeError("Expected number in index, got " + idx.typeName());
        auto arr = obj.asArray();
        if (i < 0)
            i += arr->size();
        if (i < 0 || i >= (int)arr->size())
            throw IndexError("Array index " + std::to_string(i) + " out of range");
        return (*arr)[i];
    }
    if (obj.isDict())
    {
        auto dict = obj.asDict();
        auto key = idx.toString();
        auto it = dict->find(key);
        if (it == dict->end())
            return QuantumValue();
        return it->second;
    }
    if (obj.isString())
    {
        int i = (int)toNum(idx, "index");
        const auto &s = obj.asString();
        if (i < 0)
            i += s.size();
        if (i < 0 || i >= (int)s.size())
            throw IndexError("String index out of range");
        return QuantumValue(std::string(1, s[i]));
    }
    throw TypeError("Cannot index " + obj.typeName());
}

QuantumValue Interpreter::evalMember(MemberExpr &e)
{
    auto obj = evaluate(*e.object);
    if (obj.isInstance())
    {
        auto inst = obj.asInstance();
        // Check for __str__ when toString is called
        try
        {
            return inst->getField(e.member);
        }
        catch (NameError &)
        {
            throw TypeError("No member '" + e.member + "' on instance of " + inst->klass->name);
        }
    }
    if (obj.isClass())
    {
        // Static method/field access: ClassName.method or ClassName.field
        auto klass = obj.asClass();
        auto it = klass->staticMethods.find(e.member);
        if (it != klass->staticMethods.end())
            return QuantumValue(it->second);
        auto fit = klass->staticFields.find(e.member);
        if (fit != klass->staticFields.end())
            return fit->second;
        throw TypeError("No static member '" + e.member + "' on class " + klass->name);
    }
    if (obj.isDict())
    {
        auto dict = obj.asDict();
        auto it = dict->find(e.member);
        return it != dict->end() ? it->second : QuantumValue();
    }
    // String len property
    if (obj.isString() && e.member == "length")
        return QuantumValue((double)obj.asString().size());
    if (obj.isArray() && e.member == "length")
        return QuantumValue((double)obj.asArray()->size());
    throw TypeError("No member '" + e.member + "' on " + obj.typeName());
}

QuantumValue Interpreter::evalArray(ArrayLiteral &e)
{
    auto arr = std::make_shared<Array>();
    for (auto &el : e.elements)
        arr->push_back(evaluate(*el));
    return QuantumValue(arr);
}

QuantumValue Interpreter::evalDict(DictLiteral &e)
{
    auto dict = std::make_shared<Dict>();
    for (auto &[k, v] : e.pairs)
    {
        auto key = evaluate(*k);
        auto val = evaluate(*v);
        (*dict)[key.toString()] = std::move(val);
    }
    return QuantumValue(dict);
}

QuantumValue Interpreter::evalListComp(ListComp &e)
{
    auto result = std::make_shared<Array>();
    auto iter = evaluate(*e.iterable);
    bool hasTuple = e.vars.size() >= 2;

    auto processItem = [&](QuantumValue item)
    {
        auto scope = std::make_shared<Environment>(env);
        if (hasTuple)
        {
            if (item.isArray() && item.asArray()->size() >= 2)
            {
                scope->define(e.vars[0], (*item.asArray())[0]);
                scope->define(e.vars[1], (*item.asArray())[1]);
            }
            else
            {
                scope->define(e.vars[0], item);
                if (e.vars.size() > 1)
                    scope->define(e.vars[1], QuantumValue());
            }
        }
        else
            scope->define(e.vars[0], item);

        // Evaluate optional filter condition
        if (e.condition)
        {
            auto saved = env;
            env = scope;
            bool pass = evaluate(*e.condition).isTruthy();
            env = saved;
            if (!pass)
                return;
        }

        // Evaluate the expression in the loop scope
        auto saved = env;
        env = scope;
        auto val = evaluate(*e.expr);
        env = saved;
        result->push_back(std::move(val));
    };

    if (iter.isArray())
        for (auto &item : *iter.asArray())
            processItem(item);
    else if (iter.isString())
        for (char c : iter.asString())
            processItem(QuantumValue(std::string(1, c)));
    else if (iter.isDict())
        for (auto &[k, v] : *iter.asDict())
        {
            auto pair = std::make_shared<Array>();
            pair->push_back(QuantumValue(k));
            pair->push_back(v);
            processItem(QuantumValue(pair));
        }

    return QuantumValue(result);
}

QuantumValue Interpreter::evalLambda(LambdaExpr &e)
{
    auto fn = std::make_shared<QuantumFunction>();
    fn->name = "<lambda>";
    fn->params = e.params;
    fn->body = e.body.get();
    fn->closure = env;
    return QuantumValue(fn);
}

// ─── C++ Pointer Operators ────────────────────────────────────────────────────

QuantumValue Interpreter::evalAddressOf(AddressOfExpr &e)
{
    // &var — returns a live shared pointer to the variable's cell
    if (!e.operand->is<Identifier>())
        throw RuntimeError("Address-of operator requires an lvalue variable");
    const std::string &name = e.operand->as<Identifier>().name;

    // Ensure variable exists
    if (!env->has(name))
        env->define(name, QuantumValue());

    auto cell = env->getCell(name);
    if (!cell)
        throw RuntimeError("Could not get address of '" + name + "'");

    auto ptr = std::make_shared<QuantumPointer>();
    ptr->cell = cell;
    ptr->varName = name;
    ptr->offset = 0;
    return QuantumValue(ptr);
}

QuantumValue Interpreter::evalDeref(DerefExpr &e)
{
    // *ptr — read the value the pointer points to
    auto ptrVal = evaluate(*e.operand);
    if (!ptrVal.isPointer())
        throw RuntimeError("Cannot dereference non-pointer (type: " + ptrVal.typeName() + ")");
    auto ptr = ptrVal.asPointer();
    if (ptr->isNull())
        throw RuntimeError("Null pointer dereference");
    // Unwrap one level if cell itself holds a pointer (double-indirection)
    auto cell = ptr->cell;
    while (cell->isPointer())
        cell = cell->asPointer()->cell;
    // If the cell holds an array, index into it using offset: *(ptr + i)
    if (cell->isArray())
    {
        auto arr = cell->asArray();
        int i = ptr->offset;
        if (i < 0)
            i += (int)arr->size();
        if (i < 0 || i >= (int)arr->size())
            throw IndexError("Pointer dereference out of bounds (offset " + std::to_string(ptr->offset) + ")");
        return (*arr)[i];
    }
    return *cell;
}

QuantumValue Interpreter::evalArrow(ArrowExpr &e)
{
    // ptr->member — dereference pointer then access member
    auto ptrVal = evaluate(*e.object);
    if (!ptrVal.isPointer())
        throw RuntimeError("Arrow operator requires a pointer (type: " + ptrVal.typeName() + ")");
    auto ptr = ptrVal.asPointer();
    if (ptr->isNull())
        throw RuntimeError("Null pointer dereference via ->");
    auto &cell = *ptr->cell;
    if (cell.isInstance())
    {
        auto inst = cell.asInstance();
        try
        {
            return inst->getField(e.member);
        }
        catch (...)
        {
            throw TypeError("No member '" + e.member + "' on pointed-to instance");
        }
    }
    if (cell.isDict())
    {
        auto dict = cell.asDict();
        auto it = dict->find(e.member);
        return it != dict->end() ? it->second : QuantumValue();
    }
    throw RuntimeError("Cannot use -> on type " + cell.typeName());
}

QuantumValue Interpreter::evalNewExpr(NewExpr &e)
{
    static const std::unordered_set<std::string> primitives = {
        "int", "long", "short", "unsigned", "float", "double", "char", "bool"};

    // ── new T[n] — allocate array of n zero-initialised elements ─────────────
    if (e.isArray)
    {
        int n = 0;
        if (e.sizeExpr)
        {
            auto sv = evaluate(*e.sizeExpr);
            n = sv.isNumber() ? (int)sv.asNumber() : 0;
        }
        if (n < 0)
            n = 0;
        auto arr = std::make_shared<Array>();
        arr->resize(n, QuantumValue(0.0));
        auto cell = std::make_shared<QuantumValue>(QuantumValue(arr));
        auto ptr = std::make_shared<QuantumPointer>();
        ptr->cell = cell;
        ptr->varName = e.typeName + "[]";
        return QuantumValue(ptr);
    }

    // ── new T(args) — single-value heap allocation ────────────────────────────
    std::vector<QuantumValue> args;
    for (auto &a : e.args)
        args.push_back(evaluate(*a));

    QuantumValue val;
    if (primitives.count(e.typeName))
    {
        val = args.empty() ? QuantumValue(0.0) : args[0];
        if (e.typeName == "int" || e.typeName == "long" || e.typeName == "short" || e.typeName == "unsigned")
        {
            if (val.isNumber())
                val = QuantumValue((double)(long long)val.asNumber());
        }
        else if (e.typeName == "char")
        {
            if (val.isNumber())
                val = QuantumValue(std::string(1, (char)(int)val.asNumber()));
            else if (val.isString())
                val = QuantumValue(val.asString().empty() ? std::string("") : std::string(1, val.asString()[0]));
        }
        else if (e.typeName == "bool")
            val = QuantumValue(val.isTruthy());
    }
    else
    {
        // Class type — construct instance and wrap in pointer
        QuantumValue classVal;
        try
        {
            classVal = env->get(e.typeName);
        }
        catch (...)
        {
        }
        if (classVal.isClass())
        {
            auto klass = classVal.asClass();
            auto inst = std::make_shared<QuantumInstance>();
            inst->klass = klass;
            inst->env = std::make_shared<Environment>(env);
            QuantumValue instVal(inst);
            auto k = klass.get();
            std::shared_ptr<QuantumFunction> initFn;
            std::string overloadKey = "init#" + std::to_string(args.size());
            while (k && !initFn)
            {
                auto oit = k->methods.find(overloadKey);
                if (oit != k->methods.end())
                {
                    initFn = oit->second;
                    break;
                }
                auto it = k->methods.find("init");
                if (it != k->methods.end())
                    initFn = it->second;
                k = k->base.get();
            }
            if (initFn)
            {
                auto scope = std::make_shared<Environment>(initFn->closure);
                scope->define("self", instVal);
                scope->define("this", instVal);
                size_t paramStart = (!initFn->params.empty() && (initFn->params[0] == "self" || initFn->params[0] == "this")) ? 1 : 0;
                for (size_t i = paramStart; i < initFn->params.size(); i++)
                {
                    size_t ai = i - paramStart;
                    scope->define(initFn->params[i], ai < args.size() ? args[ai] : QuantumValue());
                }
                try
                {
                    execBlock(initFn->body->as<BlockStmt>(), scope);
                }
                catch (ReturnSignal &)
                {
                }
            }
            auto cell = std::make_shared<QuantumValue>(instVal);
            auto ptr = std::make_shared<QuantumPointer>();
            ptr->cell = cell;
            ptr->varName = e.typeName;
            return QuantumValue(ptr);
        }
        val = args.empty() ? QuantumValue() : args[0];
    }

    auto cell = std::make_shared<QuantumValue>(val);
    auto ptr = std::make_shared<QuantumPointer>();
    ptr->cell = cell;
    ptr->varName = e.typeName;
    return QuantumValue(ptr);
}

// ─── Built-in Method Dispatch ────────────────────────────────────────────────

QuantumValue Interpreter::callMethod(QuantumValue &obj, const std::string &method, std::vector<QuantumValue> args)
{
    if (obj.isArray())
        return callArrayMethod(obj.asArray(), method, std::move(args));
    if (obj.isString())
        return callStringMethod(obj.asString(), method, std::move(args));
    // str.maketrans(from, to) — called on the str type native itself
    if (obj.isFunction() && std::holds_alternative<std::shared_ptr<QuantumNative>>(obj.data))
    {
        auto nat = obj.asNative();
        if ((nat->name == "str" || nat->name == "string") && method == "maketrans")
        {
            // maketrans(from_str, to_str) → dict mapping each char in from to corresponding in to
            if (args.size() < 2 || !args[0].isString() || !args[1].isString())
                throw RuntimeError("str.maketrans() requires two string arguments");
            const std::string &from = args[0].asString();
            const std::string &to = args[1].asString();
            auto table = std::make_shared<Dict>();
            size_t len = std::min(from.size(), to.size());
            for (size_t i = 0; i < len; i++)
                (*table)[std::string(1, from[i])] = QuantumValue(std::string(1, to[i]));
            return QuantumValue(table);
        }
    }
    if (obj.isDict())
    {
        // First check if the dict has a callable stored under that key
        // e.g. console.log(...) where console["log"] is a native function
        auto dict = obj.asDict();
        auto it = dict->find(method);
        if (it != dict->end() && it->second.isFunction())
        {
            auto &fn = it->second;
            if (std::holds_alternative<std::shared_ptr<QuantumNative>>(fn.data))
                return callNative(fn.asNative(), std::move(args));
            if (std::holds_alternative<std::shared_ptr<QuantumFunction>>(fn.data))
                return callFunction(fn.asFunction(), std::move(args));
        }
        return callDictMethod(dict, method, std::move(args));
    }
    if (obj.isInstance())
    {
        auto inst = obj.asInstance();
        // Look up method in class hierarchy — prefer exact arg-count match for overloading
        std::string overloadKey = method + "#" + std::to_string(args.size());
        auto k = inst->klass.get();
        while (k)
        {
            // Try exact overload match first
            auto oit = k->methods.find(overloadKey);
            if (oit != k->methods.end())
                return callInstanceMethod(inst, oit->second, std::move(args));
            auto it = k->methods.find(method);
            if (it != k->methods.end())
                return callInstanceMethod(inst, it->second, std::move(args));
            k = k->base.get();
        }
        // Check instance fields for callable
        if (inst->fields.count(method))
        {
            auto &field = inst->fields[method];
            if (field.isFunction())
            {
                if (std::holds_alternative<std::shared_ptr<QuantumFunction>>(field.data))
                    return callFunction(field.asFunction(), std::move(args));
                if (std::holds_alternative<std::shared_ptr<QuantumNative>>(field.data))
                    return callNative(field.asNative(), std::move(args));
            }
        }
        throw TypeError("No method '" + method + "' on instance of " + inst->klass->name);
    }
    if (obj.isClass())
    {
        // Static method call: ClassName.method(args)
        auto klass = obj.asClass();
        auto it = klass->staticMethods.find(method);
        if (it != klass->staticMethods.end())
            return callFunction(it->second, std::move(args));
        throw TypeError("No static method '" + method + "' on class " + klass->name);
    }
    throw TypeError("No method '" + method + "' on " + obj.typeName());
}

QuantumValue Interpreter::callArrayMethod(std::shared_ptr<Array> arr, const std::string &m, std::vector<QuantumValue> args)
{
    if (m == "push" || m == "append")
    {
        for (auto &a : args)
            arr->push_back(a);
        return QuantumValue();
    }
    if (m == "pop")
    {
        if (arr->empty())
            throw IndexError("pop() on empty array");
        // pop() → remove last (JS/default), pop(i) → remove at index i (Python)
        if (!args.empty() && args[0].isNumber())
        {
            int i = (int)args[0].asNumber();
            if (i < 0)
                i += (int)arr->size();
            if (i < 0 || i >= (int)arr->size())
                throw IndexError("pop() index out of range");
            auto v = (*arr)[i];
            arr->erase(arr->begin() + i);
            return v;
        }
        auto v = arr->back();
        arr->pop_back();
        return v;
    }
    if (m == "shift")
    {
        if (arr->empty())
            throw IndexError("shift() on empty array");
        auto v = arr->front();
        arr->erase(arr->begin());
        return v;
    }
    if (m == "unshift")
    {
        arr->insert(arr->begin(), args.begin(), args.end());
        return QuantumValue();
    }
    if (m == "length")
        return QuantumValue((double)arr->size());
    if (m == "reverse")
    {
        std::reverse(arr->begin(), arr->end());
        return QuantumValue();
    }
    if (m == "contains")
    {
        for (auto &v : *arr)
        {
            if (v.toString() == args[0].toString())
                return QuantumValue(true);
        }
        return QuantumValue(false);
    }
    if (m == "join")
    {
        std::string sep = args.empty() ? "," : args[0].toString();
        std::string res;
        for (size_t i = 0; i < arr->size(); i++)
        {
            if (i)
                res += sep;
            res += (*arr)[i].toString();
        }
        return QuantumValue(res);
    }
    if (m == "slice")
    {
        int start = args.empty() ? 0 : (int)toNum(args[0], "slice");
        int end_ = args.size() > 1 ? (int)toNum(args[1], "slice") : (int)arr->size();
        if (start < 0)
            start += arr->size();
        if (end_ < 0)
            end_ += arr->size();
        start = std::max(0, std::min(start, (int)arr->size()));
        end_ = std::max(0, std::min(end_, (int)arr->size()));
        auto res = std::make_shared<Array>(arr->begin() + start, arr->begin() + end_);
        return QuantumValue(res);
    }
    if (m == "map")
    {
        if (args.empty())
            throw RuntimeError("map() requires function argument");
        auto res = std::make_shared<Array>();
        for (auto &item : *arr)
        {
            std::vector<QuantumValue> callArgs = {item};
            QuantumValue fn = args[0];
            QuantumValue r;
            if (std::holds_alternative<std::shared_ptr<QuantumFunction>>(fn.data))
                r = callFunction(fn.asFunction(), callArgs);
            else if (std::holds_alternative<std::shared_ptr<QuantumNative>>(fn.data))
                r = callNative(fn.asNative(), callArgs);
            res->push_back(r);
        }
        return QuantumValue(res);
    }
    if (m == "filter")
    {
        if (args.empty())
            throw RuntimeError("filter() requires function argument");
        auto res = std::make_shared<Array>();
        for (auto &item : *arr)
        {
            std::vector<QuantumValue> callArgs = {item};
            QuantumValue fn = args[0];
            QuantumValue r;
            if (std::holds_alternative<std::shared_ptr<QuantumFunction>>(fn.data))
                r = callFunction(fn.asFunction(), callArgs);
            else if (std::holds_alternative<std::shared_ptr<QuantumNative>>(fn.data))
                r = callNative(fn.asNative(), callArgs);
            if (r.isTruthy())
                res->push_back(item);
        }
        return QuantumValue(res);
    }
    if (m == "sort")
    {
        std::sort(arr->begin(), arr->end(), [](const QuantumValue &a, const QuantumValue &b)
                  {
            if (a.isNumber() && b.isNumber()) return a.asNumber() < b.asNumber();
            return a.toString() < b.toString(); });
        return QuantumValue();
    }
    throw TypeError("Array has no method '" + m + "'");
}

QuantumValue Interpreter::callStringMethod(const std::string &str, const std::string &m, std::vector<QuantumValue> args)
{
    if (m == "length" || m == "len")
        return QuantumValue((double)str.size());
    if (m == "upper")
    {
        std::string r = str;
        std::transform(r.begin(), r.end(), r.begin(), ::toupper);
        return QuantumValue(r);
    }
    if (m == "lower")
    {
        std::string r = str;
        std::transform(r.begin(), r.end(), r.begin(), ::tolower);
        return QuantumValue(r);
    }
    if (m == "trim")
    {
        size_t s = str.find_first_not_of(" \t\n\r");
        size_t e = str.find_last_not_of(" \t\n\r");
        if (s == std::string::npos)
            return QuantumValue(std::string(""));
        return QuantumValue(str.substr(s, e - s + 1));
    }
    if (m == "split")
    {
        std::string sep = args.empty() ? " " : args[0].toString();
        auto arr = std::make_shared<Array>();
        if (sep.empty())
        {
            for (char c : str)
                arr->push_back(QuantumValue(std::string(1, c)));
            return QuantumValue(arr);
        }
        size_t pos = 0, found;
        while ((found = str.find(sep, pos)) != std::string::npos)
        {
            arr->push_back(QuantumValue(str.substr(pos, found - pos)));
            pos = found + sep.size();
        }
        arr->push_back(QuantumValue(str.substr(pos)));
        return QuantumValue(arr);
    }
    if (m == "contains")
    {
        return QuantumValue(str.find(args[0].toString()) != std::string::npos);
    }
    if (m == "starts_with")
    {
        auto p = args[0].toString();
        return QuantumValue(str.size() >= p.size() && str.substr(0, p.size()) == p);
    }
    if (m == "ends_with")
    {
        auto p = args[0].toString();
        return QuantumValue(str.size() >= p.size() && str.substr(str.size() - p.size()) == p);
    }
    if (m == "replace")
    {
        if (args.size() < 2)
            throw RuntimeError("replace() requires 2 args");
        std::string from = args[0].toString(), to = args[1].toString();
        std::string result = str;
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos)
        {
            result.replace(pos, from.size(), to);
            pos += to.size();
        }
        return QuantumValue(result);
    }
    if (m == "slice" || m == "substr")
    {
        int start = args.empty() ? 0 : (int)toNum(args[0], "slice");
        int len_ = args.size() > 1 ? (int)toNum(args[1], "slice") : (int)str.size() - start;
        if (start < 0)
            start += str.size();
        return QuantumValue(str.substr(std::max(0, start), len_));
    }
    if (m == "index")
    {
        auto sub = args[0].toString();
        auto p = str.find(sub);
        return QuantumValue(p == std::string::npos ? QuantumValue(-1.0) : QuantumValue((double)p));
    }
    if (m == "repeat")
    {
        int n = (int)toNum(args[0], "repeat");
        std::string res;
        for (int i = 0; i < n; i++)
            res += str;
        return QuantumValue(res);
    }
    if (m == "chars")
    {
        auto arr = std::make_shared<Array>();
        for (char c : str)
            arr->push_back(QuantumValue(std::string(1, c)));
        return QuantumValue(arr);
    }

    // ── JavaScript-style aliases ──────────────────────────────────────────
    if (m == "toLowerCase")
        return callStringMethod(str, "lower", std::move(args));
    if (m == "toUpperCase")
        return callStringMethod(str, "upper", std::move(args));
    if (m == "includes")
        return callStringMethod(str, "contains", std::move(args));
    if (m == "startsWith")
        return callStringMethod(str, "starts_with", std::move(args));
    if (m == "endsWith")
        return callStringMethod(str, "ends_with", std::move(args));
    if (m == "indexOf")
        return callStringMethod(str, "index", std::move(args));
    if (m == "substring" || m == "subString")
        return callStringMethod(str, "slice", std::move(args));
    if (m == "trimStart" || m == "trimEnd")
        return callStringMethod(str, "trim", std::move(args));
    if (m == "toLocaleLowerCase")
        return callStringMethod(str, "lower", std::move(args));
    if (m == "toLocaleUpperCase")
        return callStringMethod(str, "upper", std::move(args));
    if (m == "padStart")
    {
        int targetLen = args.empty() ? 0 : (int)toNum(args[0], "padStart");
        std::string pad = args.size() > 1 ? args[1].toString() : " ";
        std::string result = str;
        while ((int)result.size() < targetLen)
            result = pad + result;
        return QuantumValue(result.substr(result.size() - std::max((int)str.size(), targetLen)));
    }
    if (m == "padEnd")
    {
        int targetLen = args.empty() ? 0 : (int)toNum(args[0], "padEnd");
        std::string pad = args.size() > 1 ? args[1].toString() : " ";
        std::string result = str;
        while ((int)result.size() < targetLen)
            result += pad;
        return QuantumValue(result.substr(0, std::max((int)str.size(), targetLen)));
    }
    if (m == "charAt")
    {
        int idx = args.empty() ? 0 : (int)toNum(args[0], "charAt");
        if (idx < 0 || idx >= (int)str.size())
            return QuantumValue(std::string(""));
        return QuantumValue(std::string(1, str[idx]));
    }
    if (m == "charCodeAt")
    {
        int idx = args.empty() ? 0 : (int)toNum(args[0], "charCodeAt");
        if (idx < 0 || idx >= (int)str.size())
            return QuantumValue(std::numeric_limits<double>::quiet_NaN());
        return QuantumValue((double)(unsigned char)str[idx]);
    }
    if (m == "at")
    {
        int idx = args.empty() ? 0 : (int)toNum(args[0], "at");
        if (idx < 0)
            idx += (int)str.size();
        if (idx < 0 || idx >= (int)str.size())
            return QuantumValue();
        return QuantumValue(std::string(1, str[idx]));
    }

    // translate(table) — apply a char-mapping dict produced by maketrans
    if (m == "translate")
    {
        if (args.empty() || !args[0].isDict())
            throw RuntimeError("translate() requires a dict translation table");
        auto &table = *args[0].asDict();
        std::string result;
        for (char c : str)
        {
            std::string key(1, c);
            auto it = table.find(key);
            if (it != table.end())
                result += it->second.toString();
            else
                result += c;
        }
        return QuantumValue(result);
    }
    if (m == "isdigit" || m == "isnumeric")
    {
        if (str.empty())
            return QuantumValue(false);
        for (char c : str)
            if (!std::isdigit((unsigned char)c))
                return QuantumValue(false);
        return QuantumValue(true);
    }
    if (m == "isalpha")
    {
        if (str.empty())
            return QuantumValue(false);
        for (char c : str)
            if (!std::isalpha((unsigned char)c))
                return QuantumValue(false);
        return QuantumValue(true);
    }
    if (m == "isalnum")
    {
        if (str.empty())
            return QuantumValue(false);
        for (char c : str)
            if (!std::isalnum((unsigned char)c))
                return QuantumValue(false);
        return QuantumValue(true);
    }
    if (m == "isspace")
    {
        if (str.empty())
            return QuantumValue(false);
        for (char c : str)
            if (!std::isspace((unsigned char)c))
                return QuantumValue(false);
        return QuantumValue(true);
    }
    if (m == "isupper")
    {
        if (str.empty())
            return QuantumValue(false);
        for (char c : str)
            if (std::isalpha((unsigned char)c) && !std::isupper((unsigned char)c))
                return QuantumValue(false);
        return QuantumValue(true);
    }
    if (m == "islower")
    {
        if (str.empty())
            return QuantumValue(false);
        for (char c : str)
            if (std::isalpha((unsigned char)c) && !std::islower((unsigned char)c))
                return QuantumValue(false);
        return QuantumValue(true);
    }
    if (m == "strip")
    {
        std::string chars = args.empty() ? " \t\n\r" : args[0].asString();
        std::string r = str;
        auto isChars = [&](char c)
        { return chars.find(c) != std::string::npos; };
        size_t s = 0, e = r.size();
        while (s < e && isChars(r[s]))
            s++;
        while (e > s && isChars(r[e - 1]))
            e--;
        return QuantumValue(r.substr(s, e - s));
    }
    if (m == "lstrip")
    {
        std::string chars = args.empty() ? " \t\n\r" : args[0].asString();
        std::string r = str;
        size_t s = 0;
        while (s < r.size() && chars.find(r[s]) != std::string::npos)
            s++;
        return QuantumValue(r.substr(s));
    }
    if (m == "rstrip")
    {
        std::string chars = args.empty() ? " \t\n\r" : args[0].asString();
        std::string r = str;
        size_t e = r.size();
        while (e > 0 && chars.find(r[e - 1]) != std::string::npos)
            e--;
        return QuantumValue(r.substr(0, e));
    }

    throw TypeError("String has no method '" + m + "'");
}

QuantumValue Interpreter::callDictMethod(std::shared_ptr<Dict> dict, const std::string &m, std::vector<QuantumValue> args)
{
    if (m == "has" || m == "contains" || m == "hasOwnProperty")
        return QuantumValue(dict->count(args[0].toString()) > 0);
    // .items() → array of [key, value] pairs (Python/JS compatibility)
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
    if (m == "get")
    {
        auto key = args[0].toString();
        auto it = dict->find(key);
        if (it != dict->end())
            return it->second;
        return args.size() > 1 ? args[1] : QuantumValue();
    }
    if (m == "set")
    {
        (*dict)[args[0].toString()] = args[1];
        return QuantumValue();
    }
    if (m == "delete")
    {
        dict->erase(args[0].toString());
        return QuantumValue();
    }
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
    if (m == "length" || m == "size")
        return QuantumValue((double)dict->size());
    throw TypeError("Dict has no method '" + m + "'");
}