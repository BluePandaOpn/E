#pragma once

#include "token.hpp"

#include <memory>
#include <vector>

struct Expr;
struct Stmt;

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

struct Expr {
    virtual ~Expr() = default;
    int line = 1;
    enum class Kind {
        Literal,
        Variable,
        Assign,
        Unary,
        Binary,
        ListLiteral,
        Call,
        Get,
        Set,
        Index,
        IndexSet
    };
    virtual Kind kind() const = 0;
};

struct LiteralExpr : Expr {
    Token valueToken;
    Kind kind() const override { return Kind::Literal; }
};

struct VariableExpr : Expr {
    Token name;
    Kind kind() const override { return Kind::Variable; }
};

struct AssignExpr : Expr {
    Token name;
    ExprPtr value;
    Kind kind() const override { return Kind::Assign; }
};

struct UnaryExpr : Expr {
    Token op;
    ExprPtr right;
    Kind kind() const override { return Kind::Unary; }
};

struct BinaryExpr : Expr {
    ExprPtr left;
    Token op;
    ExprPtr right;
    Kind kind() const override { return Kind::Binary; }
};

struct ListLiteralExpr : Expr {
    std::vector<ExprPtr> elements;
    Kind kind() const override { return Kind::ListLiteral; }
};

struct CallExpr : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> args;
    Kind kind() const override { return Kind::Call; }
};

struct GetExpr : Expr {
    ExprPtr object;
    Token name;
    Kind kind() const override { return Kind::Get; }
};

struct SetExpr : Expr {
    ExprPtr object;
    Token name;
    ExprPtr value;
    Kind kind() const override { return Kind::Set; }
};

struct IndexExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    Kind kind() const override { return Kind::Index; }
};

struct IndexSetExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    ExprPtr value;
    Kind kind() const override { return Kind::IndexSet; }
};

struct Stmt {
    virtual ~Stmt() = default;
    int line = 1;
    enum class Kind {
        Expr,
        Print,
        VarDecl,
        Block,
        If,
        While,
        Return,
        Import,
        FromImport,
        PackageDecl,
        FuncDecl,
        ClassDecl
    };
    virtual Kind kind() const = 0;
};

struct ExprStmt : Stmt {
    ExprPtr expr;
    Kind kind() const override { return Kind::Expr; }
};

struct PrintStmt : Stmt {
    ExprPtr expr;
    Kind kind() const override { return Kind::Print; }
};

struct VarDeclStmt : Stmt {
    Token name;
    ExprPtr initializer;
    Kind kind() const override { return Kind::VarDecl; }
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;
    Kind kind() const override { return Kind::Block; }
};

struct IfStmt : Stmt {
    ExprPtr condition;
    std::shared_ptr<BlockStmt> thenBranch;
    std::shared_ptr<BlockStmt> elseBranch;
    Kind kind() const override { return Kind::If; }
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    std::shared_ptr<BlockStmt> body;
    Kind kind() const override { return Kind::While; }
};

struct ReturnStmt : Stmt {
    ExprPtr value;
    Kind kind() const override { return Kind::Return; }
};

struct ImportStmt : Stmt {
    Token module;
    Kind kind() const override { return Kind::Import; }
};

struct FromImportStmt : Stmt {
    Token module;
    std::vector<Token> names;
    bool importAll = false;
    Kind kind() const override { return Kind::FromImport; }
};

struct PackageDeclStmt : Stmt {
    Token id;
    Kind kind() const override { return Kind::PackageDecl; }
};

struct FuncDeclStmt : Stmt {
    Token name;
    std::vector<Token> params;
    std::shared_ptr<BlockStmt> body;
    Kind kind() const override { return Kind::FuncDecl; }
};

struct ClassDeclStmt : Stmt {
    Token name;
    std::vector<std::shared_ptr<FuncDeclStmt>> methods;
    Kind kind() const override { return Kind::ClassDecl; }
};
