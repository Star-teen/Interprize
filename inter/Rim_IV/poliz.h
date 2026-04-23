#ifndef POLIZ_H
#define POLIZ_H

#include <string>

enum class OpCode {
    // Push value onto stack
    PUSH_INT,   // stack ← ival (long long)
    PUSH_REAL,  // stack ← rval (double)
    PUSH_STR,   // stack ← sval (string)

    // Variable operations
    LOAD,       // stack ← value of variable sval
    STORE,      // variable sval ← top of stack

    // Arithmetic (pop 2 operands, push result)
    ADD,        // addition (string concatenation)
    SUB,        // subtraction
    MUL,        // multiplication
    DIV,        // division (checks division by zero)
    NEG,        // unary minus (pops 1 operand)

    // Comparisons (pop 2, push 0 or 1)
    LT,         // <
    GT,         // >
    LE,         // <=
    GE,         // >=
    EQ,         // ==
    NEQ,        // !=

    // Logic
    AND,        // pop R,L → (L != 0 && R != 0) ? 1 : 0
    OR,         // pop R,L → (L != 0 || R != 0) ? 1 : 0
    NOT,        // pop v   → (v == 0) ? 1 : 0

    // Jumps
    JMP,        // unconditional jump to index ival
    JZ,         // pop from stack; if 0 then jump to ival
    JNZ,        // pop from stack; if not 0 then jump to ival

    READ,       // read from cin into variable sval
    WRITE,      // pop from stack, output to cout

    POP         // pop top of stack (garbage from expression statements)
};

struct PolizOp {
    OpCode      code;
    long long   ival = 0;
    double      rval = 0.0;
    std::string sval;
};
#endif // POLIZ_H