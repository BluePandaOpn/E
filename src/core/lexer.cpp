#include "lexer.hpp"
#include "error.hpp"

#include <cctype>
#include <unordered_map>

namespace {
const std::unordered_map<std::string, TokenType> kKeywords = {
    {"var", TokenType::Var},     {"func", TokenType::Func},
    {"return", TokenType::Return},{"if", TokenType::If},
    {"else", TokenType::Else},   {"while", TokenType::While},
    {"end", TokenType::End},     {"class", TokenType::Class},
    {"this", TokenType::This},   {"import", TokenType::Import},
    {"from", TokenType::From},
    {"true", TokenType::True},   {"false", TokenType::False},
    {"null", TokenType::Null},   {"and", TokenType::And},
    {"or", TokenType::Or},       {"not", TokenType::Not},
};
}

Lexer::Lexer(const std::string& source) : source_(source) {}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        start_ = current_;
        startLine_ = line_;
        startColumn_ = column_;
        scanToken();
    }
    tokens_.push_back({TokenType::Eof, "", line_, column_});
    return tokens_;
}

bool Lexer::isAtEnd() const { return current_ >= static_cast<int>(source_.size()); }

char Lexer::advance() {
    const char c = source_[current_++];
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return c;
}

char Lexer::peek() const { return isAtEnd() ? '\0' : source_[current_]; }

char Lexer::peekNext() const {
    if (current_ + 1 >= static_cast<int>(source_.size())) return '\0';
    return source_[current_ + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[current_] != expected) return false;
    (void)advance();
    return true;
}

void Lexer::addToken(TokenType type, const std::string& lexeme) {
    if (!lexeme.empty()) {
        tokens_.push_back({type, lexeme, startLine_, startColumn_});
        return;
    }
    tokens_.push_back({type, source_.substr(start_, current_ - start_), startLine_, startColumn_});
}

void Lexer::scanToken() {
    const char c = advance();
    switch (c) {
        case '(': addToken(TokenType::LeftParen); break;
        case ')': addToken(TokenType::RightParen); break;
        case '{': addToken(TokenType::LeftBrace); break;
        case '}': addToken(TokenType::RightBrace); break;
        case ',': addToken(TokenType::Comma); break;
        case ':': addToken(TokenType::Colon); break;
        case '.': addToken(TokenType::Dot); break;
        case '+': addToken(TokenType::Plus); break;
        case '-': addToken(TokenType::Minus); break;
        case '*': addToken(TokenType::Star); break;
        case '@': addToken(TokenType::At); break;
        case ';': addToken(TokenType::Semicolon); break;
        case '/':
            if (match('/')) {
                while (peek() != '\n' && !isAtEnd()) advance();
            } else if (match('*')) {
                while (!isAtEnd()) {
                    if (peek() == '*' && peekNext() == '/') {
                        advance();
                        advance();
                        break;
                    }
                    advance();
                }
                if (isAtEnd() && !(current_ >= 2 && source_[current_ - 2] == '*' && source_[current_ - 1] == '/')) {
                    throw EppError(ErrorPhase::Lexer,
                                   "E-LEX-003",
                                   startLine_,
                                   startColumn_,
                                   "Comentario de bloque sin cerrar.",
                                   "Cierra el comentario con '*/'.");
                }
            } else {
                addToken(TokenType::Slash);
            }
            break;
        case '=': addToken(match('=') ? TokenType::EqualEqual : TokenType::Assign); break;
        case '!': addToken(match('=') ? TokenType::BangEqual : TokenType::Bang); break;
        case '>': addToken(match('=') ? TokenType::GreaterEqual : TokenType::Greater); break;
        case '<': addToken(match('=') ? TokenType::LessEqual : TokenType::Less); break;
        case '"': scanString(); break;
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            addToken(TokenType::Newline, "\n");
            break;
        default:
            if (std::isdigit(static_cast<unsigned char>(c))) {
                scanNumber();
            } else if (isAlpha(c)) {
                scanIdentifier();
            } else {
                throw EppError(ErrorPhase::Lexer,
                               "E-LEX-001",
                               startLine_,
                               startColumn_,
                               "Caracter inesperado: '" + std::string(1, c) + "'.",
                               "Revisa operadores, comillas o simbolos no validos para E++.");
            }
            break;
    }
}

void Lexer::scanString() {
    while (peek() != '"' && !isAtEnd()) {
        advance();
    }

    if (isAtEnd()) {
        throw EppError(ErrorPhase::Lexer,
                       "E-LEX-002",
                       startLine_,
                       startColumn_,
                       "Cadena sin cerrar.",
                       "Asegura cerrar la cadena con comillas dobles (\").");
    }
    advance();
    const std::string value = source_.substr(start_ + 1, current_ - start_ - 2);
    addToken(TokenType::String, value);
}

void Lexer::scanNumber() {
    while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
        advance();
        while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    }
    addToken(TokenType::Number);
}

void Lexer::scanIdentifier() {
    while (isAlphaNumeric(peek())) advance();
    const std::string text = source_.substr(start_, current_ - start_);
    const auto it = kKeywords.find(text);
    if (it != kKeywords.end()) {
        addToken(it->second, text);
    } else {
        addToken(TokenType::Identifier, text);
    }
}

bool Lexer::isAlpha(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool Lexer::isAlphaNumeric(char c) {
    return isAlpha(c) || std::isdigit(static_cast<unsigned char>(c));
}
