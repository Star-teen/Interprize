#ifndef INTERPRETER_H
#define INTERPRETER_H
//   int op int       → int
//   real op real     → real
//   real op int      → real   (int is automatically promoted to real)
//   int op real      → real
//   string + string  → string (concatenation)
//   AND: pop R and L; (L && R ? 1 : 0)
//   OR:  pop R and L; (L || R) ? 1 : 0
#include "poliz.h"
#include "symtable.h"
#include <vector>
#include <stack>
#include <iostream>
#include <stdexcept>
#include <climits>

class Interpreter {
    const std::vector<PolizOp>& code;   // POLIZ program
    SymTable& sym;    // symbol table
    std::stack<Val> st;     // operand stack

    Val pop() {
        if (st.empty()) throw std::runtime_error("Error: stack is empty");
        Val v = st.top();
        st.pop(); 
        return v;
    }
    void push(Val v) { st.push(v); }

    // Convert Val to boolean: 0 — false, everything else — true
    int toBool(const Val& v) {
        if (std::holds_alternative<int>(v)) return asInt(v) != 0 ? 1 : 0;
        if (std::holds_alternative<double>(v))    return asReal(v) != 0.0 ? 1 : 0;
        return asStr(v).empty() ? 0 : 1;
    }

    // Helper functions for overflow checking
    bool willOverflowAdd(int a, int b) {
        return (b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b);
    }

    bool willOverflowSub(int a, int b) {
        return (b > 0 && a < INT_MIN + b) || (b < 0 && a > INT_MAX + b);
    }

    bool willOverflowMul(int a, int b) {
        if (a == 0 || b == 0) return false;
        if (a > 0 && b > 0 && a > INT_MAX / b) return true;
        if (a < 0 && b < 0 && a < INT_MAX / b) return true;
        if (a > 0 && b < 0 && b < INT_MIN / a) return true;
        if (a < 0 && b > 0 && a < INT_MIN / b) return true;
        return false;
    }

    // Perform arithmetic operation
    Val ariphOp(const Val& L, const Val& R, OpCode op) {
        // String concatenation
        if (op == OpCode::ADD && std::holds_alternative<std::string>(L) && std::holds_alternative<std::string>(R))
            return asStr(L) + asStr(R);

        // If at least one operand is real → result is real
        bool anyReal = std::holds_alternative<double>(L) || std::holds_alternative<double>(R);
        if (anyReal) {
            double l = toReal(L), r = toReal(R);
            switch (op) {
                case OpCode::ADD: return l + r;
                case OpCode::SUB: return l - r;
                case OpCode::MUL: return l * r;
                case OpCode::DIV:
                    if (r == 0.0) throw std::runtime_error("Division by zero");
                    return l / r;
                default: throw std::runtime_error("Invalid operation");
            }
        }
        // Both operands are integers
        int l = asInt(L), r = asInt(R);
        switch (op) {
            case OpCode::ADD:
                if (willOverflowAdd(l, r))
                    throw std::runtime_error("Integer overflow in addition");
                return l + r;
            case OpCode::SUB:
                if (willOverflowSub(l, r))
                    throw std::runtime_error("Integer overflow in subtraction");
                return l - r;
            case OpCode::MUL:
                if (willOverflowMul(l, r))
                    throw std::runtime_error("Integer overflow in multiplication");
                return l * r;
            case OpCode::DIV:
                if (r == 0) throw std::runtime_error("Division by zero");
                if (l == INT_MIN && r == -1)
                    throw std::runtime_error("Integer overflow in division");
                return l / r;
            default:
                throw std::runtime_error("Unknown operation");
        }
    }

    // Perform comparison operation
    Val compare(const Val& L, const Val& R, OpCode op) {
        int res = 0;
        
        // String comparison — ONLY if both operands are strings
        if (std::holds_alternative<std::string>(L) && std::holds_alternative<std::string>(R)) {
            std::string l = asStr(L), r = asStr(R);
            switch (op) {
                case OpCode::LT:  res = l <  r ? 1 : 0; break;
                case OpCode::GT:  res = l >  r ? 1 : 0; break;
                case OpCode::LE:  res = l <= r ? 1 : 0; break;
                case OpCode::GE:  res = l >= r ? 1 : 0; break;
                case OpCode::EQ:  res = l == r ? 1 : 0; break;
                case OpCode::NEQ: res = l != r ? 1 : 0; break;
                default: throw std::runtime_error("Unknown comparison");
            }
            return res;
        }
        
        if ((std::holds_alternative<int>(L) || std::holds_alternative<double>(L)) &&
            (std::holds_alternative<int>(R) || std::holds_alternative<double>(R))) {
            double l = toReal(L), r = toReal(R);
            switch (op) {
                case OpCode::LT:  res = l <  r ? 1 : 0; break;
                case OpCode::GT:  res = l >  r ? 1 : 0; break;
                case OpCode::LE:  res = l <= r ? 1 : 0; break;
                case OpCode::GE:  res = l >= r ? 1 : 0; break;
                case OpCode::EQ:  res = l == r ? 1 : 0; break;
                case OpCode::NEQ: res = l != r ? 1 : 0; break;
                default: throw std::runtime_error("Unknown comparison");
            }
            return res;
        }
        
        // Incompatible types (e.g., string and int)
        throw std::runtime_error("Incompatible types in comparison operation");
    }

public:
    Interpreter(const std::vector<PolizOp>& c, SymTable& s): code(c), sym(s) {}

    void run() {
        size_t pc = 0;
        int steps = 0;
        const int MAX_STEPS = 100000;

        while (pc < code.size()) {
            steps++;
            if (steps > MAX_STEPS) throw std::runtime_error("Maximum step count exceeded (possible infinite loop)");

            const PolizOp& op = code[pc];
                        
            switch (op.code) {

                case OpCode::PUSH_INT: push(static_cast<int>(op.ival)); break;
                case OpCode::PUSH_REAL: push(op.rval); break;
                case OpCode::PUSH_STR: push(op.sval); break;

                case OpCode::LOAD: push(sym.value(op.sval)); break;
                case OpCode::STORE: {Val v = st.top(); sym.set(op.sval, v); break;}
                case OpCode::POP: pop(); break;

                case OpCode::ADD:
                case OpCode::SUB:
                case OpCode::MUL:
                case OpCode::DIV: {
                    Val R = pop();
                    Val L = pop();
                    push(ariphOp(L, R, op.code));
                    break;
                }

                case OpCode::NEG: {
                    Val v = pop();
                    if (std::holds_alternative<int>(v)) push(-asInt(v));
                    else if (std::holds_alternative<double>(v)) push(-asReal(v));
                    else throw std::runtime_error("Unary minus not applicable to string");
                    break;
                }

                case OpCode::LT: case OpCode::GT: case OpCode::LE:
                case OpCode::GE: case OpCode::EQ: case OpCode::NEQ: {
                    Val R = pop(); Val L = pop();
                    push(compare(L, R, op.code));
                    break;
                }

                case OpCode::AND: {
                    Val R = pop(); Val L = pop();
                    push((toBool(L) && toBool(R)) ? 1 : 0);
                    break;
                }
                case OpCode::OR: {
                    Val R = pop(); Val L = pop();
                    push((toBool(L) || toBool(R)) ? 1 : 0);
                    break;
                }
                case OpCode::NOT: {
                    Val v = pop();
                    push(toBool(v) == 0 ? 1 : 0);
                    break;
                }

                case OpCode::JMP:
                    pc = op.ival;
                    continue;   // do not increment pc at the end of the loop

                case OpCode::JZ: {
                    Val v = pop();
                    if (toBool(v) == 0) { pc = op.ival; continue; }
                    break;
                }

                case OpCode::JNZ: {
                    // Used in for: jump when I > limit
                    Val v = pop();
                    if (toBool(v) != 0) { pc = op.ival; continue; }
                    break;
                }

                case OpCode::READ: {
                    auto& e = sym.get(op.sval);
                    
                    if (e.type == VType::INT) {
                        int v;
                        if (!(std::cin >> v)) throw std::runtime_error("Input error: expected integer for " + op.sval);
                        sym.set(op.sval, v);
                        
                    } else if (e.type == VType::REAL) {
                        double v;
                        if (!(std::cin >> v)) throw std::runtime_error("Input error: expected real number for " + op.sval);
                        sym.set(op.sval, v);
                        
                    } else {  // STRING
                        std::string v;
                        std::cin >> v;  // read word (until whitespace)
                        sym.set(op.sval, v);
                    }
                    break;
                }
                
                case OpCode::WRITE: {
                    Val v = pop();
                    std::cout << valToStr(v) << " ";
                    break;
                }

                default: throw std::runtime_error("Unknown opcode: " + std::to_string(static_cast<int>(op.code)));
            }
            pc++;
        }
        std::cout << std::endl;
    }
};
#endif // INTERPRETER_H