#ifndef POLIZ_H
#define POLIZ_H

#include <string>

enum class OpCode {
    // Положить значение на стек
    PUSH_INT,   // стек ← ival (long long)
    PUSH_REAL,  // стек ← rval (double)
    PUSH_STR,   // стек ← sval (string)

    // Работа с переменными
    LOAD,       // стек ← значение переменной sval
    STORE,      // переменная sval ← вершина стека

    // (снимают 2 операнда, кладут результат)
    ADD,        // сложение (конкатенация строк)
    SUB,        // вычитание
    MUL,        // умножение
    DIV,        // деление (проверяем деление на 0)
    NEG,        // унарный минус (снимает 1 операнд)

    // Сравнения (снимают 2, кладут 0 или 1)
    LT,         // <
    GT,         // >
    LE,         // <=
    GE,         // >=
    EQ,         // ==
    NEQ,        // !=

    // Логика
    AND,        // снять R,L → (L≠0 && R≠0) ? 1 : 0
    OR,         // снять R,L → (L≠0 || R≠0) ? 1 : 0
    NOT,        // снять v   → (v==0) ? 1 : 0

    // Переходы
    JMP,        // безусловный переход к инструкции с индексом ival
    JZ,         // снять со стека; если 0 → перейти к ival
    JNZ,        // снять со стека; если НЕ 0 → перейти к ival

    READ,       // прочитать с cin в переменную sval
    WRITE,      // снять со стека, вывести в cout

    POP         // снять вершину стека (мусор от оператора-выражения)
};

struct PolizOp {
    OpCode      code;
    long long   ival = 0;    // операнд для PUSH_INT и адресов JMP/JZ/JNZ
    double      rval = 0.0;  // операнд для PUSH_REAL
    std::string sval;        // операнд для PUSH_STR, LOAD, STORE, READ
};
#endif // POLIZ_H