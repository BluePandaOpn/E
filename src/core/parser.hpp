#pragma once

#include "ast.hpp"
#include "token.hpp"

#include <vector>

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::vector<StmtPtr> parse();

private:
    StmtPtr declaration();
    StmtPtr importDeclaration();
    StmtPtr fromImportDeclaration();
    StmtPtr packageDeclaration();
    StmtPtr varDeclaration();
    StmtPtr funcDeclaration();
    StmtPtr classDeclaration();
    StmtPtr statement();
    StmtPtr ifStatement();
    StmtPtr whileStatement();
    StmtPtr returnStatement();
    StmtPtr expressionStatement();
    std::shared_ptr<BlockStmt> parseBlockUntilEndOrElse(bool* hitElse = nullptr);
    std::shared_ptr<BlockStmt> parseBraceBlock();

    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr logicOr();
    ExprPtr logicAnd();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr unary();
    ExprPtr call();
    ExprPtr primary();

    bool match(std::initializer_list<TokenType> types);
    bool check(TokenType type) const;
    const Token& advance();
    bool isAtEnd() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& consume(TokenType type, const std::string& message);
    void skipSeparators();
    Token parseModuleToken(const std::string& message);

    const std::vector<Token>& tokens_;
    int current_ = 0;
};
