#include "Compiler.h"
#include "Error.h"
#include "Vm.h"
#include <stdexcept>
#include <algorithm>
#include <unordered_map>

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
