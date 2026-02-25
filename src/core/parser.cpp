#include "parser.hpp"
#include "error.hpp"

#include <initializer_list>
#include <memory>
#include <string>

namespace {
std::string tokenTypeLabel(TokenType type) {
    switch (type) {
        case TokenType::LeftParen: return "'('";
        case TokenType::RightParen: return "')'";
        case TokenType::LeftBrace: return "'{'";
        case TokenType::RightBrace: return "'}'";
        case TokenType::Comma: return "','";
        case TokenType::Colon: return "':'";
        case TokenType::Dot: return "'.'";
        case TokenType::Semicolon: return "';'";
        case TokenType::Identifier: return "identificador";
        case TokenType::Number: return "numero";
        case TokenType::String: return "texto";
        case TokenType::Var: return "'var'";
        case TokenType::Func: return "'func'";
        case TokenType::If: return "'if'";
        case TokenType::Else: return "'else'";
        case TokenType::While: return "'while'";
        case TokenType::Return: return "'return'";
        case TokenType::End: return "'end'";
        case TokenType::Class: return "'class'";
        case TokenType::From: return "'from'";
        case TokenType::At: return "'@'";
        case TokenType::Newline: return "salto de linea";
        case TokenType::Eof: return "fin de archivo";
        default: return "token";
    }
}
}

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> statements;
    skipSeparators();
    while (!isAtEnd()) {
        statements.push_back(declaration());
        skipSeparators();
    }
    return statements;
}

StmtPtr Parser::declaration() {
    if (match({TokenType::Import})) return importDeclaration();
    if (match({TokenType::From})) return fromImportDeclaration();
    if (match({TokenType::At})) return packageDeclaration();
    if (match({TokenType::Var})) return varDeclaration();
    if (match({TokenType::Func})) return funcDeclaration();
    if (match({TokenType::Class})) return classDeclaration();
    return statement();
}

StmtPtr Parser::importDeclaration() {
    Token module = parseModuleToken("Se esperaba modulo en import.");
    auto stmt = std::make_shared<ImportStmt>();
    stmt->line = module.line;
    stmt->module = module;
    return stmt;
}

StmtPtr Parser::fromImportDeclaration() {
    Token module = parseModuleToken("Se esperaba modulo despues de 'from'.");
    consume(TokenType::Import, "Se esperaba 'import' despues del modulo.");

    auto stmt = std::make_shared<FromImportStmt>();
    stmt->line = module.line;
    stmt->module = module;

    if (match({TokenType::Star})) {
        stmt->importAll = true;
        return stmt;
    }

    do {
        const Token& name = consume(TokenType::Identifier, "Se esperaba identificador en from ... import ...");
        stmt->names.push_back(name);
    } while (match({TokenType::Comma}));

    if (stmt->names.empty()) {
        throw EppError(ErrorPhase::Parser,
                       "E-PARSE-001",
                       peek().line,
                       peek().column,
                       "Se esperaba al menos un simbolo en 'from ... import'.",
                       "Ejemplo: from paquete import funcion, Clase",
                       peek().lexeme);
    }

    return stmt;
}

StmtPtr Parser::packageDeclaration() {
    const Token& tag = consume(TokenType::Identifier, "Se esperaba directiva despues de '@'.");
    if (tag.lexeme != "package") {
        throw EppError(ErrorPhase::Parser,
                       "E-PARSE-001",
                       tag.line,
                       tag.column,
                       "Directiva '@" + tag.lexeme + "' no soportada.",
                       "Directiva valida: @package \"mi.paquete\"",
                       tag.lexeme);
    }

    Token id = parseModuleToken("Se esperaba id del paquete en @package.");
    auto stmt = std::make_shared<PackageDeclStmt>();
    stmt->line = id.line;
    stmt->id = id;
    return stmt;
}

StmtPtr Parser::varDeclaration() {
    Token name = consume(TokenType::Identifier, "Se esperaba nombre de variable.");
    ExprPtr init;
    if (match({TokenType::Assign})) init = expression();
    auto stmt = std::make_shared<VarDeclStmt>();
    stmt->line = name.line;
    stmt->name = name;
    stmt->initializer = init;
    return stmt;
}

StmtPtr Parser::funcDeclaration() {
    Token name = consume(TokenType::Identifier, "Se esperaba nombre de funcion.");
    consume(TokenType::LeftParen, "Se esperaba '(' despues del nombre.");
    std::vector<Token> params;
    if (!check(TokenType::RightParen)) {
        do {
            params.push_back(consume(TokenType::Identifier, "Parametro invalido."));
        } while (match({TokenType::Comma}));
    }
    consume(TokenType::RightParen, "Se esperaba ')'.");

    std::shared_ptr<BlockStmt> body;
    if (match({TokenType::LeftBrace})) {
        body = parseBraceBlock();
    } else {
        consume(TokenType::Colon, "Se esperaba '{' o ':' despues de la firma.");
        body = parseBlockUntilEndOrElse(nullptr);
        consume(TokenType::End, "Se esperaba 'end' para cerrar la funcion.");
    }

    auto stmt = std::make_shared<FuncDeclStmt>();
    stmt->line = name.line;
    stmt->name = name;
    stmt->params = std::move(params);
    stmt->body = body;
    return stmt;
}

StmtPtr Parser::classDeclaration() {
    Token name = consume(TokenType::Identifier, "Se esperaba nombre de clase.");
    consume(TokenType::LeftBrace, "Se esperaba '{' despues del nombre de clase.");

    std::vector<std::shared_ptr<FuncDeclStmt>> methods;
    skipSeparators();
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        consume(TokenType::Func, "En una clase solo se permiten metodos con 'func'.");
        auto methodStmt = std::dynamic_pointer_cast<FuncDeclStmt>(funcDeclaration());
        methods.push_back(methodStmt);
        skipSeparators();
    }
    consume(TokenType::RightBrace, "Se esperaba '}' al cerrar la clase.");

    auto stmt = std::make_shared<ClassDeclStmt>();
    stmt->line = name.line;
    stmt->name = name;
    stmt->methods = std::move(methods);
    return stmt;
}

StmtPtr Parser::statement() {
    if (match({TokenType::If})) return ifStatement();
    if (match({TokenType::While})) return whileStatement();
    if (match({TokenType::Return})) return returnStatement();
    if (match({TokenType::LeftBrace})) return parseBraceBlock();
    return expressionStatement();
}

StmtPtr Parser::ifStatement() {
    ExprPtr condition = expression();
    std::shared_ptr<BlockStmt> thenBranch;
    std::shared_ptr<BlockStmt> elseBranch;

    if (match({TokenType::LeftBrace})) {
        thenBranch = parseBraceBlock();
        skipSeparators();
        if (match({TokenType::Else})) {
            consume(TokenType::LeftBrace, "Se esperaba '{' despues de 'else'.");
            elseBranch = parseBraceBlock();
        }
    } else {
        consume(TokenType::Colon, "Se esperaba '{' o ':' despues de condicion.");
        bool hitElse = false;
        thenBranch = parseBlockUntilEndOrElse(&hitElse);
        if (hitElse) {
            consume(TokenType::Else, "Se esperaba 'else'.");
            consume(TokenType::Colon, "Se esperaba ':' despues de else.");
            elseBranch = parseBlockUntilEndOrElse(nullptr);
        }
        consume(TokenType::End, "Se esperaba 'end' para cerrar if.");
    }

    auto stmt = std::make_shared<IfStmt>();
    stmt->line = condition->line;
    stmt->condition = condition;
    stmt->thenBranch = thenBranch;
    stmt->elseBranch = elseBranch;
    return stmt;
}

StmtPtr Parser::whileStatement() {
    ExprPtr condition = expression();
    std::shared_ptr<BlockStmt> body;
    if (match({TokenType::LeftBrace})) {
        body = parseBraceBlock();
    } else {
        consume(TokenType::Colon, "Se esperaba '{' o ':' despues de condicion.");
        body = parseBlockUntilEndOrElse(nullptr);
        consume(TokenType::End, "Se esperaba 'end' para cerrar while.");
    }

    auto stmt = std::make_shared<WhileStmt>();
    stmt->line = condition->line;
    stmt->condition = condition;
    stmt->body = body;
    return stmt;
}

StmtPtr Parser::returnStatement() {
    auto stmt = std::make_shared<ReturnStmt>();
    stmt->line = previous().line;
    if (!check(TokenType::Newline) && !check(TokenType::Semicolon) && !check(TokenType::End) &&
        !check(TokenType::RightBrace) && !isAtEnd()) {
        stmt->value = expression();
    }
    return stmt;
}

StmtPtr Parser::expressionStatement() {
    ExprPtr expr = expression();
    auto stmt = std::make_shared<ExprStmt>();
    stmt->line = expr->line;
    stmt->expr = expr;
    return stmt;
}

std::shared_ptr<BlockStmt> Parser::parseBlockUntilEndOrElse(bool* hitElse) {
    auto block = std::make_shared<BlockStmt>();
    skipSeparators();
    while (!isAtEnd() && !check(TokenType::End) && !check(TokenType::Else)) {
        block->statements.push_back(declaration());
        skipSeparators();
    }
    if (hitElse) *hitElse = check(TokenType::Else);
    return block;
}

std::shared_ptr<BlockStmt> Parser::parseBraceBlock() {
    auto block = std::make_shared<BlockStmt>();
    skipSeparators();
    while (!isAtEnd() && !check(TokenType::RightBrace)) {
        block->statements.push_back(declaration());
        skipSeparators();
    }
    consume(TokenType::RightBrace, "Se esperaba '}' para cerrar bloque.");
    return block;
}

ExprPtr Parser::expression() { return assignment(); }

ExprPtr Parser::assignment() {
    ExprPtr expr = logicOr();
    if (match({TokenType::Assign})) {
        Token eq = previous();
        ExprPtr value = assignment();

        if (auto var = std::dynamic_pointer_cast<VariableExpr>(expr)) {
            auto assign = std::make_shared<AssignExpr>();
            assign->line = var->line;
            assign->name = var->name;
            assign->value = value;
            return assign;
        }
        if (auto getExpr = std::dynamic_pointer_cast<GetExpr>(expr)) {
            auto setExpr = std::make_shared<SetExpr>();
            setExpr->line = getExpr->line;
            setExpr->object = getExpr->object;
            setExpr->name = getExpr->name;
            setExpr->value = value;
            return setExpr;
        }

        throw EppError(ErrorPhase::Parser,
                       "E-PARSE-004",
                       eq.line,
                       eq.column,
                       "Asignacion invalida.",
                       "Solo se puede asignar a variables o propiedades (ej: x = 1, obj.campo = 2).",
                       eq.lexeme);
    }
    return expr;
}

ExprPtr Parser::logicOr() {
    ExprPtr expr = logicAnd();
    while (match({TokenType::Or})) {
        Token op = previous();
        ExprPtr right = logicAnd();
        auto bin = std::make_shared<BinaryExpr>();
        bin->line = op.line;
        bin->left = expr;
        bin->op = op;
        bin->right = right;
        expr = bin;
    }
    return expr;
}

ExprPtr Parser::logicAnd() {
    ExprPtr expr = equality();
    while (match({TokenType::And})) {
        Token op = previous();
        ExprPtr right = equality();
        auto bin = std::make_shared<BinaryExpr>();
        bin->line = op.line;
        bin->left = expr;
        bin->op = op;
        bin->right = right;
        expr = bin;
    }
    return expr;
}

ExprPtr Parser::equality() {
    ExprPtr expr = comparison();
    while (match({TokenType::EqualEqual, TokenType::BangEqual})) {
        Token op = previous();
        ExprPtr right = comparison();
        auto bin = std::make_shared<BinaryExpr>();
        bin->line = op.line;
        bin->left = expr;
        bin->op = op;
        bin->right = right;
        expr = bin;
    }
    return expr;
}

ExprPtr Parser::comparison() {
    ExprPtr expr = term();
    while (match({TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual})) {
        Token op = previous();
        ExprPtr right = term();
        auto bin = std::make_shared<BinaryExpr>();
        bin->line = op.line;
        bin->left = expr;
        bin->op = op;
        bin->right = right;
        expr = bin;
    }
    return expr;
}

ExprPtr Parser::term() {
    ExprPtr expr = factor();
    while (match({TokenType::Plus, TokenType::Minus})) {
        Token op = previous();
        ExprPtr right = factor();
        auto bin = std::make_shared<BinaryExpr>();
        bin->line = op.line;
        bin->left = expr;
        bin->op = op;
        bin->right = right;
        expr = bin;
    }
    return expr;
}

ExprPtr Parser::factor() {
    ExprPtr expr = unary();
    while (match({TokenType::Star, TokenType::Slash})) {
        Token op = previous();
        ExprPtr right = unary();
        auto bin = std::make_shared<BinaryExpr>();
        bin->line = op.line;
        bin->left = expr;
        bin->op = op;
        bin->right = right;
        expr = bin;
    }
    return expr;
}

ExprPtr Parser::unary() {
    if (match({TokenType::Bang, TokenType::Not, TokenType::Minus})) {
        Token op = previous();
        ExprPtr right = unary();
        auto un = std::make_shared<UnaryExpr>();
        un->line = op.line;
        un->op = op;
        un->right = right;
        return un;
    }
    return call();
}

ExprPtr Parser::call() {
    ExprPtr expr = primary();
    while (true) {
        if (match({TokenType::LeftParen})) {
            auto callExpr = std::make_shared<CallExpr>();
            callExpr->line = expr->line;
            callExpr->callee = expr;
            if (!check(TokenType::RightParen)) {
                do {
                    callExpr->args.push_back(expression());
                } while (match({TokenType::Comma}));
            }
            consume(TokenType::RightParen, "Se esperaba ')'.");
            expr = callExpr;
        } else if (match({TokenType::Dot})) {
            Token name = consume(TokenType::Identifier, "Se esperaba nombre de propiedad.");
            auto getExpr = std::make_shared<GetExpr>();
            getExpr->line = name.line;
            getExpr->object = expr;
            getExpr->name = name;
            expr = getExpr;
        } else {
            break;
        }
    }
    return expr;
}

ExprPtr Parser::primary() {
    if (match({TokenType::True, TokenType::False, TokenType::Null, TokenType::Number, TokenType::String})) {
        auto lit = std::make_shared<LiteralExpr>();
        lit->valueToken = previous();
        lit->line = previous().line;
        return lit;
    }
    if (match({TokenType::Identifier, TokenType::This})) {
        auto var = std::make_shared<VariableExpr>();
        var->name = previous();
        var->line = previous().line;
        return var;
    }
    if (match({TokenType::LeftParen})) {
        ExprPtr expr = expression();
        consume(TokenType::RightParen, "Se esperaba ')'.");
        return expr;
    }
    const Token& t = peek();
    throw EppError(ErrorPhase::Parser,
                   "E-PARSE-003",
                   t.line,
                   t.column,
                   "Expresion invalida.",
                   "Revisa parentesis, operadores y literales en la expresion.",
                   t.lexeme);
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return type == TokenType::Eof;
    return peek().type == type;
}

const Token& Parser::advance() {
    if (!isAtEnd()) ++current_;
    return previous();
}

bool Parser::isAtEnd() const { return peek().type == TokenType::Eof; }

const Token& Parser::peek() const { return tokens_[current_]; }

const Token& Parser::previous() const { return tokens_[current_ - 1]; }

const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    const Token& found = peek();
    throw EppError(ErrorPhase::Parser,
                   "E-PARSE-001",
                   found.line,
                   found.column,
                   message,
                   "Se esperaba " + tokenTypeLabel(type) + ", pero se encontro '" + found.lexeme + "'.",
                   found.lexeme);
}

Token Parser::parseModuleToken(const std::string& message) {
    if (match({TokenType::String})) return previous();
    if (!match({TokenType::Identifier})) {
        const Token& found = peek();
        throw EppError(ErrorPhase::Parser,
                       "E-PARSE-001",
                       found.line,
                       found.column,
                       message,
                       "Usa ruta entre comillas o id cualificado (ej: stdlib.asyncio).",
                       found.lexeme);
    }

    Token first = previous();
    std::string lexeme = first.lexeme;
    while (match({TokenType::Dot})) {
        const Token& segment = consume(TokenType::Identifier, "Se esperaba identificador despues de '.'.");
        lexeme += ".";
        lexeme += segment.lexeme;
    }

    Token out = first;
    out.lexeme = lexeme;
    return out;
}

void Parser::skipSeparators() {
    while (match({TokenType::Newline, TokenType::Semicolon})) {}
}
