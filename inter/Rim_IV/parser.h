#ifndef PARSER_H
#define PARSER_H
#include "token.h"
#include "symtable.h"
#include "poliz.h"
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <string>

class Parser {
    std::vector<Token> toks;       // token stream from lexer
    size_t pos;                    // current position in token stream
    SymTable& sym;                 // symbol table
    std::vector<PolizOp>& code;    // POLIZ output

    int forCounter = 0; // unique suffix for for loop hidden variables

    // Label table: label name → POLIZ address
    std::unordered_map<std::string, int> labels;

    // Forward-goto: label name → list of JMP indices awaiting resolution
    std::unordered_map<std::string, std::vector<int>> fwdGotos;

    // Convenience methods
    Token& cur() { return toks[pos]; }

    // Look ahead at token without consuming it
    Token& peekAhead(int d = 1) {
        size_t idx = pos + d;
        if (idx >= toks.size()) idx = toks.size() - 1;
        return toks[idx];
    }

    bool check(TT t) { return cur().type == t; }

    Token eat(TT t) {
        if (!check(t))
            throw std::runtime_error("Line " + std::to_string(cur().line) + ": expected " + ttName(t) + ", found '" + cur().val + "'");
        return toks[pos++];
    }

    bool tryEat(TT t) { if (check(t)) { pos++; return true; } return false; }

    // Add POLIZ instruction, return its index
    int InterPol(PolizOp op) {
        code.push_back(op);
        return static_cast<int>(code.size()) - 1;
    }

    // Current index (where the next instruction will be written)
    int here() { return static_cast<int>(code.size()); }

    // Patch already written instruction with target address
    void patch(int instrIdx, int targetAddr) {code[instrIdx].ival = targetAddr;}

    std::string ttName(TT t) {
        switch (t) {
            case TT::SEMICOLON: return "';'";
            case TT::LPAREN:    return "'('";
            case TT::RPAREN:    return "')'";
            case TT::LBRACE:    return "'{'";
            case TT::RBRACE:    return "'}'";
            case TT::COLON:     return "':'";
            case TT::COMMA:     return "','";

            case TT::ID:        return "identifier";

            case TT::LEX_PROGRAM: return "'program'";
            case TT::LEX_READ:    return "'read'";
            case TT::LEX_WRITE:   return "'write'";
            case TT::LEX_STEP:   return "'step'";
            case TT::LEX_UNTIL:  return "'until'";
            case TT::LEX_DO:     return "'do'";
            case TT::LEX_WHILE:  return "'while'";
            default:            return "token";
        }
    }

    // Register a label: store address and patch all forward-gotos
    void registerLabel(const std::string& name) {
        if (labels.count(name))
            throw std::runtime_error("Label '" + name + "' declared twice");
        int addr = here();
        labels[name] = addr;
        // Patch all gotos that referenced this label
        auto it = fwdGotos.find(name);
        if (it != fwdGotos.end()) {
            for (int idx : it->second) patch(idx, addr);
            fwdGotos.erase(it);
        }
    }

    // Type and constant parsing
    VType parseType() {
        if (tryEat(TT::LEX_INT))    return VType::INT;
        if (tryEat(TT::LEX_STRING)) return VType::STRING;
        if (tryEat(TT::LEX_REAL))   return VType::REAL;
        throw std::runtime_error("Line " + std::to_string(cur().line) + ": expected type (int / string / real)");
    }

    Val parseConst(VType expectedType) {
        bool neg = false;
        if (check(TT::MINUS))     { pos++; neg = true; }
        else if (check(TT::PLUS)) { pos++; }

        if (check(TT::INT_L)) {
            long long v = std::stoll(eat(TT::INT_L).val) * (neg ? -1 : 1);
            if (expectedType == VType::REAL) return static_cast<double>(v);
            return v;
        }
        if (check(TT::REAL_L)) {
            double v = std::stod(eat(TT::REAL_L).val) * (neg ? -1.0 : 1.0);
            return v;
        }
        if (check(TT::STR_L)) {
            if (neg) throw std::runtime_error("Unary minus not applicable to string");
            return eat(TT::STR_L).val;
        }
        throw std::runtime_error("Line " + std::to_string(cur().line) + ": expected constant");
    }

    // 〈declaration〉 -> 〈type〉 〈name〉 [= 〈const〉] { , 〈name〉 [= 〈const〉] } ;
    void parseDecl() {
        VType t = parseType();
        do {
            std::string name = eat(TT::ID).val;
            if (tryEat(TT::ASSIGN)) {
                Val v = parseConst(t);
                sym.declareInit(name, t, v);
                // Generate POLIZ for initialization
                if (t == VType::INT) InterPol({ OpCode::PUSH_INT, asInt(v) });
                else if (t == VType::REAL) InterPol({ OpCode::PUSH_REAL, 0, asReal(v) });
                else InterPol({ OpCode::PUSH_STR, 0, 0.0, asStr(v) });
                InterPol({ OpCode::STORE, 0, 0.0, name });
                InterPol({ OpCode::POP });
            } else { sym.declare(name, t);}
        } while (tryEat(TT::COMMA));
        eat(TT::SEMICOLON);
    }
    
    // Expressions by precedence
    // Assignment: right-associative (a = b = 5 → a = (b = 5))
    void parseExpr() {
        if (check(TT::ID) && peekAhead().type == TT::ASSIGN) {
            std::string name = eat(TT::ID).val;
            sym.get(name);         // verify variable is declared
            eat(TT::ASSIGN);
            parseExpr();         // recursion (right associativity)
            InterPol({ OpCode::STORE, 0, 0.0, name });
        } else { parseOr();}
    }

    //   [left] [right] OR
    void parseOr() {
        parseAnd();
        while (check(TT::LEX_OR)) {
            pos++;
            parseAnd();          // right operand is ALWAYS evaluated
            InterPol({ OpCode::OR });
        }
    }

    //   [left] [right] AND
    void parseAnd() {
        parseCmp();
        while (check(TT::LEX_AND)) {
            pos++;
            parseCmp();          // right operand is ALWAYS evaluated
            InterPol({ OpCode::AND });
        }
    }

    // Comparison operators: < > <= >= == !=
    void parseCmp() {
        parseAdd();
        while (true) {
            OpCode op;
            if      (check(TT::LT))  op = OpCode::LT;
            else if (check(TT::GT))  op = OpCode::GT;
            else if (check(TT::LE))  op = OpCode::LE;
            else if (check(TT::GE))  op = OpCode::GE;
            else if (check(TT::EQ))  op = OpCode::EQ;
            else if (check(TT::NEQ)) op = OpCode::NEQ;
            else break;
            pos++;
            parseAdd();
            InterPol({ op });
        }
    }

    // Addition and subtraction: + -
    void parseAdd() {
        parseMul();
        while (check(TT::PLUS) || check(TT::MINUS)) {
            OpCode op = check(TT::PLUS) ? OpCode::ADD : OpCode::SUB;
            pos++;
            parseMul();
            InterPol({ op });
        }
    }

    // Multiplication, division: * /
    void parseMul() {
        parseUnary();
        while (check(TT::STAR) || check(TT::SLASH)) {
            OpCode op = check(TT::STAR) ? OpCode::MUL : OpCode::DIV;
            pos++;
            parseUnary();
            InterPol({ op });
        }
    }

    // Unary operators: not and - (unary minus)
    void parseUnary() {
        if (tryEat(TT::LEX_NOT)) {
            parseUnary();
            InterPol({ OpCode::NOT });
        } else if (check(TT::MINUS)) {
            pos++;
            parseUnary();
            InterPol({ OpCode::NEG });
        } else { parseSimple(); }
    }

    // Primary: literal, variable, (expression)
    void parseSimple() {
        if (check(TT::INT_L)) {
            InterPol({ OpCode::PUSH_INT, std::stoll(eat(TT::INT_L).val) });
        } else if (check(TT::REAL_L)) {
            InterPol({ OpCode::PUSH_REAL, 0, std::stod(eat(TT::REAL_L).val) });
        } else if (check(TT::STR_L)) {
            InterPol({ OpCode::PUSH_STR, 0, 0.0, eat(TT::STR_L).val });
        } else if (check(TT::ID)) {
            std::string name = eat(TT::ID).val;
            sym.get(name);   // verify variable is declared
            InterPol({ OpCode::LOAD, 0, 0.0, name });
        } else if (tryEat(TT::LPAREN)) {
            parseExpr();
            eat(TT::RPAREN);
        } else {
            throw std::runtime_error("Line " + std::to_string(cur().line) + ": unexpected token " + cur().val);
        }
    }

    // Statements

    void parseOper() {
        // Labeled statement: label : statement
        if (check(TT::ID) && peekAhead().type == TT::COLON) {
            std::string labelName = eat(TT::ID).val;
            eat(TT::COLON);
            registerLabel(labelName);   // store address, resolve forward-gotos
            parseOper();
            return;
        }

        // goto label ; 
        //   1. Label already declared (backward goto) → emit JMP with known address
        //   2. Label not yet encountered (forward goto) → emit JMP 0, remember for patching
        if (tryEat(TT::LEX_GOTO)) {
            std::string labelName = eat(TT::ID).val;
            eat(TT::SEMICOLON);
            if (labels.count(labelName)) {
                // Backward goto — address already known
                InterPol({ OpCode::JMP, static_cast<long long>(labels[labelName]) });
            } else {
                // Forward goto — address unknown, will patch later
                int idx = InterPol({ OpCode::JMP, 0 });
                fwdGotos[labelName].push_back(idx);
            }
            return;
        }

        // if ( expression ) statement [else statement]
        // POLIZ:
        //   [condition]
        //   JZ label_else   (if false, jump to else or after if)
        //   [if body]
        //   JMP label_end   (only if else exists)
        //   label_else:
        //   [else body]     (if present)
        //   label_end:
        if (tryEat(TT::LEX_IF)) {
            eat(TT::LPAREN);
            parseExpr();
            eat(TT::RPAREN);

            int jzIdx = InterPol({ OpCode::JZ, 0 });   // address to patch later

            parseOper();   // if body

            if (tryEat(TT::LEX_ELSE)) {
                int jmpIdx = InterPol({ OpCode::JMP, 0 });
                patch(jzIdx, here());     // JZ → beginning of else
                parseOper();              // else body
                patch(jmpIdx, here());    // JMP → after else
            } else {
                patch(jzIdx, here());     // JZ → after if (no else)
            }
            return;
        }

        // for I = E1 step E2 until E3 do S
        //   • E1, E2, E3 are evaluated ONCE before entering the loop
        //   • If E1 > E3 — body is not executed at all
        //   • I: E1, E1+E2, E1+2*E2, ... while I <= E3
        //   • After loop, I's value is undefined (per specification)
        // POLIZ:
        //   [E1] STORE I POP
        //   [E2] STORE __step_N POP
        //   [E3] STORE __limit_N POP
        //  check:
        //   LOAD I; LOAD __limit_N; GT; JNZ→end
        //   [S]
        //   LOAD I; LOAD __step_N; ADD; STORE I; POP
        //   JMP→check
        //  end:
        if (tryEat(TT::LEX_FOR)) {
            int n = forCounter++;
            std::string stepVar  = "__step_"  + std::to_string(n);
            std::string limitVar = "__limit_" + std::to_string(n);

            // Loop parameter (must be int)
            std::string iName = eat(TT::ID).val;
            auto& iEnt = sym.get(iName);
            if (iEnt.type != VType::INT)
                throw std::runtime_error("For loop parameter " + iName + " must be of type int");
            eat(TT::ASSIGN);

            // E1 → I
            parseExpr();
            InterPol({ OpCode::STORE, 0, 0.0, iName });
            InterPol({ OpCode::POP });

            eat(TT::LEX_STEP);

            // E2 → __step_N (declare hidden variable)
            sym.declareIter(stepVar, VType::INT);
            parseExpr();
            InterPol({ OpCode::STORE, 0, 0.0, stepVar });
            InterPol({ OpCode::POP });

            eat(TT::LEX_UNTIL);

            // E3 → __limit_N
            sym.declareIter(limitVar, VType::INT);
            parseExpr();
            InterPol({ OpCode::STORE, 0, 0.0, limitVar });
            InterPol({ OpCode::POP });

            eat(TT::LEX_DO);

            // Start of condition check (jump back here)
            int checkAddr = here();

            // If I > limit → exit loop
            InterPol({ OpCode::LOAD, 0, 0.0, iName });
            InterPol({ OpCode::LOAD, 0, 0.0, limitVar });
            InterPol({ OpCode::GT });
            int jnzIdx = InterPol({ OpCode::JNZ, 0 });   // address to patch

            // Loop body
            parseOper();

            // I = I + step
            InterPol({ OpCode::LOAD,  0, 0.0, iName });
            InterPol({ OpCode::LOAD,  0, 0.0, stepVar });
            InterPol({ OpCode::ADD });
            InterPol({ OpCode::STORE, 0, 0.0, iName });
            InterPol({ OpCode::POP });

            // Jump back to condition check
            InterPol({ OpCode::JMP, static_cast<long long>(checkAddr) });

            // End of loop — patch exit address
            patch(jnzIdx, here());
            return;
        }

        // while ( expression ) statement 
        if (tryEat(TT::LEX_WHILE)) {
            int loopStart = here();
            eat(TT::LPAREN);
            parseExpr();
            eat(TT::RPAREN);
            int jzIdx = InterPol({ OpCode::JZ, 0 });
            parseOper();
            InterPol({ OpCode::JMP, static_cast<long long>(loopStart) });
            patch(jzIdx, here());
            return;
        }

        // read ( identifier ) ;
        if (tryEat(TT::LEX_READ)) {
            eat(TT::LPAREN);
            std::string name = eat(TT::ID).val;
            sym.get(name);
            eat(TT::RPAREN);
            eat(TT::SEMICOLON);
            InterPol({ OpCode::READ, 0, 0.0, name });
            return;
        }

        // write ( expression { , expression } ) ; 
        if (tryEat(TT::LEX_WRITE)) {
            eat(TT::LPAREN);
            parseExpr();
            InterPol({ OpCode::WRITE });
            while (tryEat(TT::COMMA)) {
                parseExpr();
                InterPol({ OpCode::WRITE });
            }
            eat(TT::RPAREN);
            eat(TT::SEMICOLON);
            return;
        }

        // { statements } — compound statement
        if (tryEat(TT::LBRACE)) {
            while (!check(TT::RBRACE) && !check(TT::EOF_TOK))
                parseOper();
            eat(TT::RBRACE);
            return;
        }

        // expression ; — expression statement 
        parseExpr();
        InterPol({ OpCode::POP });   // expression result not needed
        eat(TT::SEMICOLON);
    }

public:
    Parser(std::vector<Token> tokens, SymTable& s, std::vector<PolizOp>& c): toks(std::move(tokens)), pos(0), sym(s), code(c) {}

    // 〈program〉 - program { 〈declarations〉 〈statements〉 }
    void parse() {
        eat(TT::LEX_PROGRAM);
        eat(TT::LBRACE);

        // First: all variable declarations
        while (check(TT::LEX_INT) || check(TT::LEX_STRING) || check(TT::LEX_REAL))
            parseDecl();

        // Then: statements
        while (!check(TT::RBRACE) && !check(TT::EOF_TOK))
            parseOper();

        eat(TT::RBRACE);

        // Verify all forward-gotos resolved their labels
        if (!fwdGotos.empty()) {
            throw std::runtime_error("goto to non-existent label " + fwdGotos.begin()->first);
        }
    }
};
#endif // PARSER_H