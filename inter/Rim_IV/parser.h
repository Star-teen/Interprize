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
    std::vector<Token> toks;       // поток токенов от лексера
    size_t pos;                    // текущая позиция в потоке
    SymTable& sym;                 // таблица символов
    std::vector<PolizOp>& code;    // сюда пишем ПОЛИЗ

    int forCounter = 0; // уникальный суффикс для скрытых переменных for

    // Таблица меток: имя адрес в ПОЛИЗ
    std::unordered_map<std::string, int> labels;

    // Forward-goto: имя метки и список индексов JMP, ждущих объявления
    std::unordered_map<std::string, std::vector<int>> fwdGotos;

    // Для удобства
    Token& cur() { return toks[pos]; }

    // Смотрим на токен впереди без сдвига
    Token& peekAhead(int d = 1) {
        size_t idx = pos + d;
        if (idx >= toks.size()) idx = toks.size() - 1;
        return toks[idx];
    }

    bool check(TT t) { return cur().type == t; }

    Token eat(TT t) {
        if (!check(t))
            throw std::runtime_error("Строка " + std::to_string(cur().line) + ": ожидалось " + ttName(t) + ", найдено '" + cur().val + "'");
        return toks[pos++];
    }

    bool tryEat(TT t) { if (check(t)) { pos++; return true; } return false; }

    // Добавить инструкцию в ПОЛИЗ, вернуть её индекс
    int InterPol(PolizOp op) {
        code.push_back(op);
        return static_cast<int>(code.size()) - 1;
    }

    // Текущий индекс (куда будет записана СЛЕДУЮЩАЯ инструкция)
    int here() { return static_cast<int>(code.size()); }

    // Заполнить адрес перехода в уже записанной инструкции
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

            case TT::ID:        return "идентификатор";

            case TT::LEX_PROGRAM: return "'program'";
            case TT::LEX_READ:    return "'read'";
            case TT::LEX_WRITE:   return "'write'";
            case TT::LEX_STEP:   return "'step'";
            case TT::LEX_UNTIL:  return "'until'";
            case TT::LEX_DO:     return "'do'";
            case TT::LEX_WHILE:  return "'while'";
            default:            return "токен";
        }
    }

    // Зарегистрировать метку: запомнить адрес, все forward-goto
    void registerLabel(const std::string& name) {
        if (labels.count(name))
            throw std::runtime_error("Метка '" + name + "' объявлена дважды");
        int addr = here();
        labels[name] = addr;
        // Патчим все goto которые ссылались на эту метку
        auto it = fwdGotos.find(name);
        if (it != fwdGotos.end()) {
            for (int idx : it->second) patch(idx, addr);
            fwdGotos.erase(it);
        }
    }

    // Разбор типов и констант
    VType parseType() {
        if (tryEat(TT::LEX_INT))    return VType::INT;
        if (tryEat(TT::LEX_STRING)) return VType::STRING;
        if (tryEat(TT::LEX_REAL))   return VType::REAL;
        throw std::runtime_error("Строка " + std::to_string(cur().line) + ": ожидался тип (int / string / real)");
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
            if (neg) throw std::runtime_error("Унарный минус неприменим к строке");
            return eat(TT::STR_L).val;
        }
        throw std::runtime_error("Строка " + std::to_string(cur().line) + ": ожидалась константа");
    }

    // 〈описание〉 -> 〈тип〉 〈имя〉 [= 〈конст〉] { , 〈имя〉 [= 〈конст〉] } ;
    void parseDecl() {
        VType t = parseType();
        do {
            std::string name = eat(TT::ID).val;
            if (tryEat(TT::ASSIGN)) {
                Val v = parseConst(t);
                sym.declareInit(name, t, v);
                // Генерируем ПОЛИЗ для инициализации
                if (t == VType::INT) InterPol({ OpCode::PUSH_INT, asInt(v) });
                else if (t == VType::REAL) InterPol({ OpCode::PUSH_REAL, 0, asReal(v) });
                else InterPol({ OpCode::PUSH_STR, 0, 0.0, asStr(v) });
                InterPol({ OpCode::STORE, 0, 0.0, name });
                InterPol({ OpCode::POP });
            } else { sym.declare(name, t);}
        } while (tryEat(TT::COMMA));
        eat(TT::SEMICOLON);
    }
    // Выражения по приоритетам
    // Присваивание: правоассоциативное (a = b = 5 → a = (b = 5))
    void parseExpr() {
        if (check(TT::ID) && peekAhead().type == TT::ASSIGN) {
            std::string name = eat(TT::ID).val;
            sym.get(name);         // проверяем что переменная объявлена
            eat(TT::ASSIGN);
            parseExpr();         // рекурсия (правоассоциативность)
            InterPol({ OpCode::STORE, 0, 0.0, name });
        } else { parseOr();}
    }

    //   [левый] [правый] OR
    void parseOr() {
        parseAnd();
        while (check(TT::LEX_OR)) {
            pos++;
            parseAnd();          // правый операнд ВСЕГДА вычисляется
            InterPol({ OpCode::OR });
        }
    }

    //   [левый] [правый] AND
    void parseAnd() {
        parseCmp();
        while (check(TT::LEX_AND)) {
            pos++;
            parseCmp();          // правый операнд ВСЕГДА вычисляется
            InterPol({ OpCode::AND });
        }
    }

    // Операции сравнения: < > <= >= == !=
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

    // Сложение и вычитание: + -
    void parseAdd() {
        parseMul();
        while (check(TT::PLUS) || check(TT::MINUS)) {
            OpCode op = check(TT::PLUS) ? OpCode::ADD : OpCode::SUB;
            pos++;
            parseMul();
            InterPol({ op });
        }
    }

    // Умножение, деление: * /
    void parseMul() {
        parseUnary();
        while (check(TT::STAR) || check(TT::SLASH)) {
            OpCode op = check(TT::STAR) ? OpCode::MUL : OpCode::DIV;
            pos++;
            parseUnary();
            InterPol({ op });
        }
    }

    // Унарные операторы: not и - унарный минус
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

    // Первичные: литерал, переменная, (выражение)
    void parseSimple() {
        if (check(TT::INT_L)) {
            InterPol({ OpCode::PUSH_INT, std::stoll(eat(TT::INT_L).val) });
        } else if (check(TT::REAL_L)) {
            InterPol({ OpCode::PUSH_REAL, 0, std::stod(eat(TT::REAL_L).val) });
        } else if (check(TT::STR_L)) {
            InterPol({ OpCode::PUSH_STR, 0, 0.0, eat(TT::STR_L).val });
        } else if (check(TT::ID)) {
            std::string name = eat(TT::ID).val;
            sym.get(name);   // проверяем что переменная объявлена
            InterPol({ OpCode::LOAD, 0, 0.0, name });
        } else if (tryEat(TT::LPAREN)) {
            parseExpr();
            eat(TT::RPAREN);
        } else {
            throw std::runtime_error("Строка " + std::to_string(cur().line) + ": неожиданный токен " + cur().val);
        }
    }

    // Операторы

    void parseOper() {
        // Помеченный оператор: метка : оператор
        if (check(TT::ID) && peekAhead().type == TT::COLON) {
            std::string labelName = eat(TT::ID).val;
            eat(TT::COLON);
            registerLabel(labelName);   // запоминаем адрес, разбираем forward-goto
            parseOper();
            return;
        }

        //  goto имя ; 
        //   1. Метка уже объявлена (backward goto) InterPol JMP с готовым адресом
        //   2. Метка ещё не встречена (forward goto) InterPol JMP 0, запомнить для правки
        if (tryEat(TT::LEX_GOTO)) {
            std::string labelName = eat(TT::ID).val;
            eat(TT::SEMICOLON);
            if (labels.count(labelName)) {
                // Backward goto — адрес уже известен
                InterPol({ OpCode::JMP, static_cast<long long>(labels[labelName]) });
            } else {
                // Forward goto — адрес узнаем позже
                int idx = InterPol({ OpCode::JMP, 0 });
                fwdGotos[labelName].push_back(idx);
            }
            return;
        }

        // if ( выражение ) оператор [else оператор]
        // ПОЛИЗ:
        //   [условие]
        //   JZ метка_else   (если false, прыгаем к else или за if)
        //   [тело if]
        //   JMP метка_end   (только если есть else)
        //   метка_else:
        //   [тело else]       (если есть)
        //   метка_end:
        if (tryEat(TT::LEX_IF)) {
            eat(TT::LPAREN);
            parseExpr();
            eat(TT::RPAREN);

            int jzIdx = InterPol({ OpCode::JZ, 0 });   // адрес добавим после

            parseOper();   // тело if

            if (tryEat(TT::LEX_ELSE)) {
                int jmpIdx = InterPol({ OpCode::JMP, 0 });
                patch(jzIdx, here());     // JZ → начало else
                parseOper();              // тело else
                patch(jmpIdx, here());    // JMP → за else
            } else {
                patch(jzIdx, here());     // JZ → за if (нет else)
            }
            return;
        }

        // for I = E1 step E2 until E3 do S
        //   • E1, E2, E3 вычисляются ОДИН РАЗ до входа в цикл
        //   • Если E1 > E3 — тело не выполняется ни разу
        //   • I: E1, E1+E2, E1+2*E2, ... пока I <= E3
        //   • После завершения значение I неопределено
        // ПОЛИЗ:
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

            // Параметр цикла (должен быть int)
            std::string iName = eat(TT::ID).val;
            auto& iEnt = sym.get(iName);
            if (iEnt.type != VType::INT)
                throw std::runtime_error("Параметр цикла for " + iName + " должен быть типа int");
            eat(TT::ASSIGN);

            // E1 → I
            parseExpr();
            InterPol({ OpCode::STORE, 0, 0.0, iName });
            InterPol({ OpCode::POP });

            eat(TT::LEX_STEP);

            // E2 → __step_N (объявляем скрытую переменную)
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

            // Начало проверки условия (сюда прыгаем назад)
            int checkAddr = here();

            // Если I > limit → выходим из цикла
            InterPol({ OpCode::LOAD, 0, 0.0, iName });
            InterPol({ OpCode::LOAD, 0, 0.0, limitVar });
            InterPol({ OpCode::GT });
            int jnzIdx = InterPol({ OpCode::JNZ, 0 });   // адрес добавим после

            // Тело цикла
            parseOper();

            // I = I + step
            InterPol({ OpCode::LOAD,  0, 0.0, iName });
            InterPol({ OpCode::LOAD,  0, 0.0, stepVar });
            InterPol({ OpCode::ADD });
            InterPol({ OpCode::STORE, 0, 0.0, iName });
            InterPol({ OpCode::POP });

            // Назад к проверке
            InterPol({ OpCode::JMP, static_cast<long long>(checkAddr) });

            // Конец цикла — добавим выход
            patch(jnzIdx, here());
            return;
        }

        //  while ( выражение ) оператор 
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

        //  read ( идентификатор ) ;
        if (tryEat(TT::LEX_READ)) {
            eat(TT::LPAREN);
            std::string name = eat(TT::ID).val;
            sym.get(name);
            eat(TT::RPAREN);
            eat(TT::SEMICOLON);
            InterPol({ OpCode::READ, 0, 0.0, name });
            return;
        }

        // write ( выражение { , выражение } ) ; 
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

        // { операторы } — составной оператор
        if (tryEat(TT::LBRACE)) {
            while (!check(TT::RBRACE) && !check(TT::EOF_TOK))
                parseOper();
            eat(TT::RBRACE);
            return;
        }

        //  выражение ; — оператор-выражение 
        parseExpr();
        InterPol({ OpCode::POP });   // результат выражения не нужен
        eat(TT::SEMICOLON);
    }

public:
    Parser(std::vector<Token> tokens, SymTable& s, std::vector<PolizOp>& c): toks(std::move(tokens)), pos(0), sym(s), code(c) {}

    // 〈программа〉 - program { 〈описания〉 〈операторы〉 }
    void parse() {
        eat(TT::LEX_PROGRAM);
        eat(TT::LBRACE);

        // Сначала все описания переменных
        while (check(TT::LEX_INT) || check(TT::LEX_STRING) || check(TT::LEX_REAL))
            parseDecl();

        // Потом операторы
        while (!check(TT::RBRACE) && !check(TT::EOF_TOK))
            parseOper();

        eat(TT::RBRACE);

        // Проверяем что все forward-goto получили свои метки
        if (!fwdGotos.empty()) {
            throw std::runtime_error("goto на несуществующую метку " + fwdGotos.begin()->first);
        }
    }
};
#endif // PARSER_H
