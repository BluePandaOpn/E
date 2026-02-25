#pragma once

#include "ast.hpp"

#include <string>
#include <vector>

enum class AotBackend {
    CppAot,
    LlvmAot,
    LlvmJit,
};

struct AotCompileOptions {
    AotBackend backend = AotBackend::CppAot;
    int optLevel = 3;
};

class AotCompiler {
public:
    void compileProgram(const std::vector<StmtPtr>& program,
                        const std::string& outputExePath,
                        const std::string& sourceLabel,
                        const AotCompileOptions& options = {}) const;
};
