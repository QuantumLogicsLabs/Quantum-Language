#include "Vm.h"
#include "Error.h"
#include <memory>
#include <string>
#include <vector>

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
