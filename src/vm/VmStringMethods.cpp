#include "Vm.h"
#include "Error.h"
#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <string>
#include <vector>

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

