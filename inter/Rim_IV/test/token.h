#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TT {
    INT_L,      // integer literal
    REAL_L,     // real literal
    STR_L,      // string literal
    ID,         // identifier
    LEX_INT,    // int
    LEX_STRING, // string
    LEX_REAL,   // real

    LEX_PROGRAM, // program
    LEX_IF,      // if
    LEX_ELSE,    // else
    LEX_WHILE,   // while
    LEX_FOR,     // for
    LEX_STEP,    // step
    LEX_UNTIL,   // until
    LEX_DO,      // do
    LEX_GOTO,    // goto
    LEX_READ,    // read
    LEX_WRITE,   // write

    LEX_AND,     // and
    LEX_OR,      // or
    LEX_NOT,     // not

    PLUS,       // +
    MINUS,      // -
    STAR,       // *
    SLASH,      // /

    LT,         // <
    GT,         // >
    LE,         // <=
    GE,         // >=
    EQ,         // ==
    NEQ,        // !=

    ASSIGN,     // = assignment

    LPAREN,     // (
    RPAREN,     // )
    LBRACE,     // {
    RBRACE,     // }
    SEMICOLON,  // ;
    COMMA,      // ,
    COLON,      // : label separator: "myLabel: statement"

    EOF_TOK
};

struct Token {
    TT          type;   // token type
    std::string val;    // textual value
    int         line;   // line number in source file

    Token(TT t, std::string v, int ln)
        : type(t), val(std::move(v)), line(ln) {}
};

#endif // TOKEN_H