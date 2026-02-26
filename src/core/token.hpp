#pragma once

#include <string>

enum class TokenType {
    // Single char
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Comma,
    Colon,
    Dot,
    Plus,
    Minus,
    Star,
    Slash,
    Semicolon,

    // One or two char
    Assign,
    EqualEqual,
    Bang,
    BangEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,

    // Literals
    Identifier,
    Number,
    String,

    // Keywords
    Var,
    Func,
    Return,
    If,
    Else,
    While,
    End,
    Class,
    This,
    Import,
    From,
    True,
    False,
    Null,
    And,
    Or,
    Not,
    At,

    Newline,
    Eof
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column = 1;
};
