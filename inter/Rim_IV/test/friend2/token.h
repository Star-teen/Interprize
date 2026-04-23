#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TT {
    INT_L,    // целое число
    REAL_L,   // вещественное
    STR_L,    // строка
    ID,         // Идентификатор
    LEX_INT,     // int
    LEX_STRING,  // string
    LEX_REAL,    // real

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

    ASSIGN,     // = присвоить

    LPAREN,     // (
    RPAREN,     // )
    LBRACE,     // {
    RBRACE,     // }
    SEMICOLON,  // ;
    COMMA,      // ,
    COLON,      // :   ← разделитель метки: "myLabel: оператор"

    EOF_TOK
};

struct Token {
    TT          type;   // тип токена 
    std::string val;    // текстовое значение
    int         line;   // номер строки в исходнике

    Token(TT t, std::string v, int ln)
        : type(t), val(std::move(v)), line(ln) {}
};

#endif // TOKEN_H