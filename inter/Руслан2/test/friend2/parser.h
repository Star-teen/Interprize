#pragma once
// ============================================================
// parser.h — синтаксический анализ + генерация ПОЛИЗ
//
// Метод: рекурсивный спуск.
// Каждому правилу грамматики соответствует один метод.
// Методы вызывают друг друга согласно грамматике и одновременно
// генерируют инструкции ПОЛИЗ (синтаксически управляемая генерация).
//
// ПРИОРИТЕТ ОПЕРАЦИЙ — цепочка методов (от низкого к высокому):
//
//   parseExpr()
//     └─ parseAssign()          =  (правоассоциативное)
//          └─ parseOr()         or
//               └─ parseAnd()   and
//                    └─ parseRel()   < > <= >= == !=
//                         └─ parseAdd()   + -
//                              └─ parseMul()   * / %
//                                   └─ parseUnary()   not  -expr
//                                        └─ parsePrimary()  литерал/ID/(expr)
//
// Метод с БОЛЕЕ НИЗКИМ приоритетом вызывает метод с БОЛЕЕ ВЫСОКИМ.
// Это гарантирует: a + b * c → a + (b*c), потому что parseMul
// вызывается глубже чем parseAdd.
//
// ОБЫЧНЫЕ вычисления and/or (вариант VI.2):
//   Просто генерируем оба операнда и потом инструкцию AND/OR.
//   Оба операнда ВСЕГДА вычисляются (в отличие от ленивых).
//
// for I = E1 step E2 until E3 do S:
//   E1, E2, E3 вычисляются ОДИН РАЗ до входа в цикл (требование ТЗ).
//   E2 и E3 сохраняются в скрытых переменных __step_N и __limit_N.
//   Для вложенных for используется счётчик forCounter.
//
// goto — поддержка forward/backward:
//   labels    — уже объявленные метки: имя → адрес в ПОЛИЗ
//   fwdGotos  — незакрытые forward-goto: имя → список индексов JMP
//   При объявлении метки все fwdGotos для неё патчатся.
//   В конце parse() проверяем что fwdGotos пуст.
// ============================================================
#include "token.h"
#include "symtable.h"
#include "poliz.h"
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <string>

class Parser {
    std::vector<Token>    toks;       // поток токенов от лексера
    size_t                pos;        // текущая позиция в потоке
    SymTable&             sym;        // таблица символов
    std::vector<PolizOp>& code;       // сюда пишем ПОЛИЗ

    int forCounter = 0;               // уникальный суффикс для скрытых переменных for

    // Таблица меток: имя → адрес в ПОЛИЗ (уже объявленные)
    std::unordered_map<std::string, int> labels;

    // Forward-goto: имя метки → список индексов JMP, ждущих патчинга
    std::unordered_map<std::string, std::vector<int>> fwdGotos;

    // ---- Вспомогательные ----

    Token& cur() { return toks[pos]; }

    // Смотрим на токен впереди без сдвига (look-ahead)
    Token& peekAhead(int d = 1) {
        size_t idx = pos + d;
        if (idx >= toks.size()) idx = toks.size() - 1;
        return toks[idx];
    }

    bool check(TT t) { return cur().type == t; }

    Token eat(TT t) {
        if (!check(t))
            throw std::runtime_error(
                "Строка " + std::to_string(cur().line) +
                ": ожидалось " + ttName(t) +
                ", найдено '" + cur().val + "'");
        return toks[pos++];
    }

    bool tryEat(TT t) { if (check(t)) { pos++; return true; } return false; }

    // Добавить инструкцию в ПОЛИЗ, вернуть её индекс
    int emit(PolizOp op) {
        code.push_back(op);
        return static_cast<int>(code.size()) - 1;
    }

    // Текущий индекс (куда будет записана СЛЕДУЮЩАЯ инструкция)
    int here() { return static_cast<int>(code.size()); }

    // Заполнить адрес перехода в уже записанной инструкции (патчинг)
    void patch(int instrIdx, int targetAddr) {
        code[instrIdx].ival = targetAddr;
    }

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
            case TT::KW_STEP:   return "'step'";
            case TT::KW_UNTIL:  return "'until'";
            case TT::KW_DO:     return "'do'";
            case TT::KW_WHILE:  return "'while'";
            default:            return "токен";
        }
    }

    // Зарегистрировать метку: запомнить адрес, запатчить все forward-goto на неё
    void registerLabel(const std::string& name) {
        if (labels.count(name))
            throw std::runtime_error(
                "Метка '" + name + "' объявлена дважды");
        int addr = here();
        labels[name] = addr;
        // Патчим все goto которые ссылались на эту метку
        auto it = fwdGotos.find(name);
        if (it != fwdGotos.end()) {
            for (int idx : it->second) patch(idx, addr);
            fwdGotos.erase(it);
        }
    }

    // ---- Разбор типов и констант ----

    VType parseType() {
        if (tryEat(TT::KW_INT))    return VType::INT;
        if (tryEat(TT::KW_STRING)) return VType::STRING;
        if (tryEat(TT::KW_REAL))   return VType::REAL;
        throw std::runtime_error(
            "Строка " + std::to_string(cur().line) +
            ": ожидался тип (int / string / real)");
    }

    Val parseConst(VType expectedType) {
        bool neg = false;
        if (check(TT::MINUS))     { pos++; neg = true; }
        else if (check(TT::PLUS)) { pos++; }

        if (check(TT::INT_LIT)) {
            long long v = std::stoll(eat(TT::INT_LIT).val) * (neg ? -1 : 1);
            if (expectedType == VType::REAL) return static_cast<double>(v);
            return v;
        }
        if (check(TT::REAL_LIT)) {
            double v = std::stod(eat(TT::REAL_LIT).val) * (neg ? -1.0 : 1.0);
            return v;
        }
        if (check(TT::STR_LIT)) {
            if (neg) throw std::runtime_error("Унарный минус неприменим к строке");
            return eat(TT::STR_LIT).val;
        }
        throw std::runtime_error(
            "Строка " + std::to_string(cur().line) + ": ожидалась константа");
    }

    // ---- Описание переменных ----

    // 〈описание〉 → 〈тип〉 〈имя〉 [= 〈конст〉] { , 〈имя〉 [= 〈конст〉] } ;
    void parseDecl() {
        VType t = parseType();
        do {
            std::string name = eat(TT::ID).val;
            if (tryEat(TT::ASSIGN)) {
                Val v = parseConst(t);
                sym.declareInit(name, t, v);
                // Генерируем ПОЛИЗ для инициализации
                if (t == VType::INT)
                    emit({ OpCode::PUSH_INT, asInt(v) });
                else if (t == VType::REAL)
                    emit({ OpCode::PUSH_REAL, 0, asReal(v) });
                else
                    emit({ OpCode::PUSH_STR, 0, 0.0, asStr(v) });
                emit({ OpCode::STORE, 0, 0.0, name });
                emit({ OpCode::POP });
            } else {
                sym.declare(name, t);
            }
        } while (tryEat(TT::COMMA));
        eat(TT::SEMICOLON);
    }

    // ---- Выражения ----

    void parseExpr() { parseAssign(); }

    // Присваивание: правоассоциативное (a = b = 5 → a = (b = 5))
    // Look-ahead: если видим ID потом = (не ==) → это присваивание
    void parseAssign() {
        if (check(TT::ID) && peekAhead().type == TT::ASSIGN) {
            std::string name = eat(TT::ID).val;
            sym.get(name);         // проверяем что переменная объявлена
            eat(TT::ASSIGN);
            parseAssign();         // рекурсия (правоассоциативность)
            // STORE не снимает значение — результат присваивания = значение
            emit({ OpCode::STORE, 0, 0.0, name });
        } else {
            parseOr();
        }
    }

    // ОБЫЧНЫЕ вычисления OR (вариант VI.2):
    //   Всегда вычисляем оба операнда, потом инструкцию OR.
    //   [левый] [правый] OR
    void parseOr() {
        parseAnd();
        while (check(TT::KW_OR)) {
            pos++;
            parseAnd();          // правый операнд ВСЕГДА вычисляется
            emit({ OpCode::OR });
        }
    }

    // ОБЫЧНЫЕ вычисления AND (вариант VI.2):
    //   [левый] [правый] AND
    void parseAnd() {
        parseRel();
        while (check(TT::KW_AND)) {
            pos++;
            parseRel();          // правый операнд ВСЕГДА вычисляется
            emit({ OpCode::AND });
        }
    }

    // Операции сравнения: < > <= >= == !=
    void parseRel() {
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
            emit({ op });
        }
    }

    // Сложение и вычитание: + -
    void parseAdd() {
        parseMul();
        while (check(TT::PLUS) || check(TT::MINUS)) {
            OpCode op = check(TT::PLUS) ? OpCode::ADD : OpCode::SUB;
            pos++;
            parseMul();
            emit({ op });
        }
    }

    // Умножение, деление, остаток: * / %
    void parseMul() {
        parseUnary();
        while (check(TT::STAR) || check(TT::SLASH) || check(TT::PERCENT)) {
            OpCode op;
            if      (check(TT::STAR))    op = OpCode::MUL;
            else if (check(TT::SLASH))   op = OpCode::DIV;
            else                          op = OpCode::MOD;
            pos++;
            parseUnary();
            emit({ op });
        }
    }

    // Унарные операторы: not и - (унарный минус, вариант V.1)
    void parseUnary() {
        if (tryEat(TT::KW_NOT)) {
            parseUnary();
            emit({ OpCode::NOT });
        } else if (check(TT::MINUS)) {
            pos++;
            parseUnary();
            emit({ OpCode::NEG });
        } else {
            parsePrimary();
        }
    }

    // Первичные: литерал, переменная, (выражение)
    void parsePrimary() {
        if (check(TT::INT_LIT)) {
            emit({ OpCode::PUSH_INT, std::stoll(eat(TT::INT_LIT).val) });
        } else if (check(TT::REAL_LIT)) {
            emit({ OpCode::PUSH_REAL, 0, std::stod(eat(TT::REAL_LIT).val) });
        } else if (check(TT::STR_LIT)) {
            emit({ OpCode::PUSH_STR, 0, 0.0, eat(TT::STR_LIT).val });
        } else if (check(TT::ID)) {
            std::string name = eat(TT::ID).val;
            sym.get(name);   // проверяем что переменная объявлена
            emit({ OpCode::LOAD, 0, 0.0, name });
        } else if (tryEat(TT::LPAREN)) {
            parseExpr();
            eat(TT::RPAREN);
        } else {
            throw std::runtime_error(
                "Строка " + std::to_string(cur().line) +
                ": неожиданный токен '" + cur().val + "' в выражении");
        }
    }

    // ---- Операторы ----

    void parseStmt() {

        // --- Помеченный оператор: метка : оператор ---
        // Look-ahead: текущий токен ID И следующий токен ':'
        if (check(TT::ID) && peekAhead().type == TT::COLON) {
            std::string labelName = eat(TT::ID).val;
            eat(TT::COLON);
            registerLabel(labelName);   // запоминаем адрес, патчим forward-goto
            parseStmt();                // разбираем помеченный оператор
            return;
        }

        // --- goto имя ; ---
        // (вариант III.1)
        //
        // Два случая:
        //   1. Метка уже объявлена (backward goto) → emit JMP с готовым адресом
        //   2. Метка ещё не встречена (forward goto) → emit JMP 0, запомнить для патчинга
        if (tryEat(TT::KW_GOTO)) {
            std::string labelName = eat(TT::ID).val;
            eat(TT::SEMICOLON);
            if (labels.count(labelName)) {
                // Backward goto — адрес уже известен
                emit({ OpCode::JMP, static_cast<long long>(labels[labelName]) });
            } else {
                // Forward goto — адрес узнаем позже
                int idx = emit({ OpCode::JMP, 0 });
                fwdGotos[labelName].push_back(idx);
            }
            return;
        }

        // --- if ( выражение ) оператор [else оператор] ---
        // (вариант I.1)
        //
        // ПОЛИЗ:
        //   [условие]
        //   JZ → метка_else   (если false, прыгаем к else или за if)
        //   [тело if]
        //   JMP → метка_end   (только если есть else)
        //   метка_else:
        //   [тело else]       (если есть)
        //   метка_end:
        if (tryEat(TT::KW_IF)) {
            eat(TT::LPAREN);
            parseExpr();
            eat(TT::RPAREN);

            int jzIdx = emit({ OpCode::JZ, 0 });   // адрес патчим после

            parseStmt();   // тело if

            if (tryEat(TT::KW_ELSE)) {
                int jmpIdx = emit({ OpCode::JMP, 0 });
                patch(jzIdx, here());     // JZ → начало else
                parseStmt();              // тело else
                patch(jmpIdx, here());    // JMP → за else
            } else {
                patch(jzIdx, here());     // JZ → за if (нет else)
            }
            return;
        }

        // --- for I = E1 step E2 until E3 do S ---
        // (вариант II.3)
        //
        // Семантика по ТЗ:
        //   • E1, E2, E3 вычисляются ОДИН РАЗ до входа в цикл
        //   • Если E1 > E3 — тело не выполняется ни разу
        //   • I: E1, E1+E2, E1+2*E2, ... пока I <= E3
        //   • Параметр цикла I должен быть типа int (контекстное условие)
        //   • После завершения значение I неопределено (требование ТЗ)
        //
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
        if (tryEat(TT::KW_FOR)) {
            int n = forCounter++;
            std::string stepVar  = "__step_"  + std::to_string(n);
            std::string limitVar = "__limit_" + std::to_string(n);

            // Параметр цикла (должен быть int)
            std::string iName = eat(TT::ID).val;
            auto& iEntry = sym.get(iName);
            if (iEntry.type != VType::INT)
                throw std::runtime_error(
                    "Параметр цикла for '" + iName + "' должен быть типа int");

            eat(TT::ASSIGN);

            // E1 → I
            parseExpr();
            emit({ OpCode::STORE, 0, 0.0, iName });
            emit({ OpCode::POP });

            eat(TT::KW_STEP);

            // E2 → __step_N (объявляем скрытую переменную)
            sym.declareHidden(stepVar, VType::INT);
            parseExpr();
            emit({ OpCode::STORE, 0, 0.0, stepVar });
            emit({ OpCode::POP });

            eat(TT::KW_UNTIL);

            // E3 → __limit_N
            sym.declareHidden(limitVar, VType::INT);
            parseExpr();
            emit({ OpCode::STORE, 0, 0.0, limitVar });
            emit({ OpCode::POP });

            eat(TT::KW_DO);

            // Начало проверки условия (сюда прыгаем назад)
            int checkAddr = here();

            // Если I > limit → выходим из цикла
            emit({ OpCode::LOAD, 0, 0.0, iName });
            emit({ OpCode::LOAD, 0, 0.0, limitVar });
            emit({ OpCode::GT });
            int jnzIdx = emit({ OpCode::JNZ, 0 });   // адрес патчим после

            // Тело цикла
            parseStmt();

            // I = I + step
            emit({ OpCode::LOAD,  0, 0.0, iName });
            emit({ OpCode::LOAD,  0, 0.0, stepVar });
            emit({ OpCode::ADD });
            emit({ OpCode::STORE, 0, 0.0, iName });
            emit({ OpCode::POP });

            // Назад к проверке
            emit({ OpCode::JMP, static_cast<long long>(checkAddr) });

            // Конец цикла — патчим выход
            patch(jnzIdx, here());
            return;
        }

        // --- while ( выражение ) оператор ---
        // (входит в общую часть ТЗ)
        if (tryEat(TT::KW_WHILE)) {
            int loopStart = here();
            eat(TT::LPAREN);
            parseExpr();
            eat(TT::RPAREN);
            int jzIdx = emit({ OpCode::JZ, 0 });
            parseStmt();
            emit({ OpCode::JMP, static_cast<long long>(loopStart) });
            patch(jzIdx, here());
            return;
        }

        // --- read ( идентификатор ) ; ---
        if (tryEat(TT::KW_READ)) {
            eat(TT::LPAREN);
            std::string name = eat(TT::ID).val;
            sym.get(name);
            eat(TT::RPAREN);
            eat(TT::SEMICOLON);
            emit({ OpCode::READ, 0, 0.0, name });
            return;
        }

        // --- write ( выражение { , выражение } ) ; ---
        if (tryEat(TT::KW_WRITE)) {
            eat(TT::LPAREN);
            parseExpr();
            emit({ OpCode::WRITE });
            while (tryEat(TT::COMMA)) {
                parseExpr();
                emit({ OpCode::WRITE });
            }
            eat(TT::RPAREN);
            eat(TT::SEMICOLON);
            return;
        }

        // --- { операторы } — составной оператор ---
        if (tryEat(TT::LBRACE)) {
            while (!check(TT::RBRACE) && !check(TT::EOF_TOK))
                parseStmt();
            eat(TT::RBRACE);
            return;
        }

        // --- выражение ; — оператор-выражение ---
        parseExpr();
        emit({ OpCode::POP });   // результат выражения не нужен
        eat(TT::SEMICOLON);
    }

public:
    Parser(std::vector<Token> tokens, SymTable& s, std::vector<PolizOp>& c)
        : toks(std::move(tokens)), pos(0), sym(s), code(c) {}

    // 〈программа〉 → program { 〈описания〉 〈операторы〉 }
    void parse() {
        eat(TT::KW_PROGRAM);
        eat(TT::LBRACE);

        // Сначала все описания переменных
        while (check(TT::KW_INT) || check(TT::KW_STRING) || check(TT::KW_REAL))
            parseDecl();

        // Потом операторы
        while (!check(TT::RBRACE) && !check(TT::EOF_TOK))
            parseStmt();

        eat(TT::RBRACE);

        // Проверяем что все forward-goto получили свои метки
        if (!fwdGotos.empty()) {
            throw std::runtime_error(
                "goto на несуществующую метку '" + fwdGotos.begin()->first + "'");
        }
    }
};
