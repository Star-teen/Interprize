#ifndef INTERPRETER_H
#define INTERPRETER_H
// ============================================================
// interpreter.h — стековая виртуальная машина
//
// Выполняет программу в ПОЛИЗ (вектор PolizOp).
//
// КАК РАБОТАЕТ:
//   pc (program counter) — индекс текущей инструкции.
//   st — стек значений Val.
//   Инструкции читаются одна за другой.
//   JMP/JZ/JNZ изменяют pc (прыжки).
//
// АРИФМЕТИКА С ТИПАМИ (Таблица №2 из ТЗ, без boolean):
//   int op int       → int
//   real op real     → real
//   real op int      → real   (int автоматически приводится к real)
//   int op real      → real
//   string + string  → string (конкатенация)
//
// ОБЫЧНЫЕ ЛОГИЧЕСКИЕ ОПЕРАЦИИ (вариант VI.2):
//   AND: снять R и L; (L≠0 && R≠0) ? 1 : 0
//   OR:  снять R и L; (L≠0 || R≠0) ? 1 : 0
//   Оба операнда уже на стеке — парсер обеспечил их вычисление.
//
// ДЕЛЕНИЕ НА НОЛЬ:
//   Проверяем делитель перед DIV и MOD.
//   Если 0 → throw runtime_error.
// ============================================================
#include "poliz.h"
#include "symtable.h"
#include <vector>
#include <stack>
#include <iostream>
#include <stdexcept>

class Interpreter {
    const std::vector<PolizOp>& code;   // программа в ПОЛИЗ
    SymTable&                   sym;    // таблица переменных
    std::stack<Val>             st;     // рабочий стек

    Val pop() {
        if (st.empty())
            throw std::runtime_error("Внутренняя ошибка: стек пуст");
        Val v = st.top(); st.pop(); return v;
    }
    void push(Val v) { st.push(v); }

    // Привести Val к "логическому": 0 — false, всё остальное — true
    long long toBool(const Val& v) {
        if (std::holds_alternative<long long>(v)) return asInt(v) != 0 ? 1LL : 0LL;
        if (std::holds_alternative<double>(v))    return asReal(v) != 0.0 ? 1LL : 0LL;
        return asStr(v).empty() ? 0LL : 1LL;
    }

    // Выполнить арифметическую операцию над двумя операндами
    Val arith(const Val& L, const Val& R, OpCode op) {
        // Строковая конкатенация
        if (op == OpCode::ADD &&
            std::holds_alternative<std::string>(L) &&
            std::holds_alternative<std::string>(R))
            return asStr(L) + asStr(R);

        // Если хотя бы один операнд вещественный → результат вещественный
        bool anyReal = std::holds_alternative<double>(L) ||
                       std::holds_alternative<double>(R);
        if (anyReal) {
            double l = toReal(L), r = toReal(R);
            switch (op) {
                case OpCode::ADD: return l + r;
                case OpCode::SUB: return l - r;
                case OpCode::MUL: return l * r;
                case OpCode::DIV:
                    if (r == 0.0) throw std::runtime_error("Деление на ноль");
                    return l / r;
                default:
                    throw std::runtime_error("% неприменим к вещественным числам");
            }
        }
        // Оба операнда целые
        long long l = asInt(L), r = asInt(R);
        switch (op) {
            case OpCode::ADD: return l + r;
            case OpCode::SUB: return l - r;
            case OpCode::MUL: return l * r;
            case OpCode::DIV:
                if (r == 0) throw std::runtime_error("Деление на ноль");
                return l / r;
            default:
                throw std::runtime_error("Неизвестная арифметика");
        }
    }

    // Выполнить операцию сравнения
    Val compare(const Val& L, const Val& R, OpCode op) {
        long long res = 0;
        if (std::holds_alternative<std::string>(L)) {
            // Строковое сравнение — лексикографическое
            std::string l = asStr(L), r = asStr(R);
            switch (op) {
                case OpCode::LT:  res = l <  r; break;
                case OpCode::GT:  res = l >  r; break;
                case OpCode::LE:  res = l <= r; break;
                case OpCode::GE:  res = l >= r; break;
                case OpCode::EQ:  res = l == r; break;
                case OpCode::NEQ: res = l != r; break;
                default: throw std::runtime_error("Неизвестное сравнение");
            }
        } else {
            // Числовое сравнение — приводим к double
            double l = toReal(L), r = toReal(R);
            switch (op) {
                case OpCode::LT:  res = l <  r; break;
                case OpCode::GT:  res = l >  r; break;
                case OpCode::LE:  res = l <= r; break;
                case OpCode::GE:  res = l >= r; break;
                case OpCode::EQ:  res = l == r; break;
                case OpCode::NEQ: res = l != r; break;
                default: throw std::runtime_error("Неизвестное сравнение");
            }
        }
        return res;
    }

public:
    Interpreter(const std::vector<PolizOp>& c, SymTable& s)
        : code(c), sym(s) {}

    void run() {
        int pc = 0;

        while (pc < static_cast<int>(code.size())) {
            const PolizOp& op = code[pc];

            switch (op.code) {

                case OpCode::PUSH_INT:  push(op.ival); break;
                case OpCode::PUSH_REAL: push(op.rval); break;
                case OpCode::PUSH_STR:  push(op.sval); break;

                case OpCode::LOAD:
                    push(sym.value(op.sval));
                    break;

                case OpCode::STORE: {
                    // Присваивание: значение ОСТАЁТСЯ на стеке
                    // (как в C: результат a = expr равен expr)
                    Val v = st.top();
                    sym.set(op.sval, v);
                    break;
                }

                case OpCode::POP:
                    pop();
                    break;

                case OpCode::ADD:
                case OpCode::SUB:
                case OpCode::MUL:
                case OpCode::DIV:

                case OpCode::NEG: {
                    Val v = pop();
                    if      (std::holds_alternative<long long>(v)) push(-asInt(v));
                    else if (std::holds_alternative<double>(v))    push(-asReal(v));
                    else throw std::runtime_error("Унарный минус неприменим к строке");
                    break;
                }

                case OpCode::LT: case OpCode::GT: case OpCode::LE:
                case OpCode::GE: case OpCode::EQ: case OpCode::NEQ: {
                    Val R = pop(); Val L = pop();
                    push(compare(L, R, op.code));
                    break;
                }

                // Обычные вычисления (вариант VI.2):
                // Оба операнда уже на стеке — просто применяем операцию
                case OpCode::AND: {
                    Val R = pop(); Val L = pop();
                    push((toBool(L) != 0 && toBool(R) != 0) ? 1LL : 0LL);
                    break;
                }
                case OpCode::OR: {
                    Val R = pop(); Val L = pop();
                    push((toBool(L) != 0 || toBool(R) != 0) ? 1LL : 0LL);
                    break;
                }
                case OpCode::NOT: {
                    Val v = pop();
                    push(toBool(v) == 0 ? 1LL : 0LL);
                    break;
                }

                case OpCode::JMP:
                    pc = static_cast<int>(op.ival);
                    continue;   // не делаем pc++ в конце цикла

                case OpCode::JZ: {
                    Val v = pop();
                    if (toBool(v) == 0) { pc = static_cast<int>(op.ival); continue; }
                    break;
                }

                case OpCode::JNZ: {
                    // Используется в for: прыгаем если I > limit (результат GT != 0)
                    Val v = pop();
                    if (toBool(v) != 0) { pc = static_cast<int>(op.ival); continue; }
                    break;
                }

                case OpCode::READ: {
                    auto& e = sym.get(op.sval);
                    if (e.type == VType::INT) {
                        long long v; std::cin >> v; sym.set(op.sval, v);
                    } else if (e.type == VType::REAL) {
                        double v; std::cin >> v; sym.set(op.sval, v);
                    } else {
                        std::string v; std::cin >> v; sym.set(op.sval, v);
                    }
                    break;
                }

                case OpCode::WRITE: {
                    Val v = pop();
                    std::cout << valToStr(v) << " ";
                    break;
                }

                default:
                    throw std::runtime_error(
                        "Неизвестный опкод: " +
                        std::to_string(static_cast<int>(op.code)));
            }
            pc++;
        }
        std::cout << std::endl;
    }
};
#endif // INTERPRETER_H
