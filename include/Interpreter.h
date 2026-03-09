#pragma once
#include "AST.h"
#include "Value.h"
#include <memory>

class Interpreter
{
public:
    Interpreter();
    void execute(ASTNode &node);
    QuantumValue evaluate(ASTNode &node);
    void execBlock(BlockStmt &s, std::shared_ptr<Environment> scope = nullptr);

    std::shared_ptr<Environment> globals;

private:
    std::shared_ptr<Environment> env;
    long long stepCount_ = 0; // guards against infinite loops (e.g. empty-stdin programs)
    static constexpr long long MAX_STEPS = 2'000'000;

    void registerNatives();

    // Statement executors
    void execVarDecl(VarDecl &s);
    void execFunctionDecl(FunctionDecl &s);
    void execClassDecl(ClassDecl &s);
    void execIf(IfStmt &s);
    void execWhile(WhileStmt &s);
    void execFor(ForStmt &s);
    void execReturn(ReturnStmt &s);
    void execPrint(PrintStmt &s);
    void execInput(InputStmt &s);
    void execImport(ImportStmt &s);
    void execExprStmt(ExprStmt &s);

    // Expression evaluators
    QuantumValue evalBinary(BinaryExpr &e);
    QuantumValue evalUnary(UnaryExpr &e);
    QuantumValue evalAssign(AssignExpr &e);
    QuantumValue evalCall(CallExpr &e);
    QuantumValue evalIndex(IndexExpr &e);
    QuantumValue evalMember(MemberExpr &e);
    QuantumValue evalArray(ArrayLiteral &e);
    QuantumValue evalDict(DictLiteral &e);
    QuantumValue evalLambda(LambdaExpr &e);
    QuantumValue evalListComp(ListComp &e);
    QuantumValue evalIdentifier(Identifier &e);

    // ── C++ Pointer evaluators ─────────────────────────────────────────────
    QuantumValue evalAddressOf(AddressOfExpr &e); // &var
    QuantumValue evalDeref(DerefExpr &e);         // *ptr
    QuantumValue evalArrow(ArrowExpr &e);         // ptr->member
    QuantumValue evalNewExpr(NewExpr &e);         // new T(args)

    QuantumValue callFunction(std::shared_ptr<QuantumFunction> fn, std::vector<QuantumValue> args);
    QuantumValue callNative(std::shared_ptr<QuantumNative> fn, std::vector<QuantumValue> args);
    QuantumValue callInstanceMethod(std::shared_ptr<QuantumInstance> inst, std::shared_ptr<QuantumFunction> fn, std::vector<QuantumValue> args);

    // Built-in method dispatch
    QuantumValue callMethod(QuantumValue &obj, const std::string &method, std::vector<QuantumValue> args);
    QuantumValue callArrayMethod(std::shared_ptr<Array> arr, const std::string &method, std::vector<QuantumValue> args);
    QuantumValue callStringMethod(const std::string &str, const std::string &method, std::vector<QuantumValue> args);
    QuantumValue callDictMethod(std::shared_ptr<Dict> dict, const std::string &method, std::vector<QuantumValue> args);

    void setLValue(ASTNode &target, QuantumValue val, const std::string &op);
};