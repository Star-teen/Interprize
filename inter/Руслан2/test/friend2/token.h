#pragma once
// ============================================================
// token.h — типы лексем и структура Token
//
// Токен (лексема) — минимальная смысловая единица языка.
// Лексер разбивает весь исходный текст на поток токенов.
//
// Примеры:
//   "for"   → Token{ KW_FOR,  "for",  строка }
//   "3.14"  → Token{ REAL_LIT,"3.14", строка }
//   ":"     → Token{ COLON,   ":",    строка }
//   "myVar" → Token{ ID,      "myVar",строка }
// ============================================================
#include <string>

enum class TT {
    // --- Литералы ---
    INT_LIT,    // целое число:       42
    REAL_LIT,   // вещественное:      3.14
    STR_LIT,    // строка:            "hello"

    // --- Идентификатор (имя переменной или метки) ---
    ID,

    // --- Ключевые слова: типы данных ---
    KW_INT,     // int
    KW_STRING,  // string
    KW_REAL,    // real

    // --- Ключевые слова: управление программой ---
    KW_PROGRAM, // program
    KW_IF,      // if
    KW_ELSE,    // else
    KW_WHILE,   // while
    KW_FOR,     // for      ← вариант II.3
    KW_STEP,    // step     ← вариант II.3
    KW_UNTIL,   // until    ← вариант II.3
    KW_DO,      // do       ← вариант II.3
    KW_GOTO,    // goto     ← вариант III.1
    KW_READ,    // read
    KW_WRITE,   // write

    // --- Ключевые слова: логика ---
    KW_AND,     // and
    KW_OR,      // or
    KW_NOT,     // not

    // --- Арифметические операторы ---
    PLUS,       // +
    MINUS,      // -
    STAR,       // *
    SLASH,      // /
    PERCENT,    // %

    // --- Операторы сравнения ---
    LT,         // <
    GT,         // >
    LE,         // <=
    GE,         // >=
    EQ,         // ==
    NEQ,        // !=

    // --- Присваивание ---
    ASSIGN,     // =   (одиночное, не ==)

    // --- Разделители ---
    LPAREN,     // (
    RPAREN,     // )
    LBRACE,     // {
    RBRACE,     // }
    SEMICOLON,  // ;
    COMMA,      // ,
    COLON,      // :   ← разделитель метки: "myLabel: оператор"

    // --- Конец файла ---
    EOF_TOK
};

// Структура одной лексемы
struct Token {
    TT          type;   // тип токена (из enum выше)
    std::string val;    // текстовое значение ("42", "myVar", "+", ...)
    int         line;   // номер строки в исходнике (для сообщений об ошибках)

    Token(TT t, std::string v, int ln)
        : type(t), val(std::move(v)), line(ln) {}
};
