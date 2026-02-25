#pragma once

#include "token.hpp"

#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> scanTokens();

private:
    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    void addToken(TokenType type, const std::string& lexeme = "");
    void scanToken();
    void scanString();
    void scanNumber();
    void scanIdentifier();
    static bool isAlpha(char c);
    static bool isAlphaNumeric(char c);

    std::string source_;
    std::vector<Token> tokens_;
    int start_ = 0;
    int current_ = 0;
    int startLine_ = 1;
    int startColumn_ = 1;
    int line_ = 1;
    int column_ = 1;
};
