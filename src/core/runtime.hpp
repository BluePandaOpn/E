#pragma once

#include "ast.hpp"

#include <functional>
#include <memory>
#include <unordered_set>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

struct Function;
struct NativeFunction;
struct ClassValue;
struct InstanceValue;
class Environment;
class Interpreter;

using Value = std::variant<std::monostate,
                           double,
                           bool,
                           std::string,
                           std::shared_ptr<Function>,
                           std::shared_ptr<NativeFunction>,
                           std::shared_ptr<ClassValue>,
                           std::shared_ptr<InstanceValue>>;

std::string valueToString(const Value& value);
bool valueIsTruthy(const Value& value);
bool valueEquals(const Value& a, const Value& b);

class Environment : public std::enable_shared_from_this<Environment> {
public:
    explicit Environment(std::shared_ptr<Environment> parent = nullptr);

    void define(const std::string& name, const Value& value);
    void assign(const std::string& name, const Value& value, int line);
    Value get(const std::string& name, int line) const;
    bool hasLocal(const std::string& name) const;
    Value getLocal(const std::string& name) const;
    void eraseLocal(const std::string& name);

private:
    std::unordered_map<std::string, Value> values_;
    std::shared_ptr<Environment> parent_;
};

struct Function {
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<BlockStmt> body;
    std::shared_ptr<Environment> closure;
    bool isInitializer = false;
};

struct NativeFunction {
    std::string name;
    int arity = -1; // -1 = variadica
    std::function<Value(const std::vector<Value>&)> fn;
};

struct ClassValue {
    std::string name;
    std::unordered_map<std::string, std::shared_ptr<Function>> methods;
};

struct InstanceValue {
    std::shared_ptr<ClassValue> klass;
    std::unordered_map<std::string, Value> fields;
};

class Interpreter {
public:
    Interpreter();
    explicit Interpreter(const std::string& entryFilePath);
    void executeProgram(const std::vector<StmtPtr>& program);

private:
    Value evaluate(const ExprPtr& expr);
    void execute(const StmtPtr& stmt);
    void executeBlock(const std::vector<StmtPtr>& statements, const std::shared_ptr<Environment>& env);

    Value evalLiteral(const LiteralExpr& expr) const;
    Value evalUnary(const UnaryExpr& expr);
    Value evalBinary(const BinaryExpr& expr);
    Value evalCall(const CallExpr& expr);
    Value callFunction(const std::shared_ptr<Function>& fn, const Value* args, size_t argCount, int line);
    Value evalGet(const GetExpr& expr);
    Value evalSet(const SetExpr& expr);
    std::shared_ptr<Function> bindThis(const std::shared_ptr<Function>& method, const std::shared_ptr<InstanceValue>& instance);
    void executeImportToken(const Token& module, int line, int column);

    static double expectNumber(const Value& value, int line, const std::string& context);

    std::shared_ptr<Environment> globals_;
    std::shared_ptr<Environment> env_;
    std::string currentFilePath_;
    std::unordered_set<std::string> importedModules_;
    std::unordered_map<std::string, std::string> importedModulePackageIds_;
    std::unordered_map<std::string, std::string> packageIdToModulePath_;
    std::unordered_map<std::string, int> loadedLibRegistryCount_;
#ifdef _WIN32
    std::vector<void*> loadedLibHandles_;
#else
    std::vector<void*> loadedLibHandles_;
#endif
};
