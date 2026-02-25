#pragma once

#include <stdexcept>
#include <string>

enum class ErrorPhase {
    Lexer,
    Parser,
    Runtime,
    Compiler,
    Internal,
};

inline const char* errorPhaseName(ErrorPhase phase) {
    switch (phase) {
        case ErrorPhase::Lexer: return "LEXER";
        case ErrorPhase::Parser: return "PARSER";
        case ErrorPhase::Runtime: return "RUNTIME";
        case ErrorPhase::Compiler: return "COMPILER";
        case ErrorPhase::Internal: return "INTERNAL";
    }
    return "UNKNOWN";
}

class EppError : public std::runtime_error {
public:
    EppError(int line, const std::string& message)
        : std::runtime_error(message),
          phase_(ErrorPhase::Internal),
          code_("E-INTERNAL-000"),
          line_(line),
          column_(1) {}

    EppError(ErrorPhase phase,
             const std::string& code,
             int line,
             int column,
             const std::string& message,
             const std::string& hint = "",
             const std::string& near = "")
        : std::runtime_error(message),
          phase_(phase),
          code_(code),
          line_(line),
          column_(column),
          hint_(hint),
          near_(near) {}

    ErrorPhase phase() const { return phase_; }
    const std::string& code() const { return code_; }
    int line() const { return line_; }
    int column() const { return column_; }
    const std::string& hint() const { return hint_; }
    const std::string& near() const { return near_; }

private:
    ErrorPhase phase_;
    std::string code_;
    int line_;
    int column_;
    std::string hint_;
    std::string near_;
};
