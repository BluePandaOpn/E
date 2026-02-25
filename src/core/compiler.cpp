#include "compiler.hpp"
#include "error.hpp"

#include "token.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <array>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

namespace {
namespace term {
constexpr const char* kReset = "\x1b[0m";
constexpr const char* kCyan = "\x1b[36m";
constexpr const char* kGreen = "\x1b[32m";
constexpr const char* kYellow = "\x1b[33m";
constexpr const char* kRed = "\x1b[31m";
}

void printStep(const char* color, const std::string& tag, const std::string& message) {
    std::cout << color << "[" << tag << "]" << term::kReset << " " << message << "\n";
}

int runWithSpinner(const std::string& command,
                   const std::string& message,
                   const std::string& successMessage,
                   const std::string& failMessage) {
    static const std::array<const char*, 4> kFrames = {"/", "-", "|", "\\"};
    auto runner = std::async(std::launch::async, [&command]() { return std::system(command.c_str()); });

    size_t frame = 0;
    while (runner.wait_for(std::chrono::milliseconds(120)) != std::future_status::ready) {
        std::cout << "\r" << term::kCyan << "[" << kFrames[frame % kFrames.size()] << "]" << term::kReset << " " << message
                  << "   " << std::flush;
        ++frame;
    }
    const int rc = runner.get();
    std::cout << "\r\033[2K\r" << std::flush;

    if (rc == 0) {
        printStep(term::kGreen, "+", successMessage);
    } else {
        printStep(term::kRed, "!", failMessage);
    }
    return rc;
}

std::string escapeCpp(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (char c : text) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

std::string tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::LeftParen: return "TokenType::LeftParen";
        case TokenType::RightParen: return "TokenType::RightParen";
        case TokenType::LeftBrace: return "TokenType::LeftBrace";
        case TokenType::RightBrace: return "TokenType::RightBrace";
        case TokenType::Comma: return "TokenType::Comma";
        case TokenType::Colon: return "TokenType::Colon";
        case TokenType::Dot: return "TokenType::Dot";
        case TokenType::Plus: return "TokenType::Plus";
        case TokenType::Minus: return "TokenType::Minus";
        case TokenType::Star: return "TokenType::Star";
        case TokenType::Slash: return "TokenType::Slash";
        case TokenType::Semicolon: return "TokenType::Semicolon";
        case TokenType::Assign: return "TokenType::Assign";
        case TokenType::EqualEqual: return "TokenType::EqualEqual";
        case TokenType::Bang: return "TokenType::Bang";
        case TokenType::BangEqual: return "TokenType::BangEqual";
        case TokenType::Greater: return "TokenType::Greater";
        case TokenType::GreaterEqual: return "TokenType::GreaterEqual";
        case TokenType::Less: return "TokenType::Less";
        case TokenType::LessEqual: return "TokenType::LessEqual";
        case TokenType::Identifier: return "TokenType::Identifier";
        case TokenType::Number: return "TokenType::Number";
        case TokenType::String: return "TokenType::String";
        case TokenType::Var: return "TokenType::Var";
        case TokenType::Func: return "TokenType::Func";
        case TokenType::Return: return "TokenType::Return";
        case TokenType::If: return "TokenType::If";
        case TokenType::Else: return "TokenType::Else";
        case TokenType::While: return "TokenType::While";
        case TokenType::End: return "TokenType::End";
        case TokenType::Class: return "TokenType::Class";
        case TokenType::This: return "TokenType::This";
        case TokenType::Import: return "TokenType::Import";
        case TokenType::From: return "TokenType::From";
        case TokenType::True: return "TokenType::True";
        case TokenType::False: return "TokenType::False";
        case TokenType::Null: return "TokenType::Null";
        case TokenType::And: return "TokenType::And";
        case TokenType::Or: return "TokenType::Or";
        case TokenType::Not: return "TokenType::Not";
        case TokenType::At: return "TokenType::At";
        case TokenType::Newline: return "TokenType::Newline";
        case TokenType::Eof: return "TokenType::Eof";
    }
    return "TokenType::Eof";
}

std::filesystem::path findProjectRoot(const std::filesystem::path& startDir) {
    std::filesystem::path current = std::filesystem::absolute(startDir);
    while (!current.empty()) {
        const bool hasCore =
            std::filesystem::exists(current / "src" / "core" / "lexer.cpp") &&
            std::filesystem::exists(current / "src" / "core" / "parser.cpp") &&
            std::filesystem::exists(current / "src" / "core" / "runtime.cpp");
        if (hasCore) return current;

        auto parent = current.parent_path();
        if (parent == current) break;
        current = parent;
    }
    return {};
}

std::filesystem::path resolveProjectRoot(const std::string& sourceLabel) {
    if (!sourceLabel.empty()) {
        std::filesystem::path sourcePath = std::filesystem::absolute(sourceLabel);
        auto fromSource = findProjectRoot(sourcePath.parent_path());
        if (!fromSource.empty()) return fromSource;
    }

    auto fromCwd = findProjectRoot(std::filesystem::current_path());
    if (!fromCwd.empty()) return fromCwd;

    std::filesystem::path fromFile(__FILE__);
    if (fromFile.is_relative()) fromFile = std::filesystem::absolute(fromFile);
    if (fromFile.has_parent_path()) {
        auto fromFilePath = findProjectRoot(fromFile.parent_path());
        if (!fromFilePath.empty()) return fromFilePath;
    }

    return std::filesystem::current_path();
}

std::filesystem::path resolveNativeIncludeDir(const std::filesystem::path& projectRoot) {
    const std::filesystem::path localInclude = projectRoot / "include";
    if (std::filesystem::exists(localInclude / "epp_native.h")) return localInclude;

    const std::filesystem::path siblingLibInclude = projectRoot.parent_path() / "lib" / "include";
    if (std::filesystem::exists(siblingLibInclude / "epp_native.h")) return siblingLibInclude;

    const std::filesystem::path nestedLibInclude = projectRoot / "lib" / "include";
    if (std::filesystem::exists(nestedLibInclude / "epp_native.h")) return nestedLibInclude;

    return localInclude;
}

int normalizeOptLevel(int level) {
    if (level < 0) return 0;
    if (level > 3) return 3;
    return level;
}

std::string backendName(AotBackend backend) {
    switch (backend) {
        case AotBackend::CppAot: return "cpp";
        case AotBackend::LlvmAot: return "llvm-aot";
        case AotBackend::LlvmJit: return "llvm-jit";
    }
    return "cpp";
}

std::string gxxOptFlag(int level) {
    switch (normalizeOptLevel(level)) {
        case 0: return "-O0";
        case 1: return "-O1";
        case 2: return "-O2";
        default: return "-O3";
    }
}

std::string msvcOptFlags(int level) {
    switch (normalizeOptLevel(level)) {
        case 0: return "/Od";
        case 1: return "/O1";
        default: return "/O2 /Ot";
    }
}

class AstEmitter {
public:
    std::string emitProgram(const std::vector<StmtPtr>& program, const std::string& sourceLabel) {
        out_ << "#include \"runtime.hpp\"\n";
        out_ << "#include \"error.hpp\"\n";
        out_ << "\n";
        out_ << "#include <iostream>\n";
        out_ << "#include <memory>\n";
        out_ << "#include <string>\n";
        out_ << "#include <vector>\n\n";
        out_ << "namespace {\n";
        out_ << "Token makeToken(TokenType type, const std::string& lexeme, int line) {\n";
        out_ << "    return Token{type, lexeme, line};\n";
        out_ << "}\n\n";
        out_ << "std::vector<StmtPtr> buildProgram() {\n";
        out_ << "    std::vector<StmtPtr> program;\n";
        out_ << "    program.reserve(" << program.size() << ");\n";
        for (const auto& stmt : program) {
            const std::string stmtVar = emitStmt(stmt);
            out_ << "    program.push_back(" << stmtVar << ");\n";
        }
        out_ << "    return program;\n";
        out_ << "}\n";
        out_ << "} // namespace\n\n";
        out_ << "int main() {\n";
        out_ << "    try {\n";
        out_ << "        Interpreter interpreter(\"" << escapeCpp(sourceLabel) << "\");\n";
        out_ << "        interpreter.executeProgram(buildProgram());\n";
        out_ << "        return 0;\n";
        out_ << "    } catch (const EppError& e) {\n";
        out_ << "        std::cerr << \"\\x1b[31m[E++ \" << errorPhaseName(e.phase()) << \" \" << e.code() << \"]\\x1b[0m \"\n";
        out_ << "                  << \"Linea \" << e.line() << \", Columna \" << e.column() << \": \" << e.what() << \"\\n\";\n";
        out_ << "        if (!e.near().empty()) std::cerr << \"  Cerca de: '\" << e.near() << \"'\\n\";\n";
        out_ << "        if (!e.hint().empty()) std::cerr << \"  Sugerencia: \" << e.hint() << \"\\n\";\n";
        out_ << "        return 2;\n";
        out_ << "    } catch (const std::exception& e) {\n";
        out_ << "        std::cerr << \"[E++ FATAL] (" << escapeCpp(sourceLabel) << ") \" << e.what() << \"\\n\";\n";
        out_ << "        return 3;\n";
        out_ << "    }\n";
        out_ << "}\n";
        return out_.str();
    }

private:
    std::string emitExpr(const ExprPtr& expr) {
        if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
            const std::string name = next("e");
            out_ << "    auto " << name << " = std::make_shared<LiteralExpr>();\n";
            out_ << "    " << name << "->line = " << lit->line << ";\n";
            out_ << "    " << name << "->valueToken = makeToken(" << tokenTypeName(lit->valueToken.type)
                 << ", \"" << escapeCpp(lit->valueToken.lexeme) << "\", " << lit->valueToken.line << ");\n";
            return name;
        }
        if (auto var = std::dynamic_pointer_cast<VariableExpr>(expr)) {
            const std::string name = next("e");
            out_ << "    auto " << name << " = std::make_shared<VariableExpr>();\n";
            out_ << "    " << name << "->line = " << var->line << ";\n";
            out_ << "    " << name << "->name = makeToken(" << tokenTypeName(var->name.type)
                 << ", \"" << escapeCpp(var->name.lexeme) << "\", " << var->name.line << ");\n";
            return name;
        }
        if (auto assign = std::dynamic_pointer_cast<AssignExpr>(expr)) {
            const std::string value = emitExpr(assign->value);
            const std::string name = next("e");
            out_ << "    auto " << name << " = std::make_shared<AssignExpr>();\n";
            out_ << "    " << name << "->line = " << assign->line << ";\n";
            out_ << "    " << name << "->name = makeToken(" << tokenTypeName(assign->name.type)
                 << ", \"" << escapeCpp(assign->name.lexeme) << "\", " << assign->name.line << ");\n";
            out_ << "    " << name << "->value = " << value << ";\n";
            return name;
        }
        if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
            const std::string right = emitExpr(unary->right);
            const std::string name = next("e");
            out_ << "    auto " << name << " = std::make_shared<UnaryExpr>();\n";
            out_ << "    " << name << "->line = " << unary->line << ";\n";
            out_ << "    " << name << "->op = makeToken(" << tokenTypeName(unary->op.type)
                 << ", \"" << escapeCpp(unary->op.lexeme) << "\", " << unary->op.line << ");\n";
            out_ << "    " << name << "->right = " << right << ";\n";
            return name;
        }
        if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
            const std::string left = emitExpr(binary->left);
            const std::string right = emitExpr(binary->right);
            const std::string name = next("e");
            out_ << "    auto " << name << " = std::make_shared<BinaryExpr>();\n";
            out_ << "    " << name << "->line = " << binary->line << ";\n";
            out_ << "    " << name << "->left = " << left << ";\n";
            out_ << "    " << name << "->op = makeToken(" << tokenTypeName(binary->op.type)
                 << ", \"" << escapeCpp(binary->op.lexeme) << "\", " << binary->op.line << ");\n";
            out_ << "    " << name << "->right = " << right << ";\n";
            return name;
        }
        if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
            const std::string callee = emitExpr(call->callee);
            const std::string name = next("e");
            out_ << "    auto " << name << " = std::make_shared<CallExpr>();\n";
            out_ << "    " << name << "->line = " << call->line << ";\n";
            out_ << "    " << name << "->callee = " << callee << ";\n";
            for (const auto& arg : call->args) {
                const std::string argExpr = emitExpr(arg);
                out_ << "    " << name << "->args.push_back(" << argExpr << ");\n";
            }
            return name;
        }
        if (auto getExpr = std::dynamic_pointer_cast<GetExpr>(expr)) {
            const std::string object = emitExpr(getExpr->object);
            const std::string name = next("e");
            out_ << "    auto " << name << " = std::make_shared<GetExpr>();\n";
            out_ << "    " << name << "->line = " << getExpr->line << ";\n";
            out_ << "    " << name << "->object = " << object << ";\n";
            out_ << "    " << name << "->name = makeToken(" << tokenTypeName(getExpr->name.type)
                 << ", \"" << escapeCpp(getExpr->name.lexeme) << "\", " << getExpr->name.line << ");\n";
            return name;
        }
        if (auto setExpr = std::dynamic_pointer_cast<SetExpr>(expr)) {
            const std::string object = emitExpr(setExpr->object);
            const std::string value = emitExpr(setExpr->value);
            const std::string name = next("e");
            out_ << "    auto " << name << " = std::make_shared<SetExpr>();\n";
            out_ << "    " << name << "->line = " << setExpr->line << ";\n";
            out_ << "    " << name << "->object = " << object << ";\n";
            out_ << "    " << name << "->name = makeToken(" << tokenTypeName(setExpr->name.type)
                 << ", \"" << escapeCpp(setExpr->name.lexeme) << "\", " << setExpr->name.line << ");\n";
            out_ << "    " << name << "->value = " << value << ";\n";
            return name;
        }
        throw EppError(ErrorPhase::Compiler,
                       "E-COMP-001",
                       expr ? expr->line : 0,
                       1,
                       "Nodo de expresion no soportado en compilacion AOT.");
    }

    std::string emitBlock(const std::shared_ptr<BlockStmt>& block) {
        const std::string name = next("b");
        out_ << "    auto " << name << " = std::make_shared<BlockStmt>();\n";
        out_ << "    " << name << "->line = " << block->line << ";\n";
        for (const auto& stmt : block->statements) {
            const std::string stmtVar = emitStmt(stmt);
            out_ << "    " << name << "->statements.push_back(" << stmtVar << ");\n";
        }
        return name;
    }

    std::string emitFuncDecl(const std::shared_ptr<FuncDeclStmt>& func) {
        const std::string body = emitBlock(func->body);
        const std::string name = next("s");
        out_ << "    auto " << name << " = std::make_shared<FuncDeclStmt>();\n";
        out_ << "    " << name << "->line = " << func->line << ";\n";
        out_ << "    " << name << "->name = makeToken(" << tokenTypeName(func->name.type)
             << ", \"" << escapeCpp(func->name.lexeme) << "\", " << func->name.line << ");\n";
        for (const auto& p : func->params) {
            out_ << "    " << name << "->params.push_back(makeToken(" << tokenTypeName(p.type)
                 << ", \"" << escapeCpp(p.lexeme) << "\", " << p.line << "));\n";
        }
        out_ << "    " << name << "->body = " << body << ";\n";
        return name;
    }

    std::string emitStmt(const StmtPtr& stmt) {
        if (auto s = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
            const std::string expr = emitExpr(s->expr);
            const std::string name = next("s");
            out_ << "    auto " << name << " = std::make_shared<ExprStmt>();\n";
            out_ << "    " << name << "->line = " << s->line << ";\n";
            out_ << "    " << name << "->expr = " << expr << ";\n";
            return name;
        }
        if (auto s = std::dynamic_pointer_cast<PrintStmt>(stmt)) {
            const std::string expr = emitExpr(s->expr);
            const std::string name = next("s");
            out_ << "    auto " << name << " = std::make_shared<PrintStmt>();\n";
            out_ << "    " << name << "->line = " << s->line << ";\n";
            out_ << "    " << name << "->expr = " << expr << ";\n";
            return name;
        }
        if (auto s = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
            const std::string name = next("s");
            out_ << "    auto " << name << " = std::make_shared<VarDeclStmt>();\n";
            out_ << "    " << name << "->line = " << s->line << ";\n";
            out_ << "    " << name << "->name = makeToken(" << tokenTypeName(s->name.type)
                 << ", \"" << escapeCpp(s->name.lexeme) << "\", " << s->name.line << ");\n";
            if (s->initializer) {
                const std::string initExpr = emitExpr(s->initializer);
                out_ << "    " << name << "->initializer = " << initExpr << ";\n";
            }
            return name;
        }
        if (auto s = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
            return emitBlock(s);
        }
        if (auto s = std::dynamic_pointer_cast<IfStmt>(stmt)) {
            const std::string cond = emitExpr(s->condition);
            const std::string thenBlock = emitBlock(s->thenBranch);
            const std::string name = next("s");
            out_ << "    auto " << name << " = std::make_shared<IfStmt>();\n";
            out_ << "    " << name << "->line = " << s->line << ";\n";
            out_ << "    " << name << "->condition = " << cond << ";\n";
            out_ << "    " << name << "->thenBranch = " << thenBlock << ";\n";
            if (s->elseBranch) {
                const std::string elseBlock = emitBlock(s->elseBranch);
                out_ << "    " << name << "->elseBranch = " << elseBlock << ";\n";
            }
            return name;
        }
        if (auto s = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
            const std::string cond = emitExpr(s->condition);
            const std::string body = emitBlock(s->body);
            const std::string name = next("s");
            out_ << "    auto " << name << " = std::make_shared<WhileStmt>();\n";
            out_ << "    " << name << "->line = " << s->line << ";\n";
            out_ << "    " << name << "->condition = " << cond << ";\n";
            out_ << "    " << name << "->body = " << body << ";\n";
            return name;
        }
        if (auto s = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
            const std::string name = next("s");
            out_ << "    auto " << name << " = std::make_shared<ReturnStmt>();\n";
            out_ << "    " << name << "->line = " << s->line << ";\n";
            if (s->value) {
                const std::string valueExpr = emitExpr(s->value);
                out_ << "    " << name << "->value = " << valueExpr << ";\n";
            }
            return name;
        }
        if (auto s = std::dynamic_pointer_cast<ImportStmt>(stmt)) {
            const std::string name = next("s");
            out_ << "    auto " << name << " = std::make_shared<ImportStmt>();\n";
            out_ << "    " << name << "->line = " << s->line << ";\n";
            out_ << "    " << name << "->module = makeToken(" << tokenTypeName(s->module.type)
                 << ", \"" << escapeCpp(s->module.lexeme) << "\", " << s->module.line << ");\n";
            return name;
        }
        if (auto s = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
            return emitFuncDecl(s);
        }
        if (auto s = std::dynamic_pointer_cast<ClassDeclStmt>(stmt)) {
            const std::string name = next("s");
            out_ << "    auto " << name << " = std::make_shared<ClassDeclStmt>();\n";
            out_ << "    " << name << "->line = " << s->line << ";\n";
            out_ << "    " << name << "->name = makeToken(" << tokenTypeName(s->name.type)
                 << ", \"" << escapeCpp(s->name.lexeme) << "\", " << s->name.line << ");\n";
            for (const auto& method : s->methods) {
                const std::string methodVar = emitFuncDecl(method);
                out_ << "    " << name << "->methods.push_back(" << methodVar << ");\n";
            }
            return name;
        }
        throw EppError(ErrorPhase::Compiler,
                       "E-COMP-002",
                       stmt ? stmt->line : 0,
                       1,
                       "Nodo de sentencia no soportado en compilacion AOT.");
    }

    std::string next(const char* prefix) {
        ++counter_;
        return std::string(prefix) + std::to_string(counter_);
    }

    std::ostringstream out_;
    int counter_ = 0;
};
} // namespace

void AotCompiler::compileProgram(const std::vector<StmtPtr>& program,
                                 const std::string& outputExePath,
                                 const std::string& sourceLabel,
                                 const AotCompileOptions& options) const {
    if (options.backend != AotBackend::CppAot) {
        throw EppError(ErrorPhase::Compiler,
                       "E-COMP-201",
                       0,
                       1,
                       "Backend '" + backendName(options.backend) + "' aun no esta implementado en esta version.",
                       "Usa --backend cpp por ahora. Roadmap: AST tipado -> LLVM IR -> AOT/JIT.");
    }

    const int optLevel = normalizeOptLevel(options.optLevel);
    const std::string gxxOpt = gxxOptFlag(optLevel);
    const std::string msvcOpt = msvcOptFlags(optLevel);

    printStep(term::kYellow,
              "+",
              "Iniciando compilacion AOT de: " + sourceLabel + " (backend=" + backendName(options.backend) +
                  ", opt=O" + std::to_string(optLevel) + ")");

    const std::filesystem::path outExe = std::filesystem::absolute(outputExePath);
    const std::filesystem::path outDir = outExe.parent_path();
    if (!outDir.empty()) std::filesystem::create_directories(outDir);

    const std::filesystem::path cppPath = outExe.parent_path() / (outExe.stem().string() + ".aot.cpp");
    const std::filesystem::path logPath = outExe.parent_path() / (outExe.stem().string() + ".build.log");
    const std::filesystem::path projectRoot = resolveProjectRoot(sourceLabel);
    const std::filesystem::path coreDir = projectRoot / "src" / "core";
    const std::filesystem::path includeDir = resolveNativeIncludeDir(projectRoot);
    const std::filesystem::path lexerCpp = coreDir / "lexer.cpp";
    const std::filesystem::path parserCpp = coreDir / "parser.cpp";
    const std::filesystem::path runtimeCpp = coreDir / "runtime.cpp";

    AstEmitter emitter;
    const std::string cppCode = emitter.emitProgram(program, sourceLabel);
    printStep(term::kYellow, "+", "Generando archivo C++ intermedio: " + cppPath.string());
    {
        std::ofstream out(cppPath, std::ios::binary);
        if (!out) {
            throw EppError(ErrorPhase::Compiler,
                           "E-COMP-010",
                           0,
                           1,
                           "No se pudo crear archivo temporal AOT: " + cppPath.string());
        }
        out << cppCode;
    }

    int rc = 0;
#ifdef _WIN32
    {
        std::ostringstream cmd;
        cmd << "cl /nologo /std:c++20 " << msvcOpt << " /GL /Gw /Gy /DNDEBUG /EHsc /MT "
            << "/I \"" << coreDir.string() << "\" "
            << "/I \"" << includeDir.string() << "\" "
            << "\"" << cppPath.string() << "\" "
            << "\"" << lexerCpp.string() << "\" "
            << "\"" << parserCpp.string() << "\" "
            << "\"" << runtimeCpp.string() << "\" "
            << "/link /LTCG /OPT:REF /OPT:ICF ws2_32.lib /OUT:\"" << outExe.string() << "\" "
            << "> \"" << logPath.string() << "\" 2>&1";
        rc = runWithSpinner(cmd.str(),
                            "Compilando con MSVC",
                            "MSVC completo.",
                            "MSVC fallo. Intentando g++...");
    }

    if (rc != 0) {
        std::ostringstream cmd;
        cmd << "g++ -std=c++17 " << gxxOpt << " -DNDEBUG -s "
            << "-I \"" << coreDir.string() << "\" "
            << "-I \"" << includeDir.string() << "\" "
            << "\"" << cppPath.string() << "\" "
            << "\"" << lexerCpp.string() << "\" "
            << "\"" << parserCpp.string() << "\" "
            << "\"" << runtimeCpp.string() << "\" "
            << "-lws2_32 "
            << "-o \"" << outExe.string() << "\" "
            << "> \"" << logPath.string() << "\" 2>&1";
        rc = runWithSpinner(cmd.str(),
                            "Compilando con g++",
                            "g++ completo.",
                            "g++ fallo. Intentando CMake + MSBuild...");
    }

    if (rc != 0) {
        const std::filesystem::path cmakeDir = outExe.parent_path() / (outExe.stem().string() + ".aot_build");
        const std::filesystem::path cmakeLists = cmakeDir / "CMakeLists.txt";
        const std::filesystem::path cmakeBuild = cmakeDir / "build";

        std::filesystem::create_directories(cmakeDir);

        const std::string lexerCppAbs = std::filesystem::absolute(lexerCpp).string();
        const std::string parserCppAbs = std::filesystem::absolute(parserCpp).string();
        const std::string runtimeCppAbs = std::filesystem::absolute(runtimeCpp).string();
        const std::string srcDirAbs = std::filesystem::absolute(coreDir).string();
        const std::string includeDirAbs = std::filesystem::absolute(includeDir).string();

        std::ofstream cm(cmakeLists, std::ios::binary);
        if (!cm) {
            throw EppError(ErrorPhase::Compiler,
                           "E-COMP-011",
                           0,
                           1,
                           "No se pudo crear CMakeLists temporal: " + cmakeLists.string());
        }
        cm << "cmake_minimum_required(VERSION 3.16)\n";
        cm << "project(epp_aot LANGUAGES CXX)\n";
        cm << "set(CMAKE_CXX_STANDARD 17)\n";
        cm << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n";
        cm << "set(CMAKE_CXX_EXTENSIONS OFF)\n";
        cm << "add_executable(epp_aot \""
           << escapeCpp(cppPath.string()) << "\" \""
           << escapeCpp(lexerCppAbs) << "\" \""
           << escapeCpp(parserCppAbs) << "\" \""
           << escapeCpp(runtimeCppAbs) << "\")\n";
        cm << "target_include_directories(epp_aot PRIVATE \"" << escapeCpp(srcDirAbs)
           << "\" \"" << escapeCpp(includeDirAbs) << "\")\n";
        cm << "if(MSVC)\n";
        cm << "  target_compile_options(epp_aot PRIVATE " << msvcOpt << " /GL /Gw /Gy /DNDEBUG /MT)\n";
        cm << "  target_link_options(epp_aot PRIVATE /LTCG /OPT:REF /OPT:ICF)\n";
        cm << "endif()\n";
        cm << "if(WIN32)\n";
        cm << "  target_link_libraries(epp_aot PRIVATE ws2_32)\n";
        cm << "endif()\n";
        cm << "set_target_properties(epp_aot PROPERTIES OUTPUT_NAME \"" << escapeCpp(outExe.stem().string()) << "\")\n";
        cm << "set_target_properties(epp_aot PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE \"" << escapeCpp(outExe.parent_path().string()) << "\")\n";
        cm.close();

        std::ostringstream cmakeCmd1;
        cmakeCmd1 << "cmake -S \"" << cmakeDir.string() << "\" -B \"" << cmakeBuild.string()
                  << "\" -G \"Visual Studio 17 2022\" -A x64 "
                  << "> \"" << logPath.string() << "\" 2>&1";
        rc = runWithSpinner(cmakeCmd1.str(),
                            "Configurando build temporal (CMake)",
                            "CMake configurado.",
                            "CMake fallo.");
        if (rc == 0) {
            std::ostringstream cmakeCmd2;
            cmakeCmd2 << "cmake --build \"" << cmakeBuild.string()
                      << "\" --config Release --target epp_aot "
                      << ">> \"" << logPath.string() << "\" 2>&1";
            rc = runWithSpinner(cmakeCmd2.str(),
                                "Compilando proyecto temporal (MSBuild)",
                                "MSBuild completo.",
                                "MSBuild fallo.");
        }
    }
#else
    std::ostringstream cmd;
    cmd << "g++ -std=c++17 " << gxxOpt << " -DNDEBUG -s "
        << "-I \"" << coreDir.string() << "\" "
        << "-I \"" << includeDir.string() << "\" "
        << "\"" << cppPath.string() << "\" "
        << "\"" << lexerCpp.string() << "\" "
        << "\"" << parserCpp.string() << "\" "
        << "\"" << runtimeCpp.string() << "\" "
        << "-o \"" << outExe.string() << "\" "
        << "> \"" << logPath.string() << "\" 2>&1";
    rc = runWithSpinner(cmd.str(), "Compilando con g++", "g++ completo.", "g++ fallo.");
#endif

    if (rc != 0) {
        std::ifstream logIn(logPath, std::ios::binary);
        std::stringstream logBuffer;
        if (logIn) logBuffer << logIn.rdbuf();
        throw EppError(ErrorPhase::Compiler,
                       "E-COMP-100",
                       0,
                       1,
                       "Fallo la compilacion AOT. Verifica que el compilador C++ este en PATH.\n"
                       "Comando genero log en: " +
                           logPath.string() + "\n" + logBuffer.str(),
                       "Instala Build Tools (MSVC) o un g++ moderno y vuelve a ejecutar.");
    }

    printStep(term::kGreen, "+", "Compilacion finalizada: " + outExe.string());
}
