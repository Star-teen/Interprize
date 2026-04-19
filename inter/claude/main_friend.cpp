// ============================================================
//  ИНТЕРПРЕТАТОР МОДЕЛЬНОГО ЯЗЫКА ПРОГРАММИРОВАНИЯ
//  2 курс ВМиК МГУ
//
//  Вариант:
//    I.1   — if (expr) stmt [else stmt]
//    II.3  — for I = E1 step E2 until E3 do S
//    III.1 — goto метка; / метка: оператор
//    IV.2  — тип real (вещественный)
//    V.1   — унарный минус
//    VI.2  — обычные вычисления (всё выражение, не ленивые)
//
//  Инструментальный язык: C++17
//  Сборка: g++ -std=c++17 -Wall -o interp main.cpp
//  Запуск: ./interp program.ml
// ============================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <variant>
#include <stdexcept>
#include <cctype>


// ============================================================
// ЧАСТЬ 1. ЛЕКСЕМЫ (ТОКЕНЫ)
//
// Токен — минимальная смысловая единица языка.
// "for" — один токен, "3.14" — один токен, "<=" — один токен.
// ============================================================

enum class TT {
    // Литералы
    INT_LIT,    // целое: 42
    REAL_LIT,   // вещественное: 3.14
    STR_LIT,    // строка: "hello"

    // Идентификатор (имя переменной или метки)
    ID,

    // Ключевые слова — типы данных
    KW_INT,
    KW_STRING,
    KW_REAL,

    // Ключевые слова — управление
    KW_PROGRAM,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_FOR,     // for  (цикл с параметром — наш вариант II.3)
    KW_STEP,    // step
    KW_UNTIL,   // until
    KW_DO,      // do
    KW_GOTO,    // goto (наш вариант III.1)
    KW_READ,
    KW_WRITE,

    // Логика
    KW_AND,
    KW_OR,
    KW_NOT,

    // Арифметика
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,

    // Сравнение
    LT, GT, LE, GE, EQ, NEQ,

    // Присваивание
    ASSIGN,     // =

    // Разделители
    LPAREN,     // (
    RPAREN,     // )
    LBRACE,     // {
    RBRACE,     // }
    SEMICOLON,  // ;
    COMMA,      // ,
    COLON,      // : (разделитель метки: "метка: оператор")

    EOF_TOK
};

struct Token {
    TT          type;
    std::string val;
    int         line;

    Token(TT t, std::string v, int ln)
        : type(t), val(std::move(v)), line(ln) {}
};


// ============================================================
// ЧАСТЬ 2. ЛЕКСЕР (ЛЕКСИЧЕСКИЙ АНАЛИЗАТОР)
//
// Читает строку src символ за символом, выдаёт вектор токенов.
// Реализован как конечный автомат.
// ============================================================

class Lexer {
    std::string src;
    size_t      pos;
    int         ln;

    char cur()      { return pos < src.size() ? src[pos] : '\0'; }
    char peekNext() { return (pos+1) < src.size() ? src[pos+1] : '\0'; }

    char advance() {
        char c = src[pos++];
        if (c == '\n') ln++;
        return c;
    }

    void skipWhitespace() {
        while (pos < src.size() && std::isspace((unsigned char)cur()))
            advance();
    }

    void skipComment() {
        advance(); advance(); // съедаем '/' и '*'
        while (pos + 1 < src.size()) {
            if (cur() == '*' && peekNext() == '/') {
                advance(); advance();
                return;
            }
            advance();
        }
        throw std::runtime_error("Незакрытый комментарий /* без */");
    }

    Token readNumber(int L) {
        std::string s;
        while (pos < src.size() && std::isdigit((unsigned char)cur()))
            s += advance();

        // Точка после числа → вещественное
        if (cur() == '.' && std::isdigit((unsigned char)peekNext())) {
            s += advance(); // '.'
            while (pos < src.size() && std::isdigit((unsigned char)cur()))
                s += advance();
            return Token(TT::REAL_LIT, s, L);
        }
        return Token(TT::INT_LIT, s, L);
    }

    Token readString(int L) {
        advance(); // открывающая "
        std::string s;
        while (pos < src.size() && cur() != '"') {
            if (cur() == '\n')
                throw std::runtime_error(
                    "Строка не закрыта (строка " + std::to_string(ln) + ")");
            s += advance();
        }
        if (cur() != '"')
            throw std::runtime_error("Строка не закрыта (конец файла)");
        advance(); // закрывающая "
        return Token(TT::STR_LIT, s, L);
    }

    Token readIdentOrKeyword(int L) {
        std::string s;
        while (pos < src.size() &&
               (std::isalnum((unsigned char)cur()) || cur() == '_'))
            s += advance();

        // Таблица ключевых слов
        if (s == "program") return Token(TT::KW_PROGRAM, s, L);
        if (s == "int")     return Token(TT::KW_INT,     s, L);
        if (s == "string")  return Token(TT::KW_STRING,  s, L);
        if (s == "real")    return Token(TT::KW_REAL,     s, L);
        if (s == "if")      return Token(TT::KW_IF,       s, L);
        if (s == "else")    return Token(TT::KW_ELSE,     s, L);
        if (s == "while")   return Token(TT::KW_WHILE,    s, L);
        if (s == "for")     return Token(TT::KW_FOR,      s, L);
        if (s == "step")    return Token(TT::KW_STEP,     s, L);
        if (s == "until")   return Token(TT::KW_UNTIL,    s, L);
        if (s == "do")      return Token(TT::KW_DO,       s, L);
        if (s == "goto")    return Token(TT::KW_GOTO,     s, L);
        if (s == "read")    return Token(TT::KW_READ,     s, L);
        if (s == "write")   return Token(TT::KW_WRITE,    s, L);
        if (s == "and")     return Token(TT::KW_AND,      s, L);
        if (s == "or")      return Token(TT::KW_OR,       s, L);
        if (s == "not")     return Token(TT::KW_NOT,      s, L);

        return Token(TT::ID, s, L); // имя переменной или метки
    }

public:
    explicit Lexer(std::string text) : src(std::move(text)), pos(0), ln(1) {}

    std::vector<Token> tokenize() {
        std::vector<Token> result;

        while (true) {
            // Пропускаем пробелы и комментарии
            while (true) {
                skipWhitespace();
                if (cur() == '/' && peekNext() == '*') {
                    skipComment();
                    continue;
                }
                break;
            }

            if (cur() == '\0') {
                result.push_back(Token(TT::EOF_TOK, "", ln));
                break;
            }

            int L = ln;
            char c = cur();

            if (std::isdigit((unsigned char)c)) {
                result.push_back(readNumber(L)); continue;
            }
            if (std::isalpha((unsigned char)c) || c == '_') {
                result.push_back(readIdentOrKeyword(L)); continue;
            }
            if (c == '"') {
                result.push_back(readString(L)); continue;
            }

            // Двухсимвольные операторы
            if (c == '<' && peekNext() == '=') { advance(); advance(); result.push_back(Token(TT::LE,  "<=", L)); continue; }
            if (c == '>' && peekNext() == '=') { advance(); advance(); result.push_back(Token(TT::GE,  ">=", L)); continue; }
            if (c == '=' && peekNext() == '=') { advance(); advance(); result.push_back(Token(TT::EQ,  "==", L)); continue; }
            if (c == '!' && peekNext() == '=') { advance(); advance(); result.push_back(Token(TT::NEQ, "!=", L)); continue; }

            // Односимвольные
            advance();
            switch (c) {
                case '+': result.push_back(Token(TT::PLUS,      "+", L)); break;
                case '-': result.push_back(Token(TT::MINUS,     "-", L)); break;
                case '*': result.push_back(Token(TT::STAR,      "*", L)); break;
                case '/': result.push_back(Token(TT::SLASH,     "/", L)); break;
                case '%': result.push_back(Token(TT::PERCENT,   "%", L)); break;
                case '<': result.push_back(Token(TT::LT,        "<", L)); break;
                case '>': result.push_back(Token(TT::GT,        ">", L)); break;
                case '=': result.push_back(Token(TT::ASSIGN,    "=", L)); break;
                case '(': result.push_back(Token(TT::LPAREN,    "(", L)); break;
                case ')': result.push_back(Token(TT::RPAREN,    ")", L)); break;
                case '{': result.push_back(Token(TT::LBRACE,    "{", L)); break;
                case '}': result.push_back(Token(TT::RBRACE,    "}", L)); break;
                case ';': result.push_back(Token(TT::SEMICOLON, ";", L)); break;
                case ',': result.push_back(Token(TT::COMMA,     ",", L)); break;
                case ':': result.push_back(Token(TT::COLON,     ":", L)); break;
                default:
                    throw std::runtime_error(
                        "Строка " + std::to_string(L) +
                        ": неизвестный символ '" + c + "'");
            }
        }
        return result;
    }
};


// ============================================================
// ЧАСТЬ 3. ТАБЛИЦА СИМВОЛОВ
//
// Хранит:
//   - переменные: имя → { тип, значение, инициализирована }
//   - метки (labels): имя → адрес в ПОЛИЗ
// ============================================================

enum class VType { INT, REAL, STRING };

using Val = std::variant<long long, double, std::string>;

VType valType(const Val& v) {
    if (std::holds_alternative<long long>(v)) return VType::INT;
    if (std::holds_alternative<double>(v))    return VType::REAL;
    return VType::STRING;
}
long long   asInt(const Val& v)  { return std::get<long long>(v); }
double      asReal(const Val& v) { return std::get<double>(v); }
std::string asStr(const Val& v)  { return std::get<std::string>(v); }

double toReal(const Val& v) {
    if (std::holds_alternative<long long>(v)) return static_cast<double>(asInt(v));
    return asReal(v);
}

std::string valToStr(const Val& v) {
    if (std::holds_alternative<long long>(v))
        return std::to_string(asInt(v));
    if (std::holds_alternative<double>(v)) {
        // Убираем лишние нули: 3.140000 → 3.14
        std::string s = std::to_string(asReal(v));
        auto dot = s.find('.');
        if (dot != std::string::npos) {
            size_t last = s.find_last_not_of('0');
            if (last == dot) last++;
            s = s.substr(0, last + 1);
        }
        return s;
    }
    return asStr(v);
}

struct SymEntry {
    VType type;
    Val   val;
    bool  initialized;
};

class SymTable {
    std::unordered_map<std::string, SymEntry> vars;

public:
    void declare(const std::string& name, VType t) {
        if (vars.count(name))
            throw std::runtime_error("Повторное объявление переменной '" + name + "'");
        Val def;
        if      (t == VType::INT)    def = 0LL;
        else if (t == VType::REAL)   def = 0.0;
        else                          def = std::string{};
        vars[name] = { t, def, false };
    }

    void declareInit(const std::string& name, VType t, Val v) {
        declare(name, t);
        set(name, v);
        vars[name].initialized = true;
    }

    // Объявление скрытой служебной переменной (для for: __step, __limit)
    // Не проверяем повторное объявление — используется только внутри
    void declareHidden(const std::string& name, VType t) {
        Val def;
        if      (t == VType::INT)  def = 0LL;
        else if (t == VType::REAL) def = 0.0;
        else                        def = std::string{};
        vars[name] = { t, def, false };
    }

    SymEntry& get(const std::string& name) {
        auto it = vars.find(name);
        if (it == vars.end())
            throw std::runtime_error("Переменная '" + name + "' не объявлена");
        return it->second;
    }

    // Присваивание с проверкой совместимости типов (Таблица №2)
    void set(const std::string& name, Val v) {
        auto& e = get(name);
        if (e.type == VType::INT) {
            if (std::holds_alternative<double>(v))
                v = static_cast<long long>(asReal(v));   // real→int: усечение
            else if (!std::holds_alternative<long long>(v))
                throw std::runtime_error(
                    "Несовместимость типов при присваивании в '" + name + "'");
        } else if (e.type == VType::REAL) {
            if (std::holds_alternative<long long>(v))
                v = static_cast<double>(asInt(v));        // int→real: расширение
            else if (!std::holds_alternative<double>(v))
                throw std::runtime_error(
                    "Несовместимость типов при присваивании в '" + name + "'");
        } else {
            if (!std::holds_alternative<std::string>(v))
                throw std::runtime_error(
                    "Несовместимость типов при присваивании в '" + name + "'");
        }
        e.val         = v;
        e.initialized = true;
    }

    Val value(const std::string& name) {
        auto& e = get(name);
        if (!e.initialized)
            throw std::runtime_error(
                "Переменная '" + name + "' использована до инициализации");
        return e.val;
    }
};


// ============================================================
// ЧАСТЬ 4. ПОЛИЗ (ПОЛЬСКАЯ ИНВЕРСНАЯ ЗАПИСЬ)
//
// Программа компилируется в вектор инструкций PolizOp.
// Выполняется стековой машиной (Interpreter).
// ============================================================

enum class OpCode {
    PUSH_INT,   // стек ← ival
    PUSH_REAL,  // стек ← rval
    PUSH_STR,   // стек ← sval

    LOAD,       // стек ← значение переменной sval
    STORE,      // переменная sval ← вершина стека (не снимать)

    ADD, SUB, MUL, DIV, MOD,
    NEG,        // унарный минус

    LT, GT, LE, GE, EQ, NEQ,   // сравнения → 0 или 1

    // Обычные (не ленивые) логические операции:
    AND,        // снять R, L; положить (L != 0 && R != 0) ? 1 : 0
    OR,         // снять R, L; положить (L != 0 || R != 0) ? 1 : 0
    NOT,        // снять v; положить (v == 0) ? 1 : 0

    JMP,        // безусловный переход к ival
    JZ,         // если вершина == 0 → перейти к ival
    JNZ,        // если вершина != 0 → перейти к ival (для for: I > limit)

    READ,       // cin → переменная sval
    WRITE,      // снять со стека, вывести
    POP         // снять вершину (мусор от expr-оператора)
};

struct PolizOp {
    OpCode      code;
    long long   ival = 0;
    double      rval = 0.0;
    std::string sval;
};


// ============================================================
// ЧАСТЬ 5. ПАРСЕР (РЕКУРСИВНЫЙ СПУСК + ГЕНЕРАЦИЯ ПОЛИЗ)
//
// Каждый метод соответствует правилу грамматики.
// Методы вызывают друг друга, строя ПОЛИЗ на ходу.
//
// Приоритет операций (от низкого к высокому):
//   parseAssign → parseOr → parseAnd → parseRel →
//   parseAdd → parseMul → parseUnary → parsePrimary
// ============================================================

class Parser {
    std::vector<Token>    toks;
    size_t                pos;
    SymTable&             sym;
    std::vector<PolizOp>& code;

    // Счётчик для генерации уникальных имён служебных переменных
    // (нужно для вложенных for-циклов)
    int forCounter = 0;

    // Таблица меток: имя → адрес в ПОЛИЗ (уже объявленные)
    std::unordered_map<std::string, int> labels;

    // Forward-goto: имя → список индексов JMP, которые ждут адреса
    std::unordered_map<std::string, std::vector<int>> fwdGotos;

    // ---- Вспомогательные методы ----

    Token& cur() { return toks[pos]; }

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

    int emit(PolizOp op) {
        code.push_back(op);
        return static_cast<int>(code.size()) - 1;
    }

    int here() { return static_cast<int>(code.size()); }

    void patch(int idx, int addr) { code[idx].ival = addr; }

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

    // Зарегистрировать метку: запомнить адрес, запатчить все forward-goto
    void registerLabel(const std::string& name) {
        if (labels.count(name))
            throw std::runtime_error(
                "Метка '" + name + "' объявлена дважды");
        int addr = here();
        labels[name] = addr;

        // Патчим все goto которые ссылались на эту метку
        auto it = fwdGotos.find(name);
        if (it != fwdGotos.end()) {
            for (int idx : it->second)
                patch(idx, addr);
            fwdGotos.erase(it);
        }
    }

    // ---- Типы данных ----

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
        if (check(TT::MINUS))      { pos++; neg = true; }
        else if (check(TT::PLUS))  { pos++; }

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

    // ---- Описания переменных ----

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

    // Присваивание (правоассоциативное)
    void parseAssign() {
        if (check(TT::ID) && peekAhead().type == TT::ASSIGN) {
            std::string name = eat(TT::ID).val;
            sym.get(name);
            eat(TT::ASSIGN);
            parseAssign();
            emit({ OpCode::STORE, 0, 0.0, name });
        } else {
            parseOr();
        }
    }

    // ОБЫЧНЫЕ (не ленивые) вычисления OR:
    // Оба операнда всегда вычисляются. Просто генерируем оба, потом OR.
    //   [левый]
    //   [правый]
    //   OR
    void parseOr() {
        parseAnd();
        while (check(TT::KW_OR)) {
            pos++;
            parseAnd();          // всегда вычисляем правый
            emit({ OpCode::OR });
        }
    }

    // ОБЫЧНЫЕ вычисления AND:
    // Оба операнда всегда вычисляются.
    //   [левый]
    //   [правый]
    //   AND
    void parseAnd() {
        parseRel();
        while (check(TT::KW_AND)) {
            pos++;
            parseRel();          // всегда вычисляем правый
            emit({ OpCode::AND });
        }
    }

    // Сравнения
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

    // Сложение и вычитание
    void parseAdd() {
        parseMul();
        while (check(TT::PLUS) || check(TT::MINUS)) {
            OpCode op = check(TT::PLUS) ? OpCode::ADD : OpCode::SUB;
            pos++;
            parseMul();
            emit({ op });
        }
    }

    // Умножение, деление, остаток
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

    // Унарные операторы
    void parseUnary() {
        if (tryEat(TT::KW_NOT)) {
            parseUnary();
            emit({ OpCode::NOT });
        } else if (check(TT::MINUS)) {
            pos++;
            parseUnary();
            emit({ OpCode::NEG });   // унарный минус (вариант V.1)
        } else {
            parsePrimary();
        }
    }

    // Первичные: литерал, идентификатор, (выражение)
    void parsePrimary() {
        if (check(TT::INT_LIT)) {
            emit({ OpCode::PUSH_INT, std::stoll(eat(TT::INT_LIT).val) });
        } else if (check(TT::REAL_LIT)) {
            emit({ OpCode::PUSH_REAL, 0, std::stod(eat(TT::REAL_LIT).val) });
        } else if (check(TT::STR_LIT)) {
            emit({ OpCode::PUSH_STR, 0, 0.0, eat(TT::STR_LIT).val });
        } else if (check(TT::ID)) {
            std::string name = eat(TT::ID).val;
            sym.get(name);
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

        // ---- Помеченный оператор: метка : оператор ----
        // Смотрим: текущий токен — ID, следующий — ':' ?
        if (check(TT::ID) && peekAhead().type == TT::COLON) {
            std::string labelName = eat(TT::ID).val;
            eat(TT::COLON);
            registerLabel(labelName);   // регистрируем метку, патчим forward-goto
            parseStmt();                // разбираем помеченный оператор
            return;
        }

        // ---- goto имя ; ----
        if (tryEat(TT::KW_GOTO)) {
            std::string labelName = eat(TT::ID).val;
            eat(TT::SEMICOLON);

            if (labels.count(labelName)) {
                // Метка уже известна (backward goto)
                emit({ OpCode::JMP, static_cast<long long>(labels[labelName]) });
            } else {
                // Метка ещё не встречена (forward goto) — патчим позже
                int idx = emit({ OpCode::JMP, 0 });
                fwdGotos[labelName].push_back(idx);
            }
            return;
        }

        // ---- if ( выражение ) оператор [else оператор] ----
        if (tryEat(TT::KW_IF)) {
            eat(TT::LPAREN);
            parseExpr();
            eat(TT::RPAREN);

            int jzIdx = emit({ OpCode::JZ, 0 });   // false → else или конец

            parseStmt();

            if (tryEat(TT::KW_ELSE)) {
                int jmpIdx = emit({ OpCode::JMP, 0 });
                patch(jzIdx, here());     // JZ → начало else
                parseStmt();
                patch(jmpIdx, here());   // JMP → за else
            } else {
                patch(jzIdx, here());    // JZ → за if
            }
            return;
        }

        // ---- for I = E1 step E2 until E3 do S ----
        //
        // Семантика (ТЗ):
        //   E1, E2, E3 вычисляются ОДИН РАЗ до входа в цикл.
        //   I принимает значения E1, E1+E2, E1+2*E2, ... пока I <= E3.
        //   Если E1 > E3 — тело не выполняется ни разу.
        //   После нормального завершения значение I неопределено.
        //
        // Генерируемый ПОЛИЗ:
        //   [E1] STORE I POP
        //   [E2] STORE __step_N POP
        //   [E3] STORE __limit_N POP
        // loop_check:
        //   LOAD I; LOAD __limit_N; GT; JNZ → loop_end
        //   [S]
        //   LOAD I; LOAD __step_N; ADD; STORE I; POP
        //   JMP → loop_check
        // loop_end:
        if (tryEat(TT::KW_FOR)) {
            // Уникальные имена скрытых переменных для этого for
            int n = forCounter++;
            std::string stepVar  = "__step_"  + std::to_string(n);
            std::string limitVar = "__limit_" + std::to_string(n);

            // Имя параметра цикла
            std::string iName = eat(TT::ID).val;
            // Проверяем что параметр объявлен и имеет целый тип
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

            // Начало проверки условия
            int checkAddr = here();

            // Проверка: I > limit → выходим
            emit({ OpCode::LOAD, 0, 0.0, iName });
            emit({ OpCode::LOAD, 0, 0.0, limitVar });
            emit({ OpCode::GT });
            int jnzIdx = emit({ OpCode::JNZ, 0 });   // если I > limit → конец

            // Тело цикла
            parseStmt();

            // I = I + step
            emit({ OpCode::LOAD, 0, 0.0, iName });
            emit({ OpCode::LOAD, 0, 0.0, stepVar });
            emit({ OpCode::ADD });
            emit({ OpCode::STORE, 0, 0.0, iName });
            emit({ OpCode::POP });

            // Назад к проверке
            emit({ OpCode::JMP, static_cast<long long>(checkAddr) });

            // Конец цикла
            patch(jnzIdx, here());
            return;
        }

        // ---- while ( выражение ) оператор ----
        // (while входит в общую часть ТЗ)
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

        // ---- read ( идентификатор ) ; ----
        if (tryEat(TT::KW_READ)) {
            eat(TT::LPAREN);
            std::string name = eat(TT::ID).val;
            sym.get(name);
            eat(TT::RPAREN);
            eat(TT::SEMICOLON);
            emit({ OpCode::READ, 0, 0.0, name });
            return;
        }

        // ---- write ( выражение { , выражение } ) ; ----
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

        // ---- { операторы } — составной оператор ----
        if (tryEat(TT::LBRACE)) {
            while (!check(TT::RBRACE) && !check(TT::EOF_TOK))
                parseStmt();
            eat(TT::RBRACE);
            return;
        }

        // ---- выражение ; ----
        parseExpr();
        emit({ OpCode::POP });
        eat(TT::SEMICOLON);
    }

public:
    Parser(std::vector<Token> tokens, SymTable& s, std::vector<PolizOp>& c)
        : toks(std::move(tokens)), pos(0), sym(s), code(c) {}

    void parse() {
        eat(TT::KW_PROGRAM);
        eat(TT::LBRACE);

        // Описания переменных
        while (check(TT::KW_INT) || check(TT::KW_STRING) || check(TT::KW_REAL))
            parseDecl();

        // Операторы
        while (!check(TT::RBRACE) && !check(TT::EOF_TOK))
            parseStmt();

        eat(TT::RBRACE);

        // Проверяем что все forward-goto получили свои метки
        if (!fwdGotos.empty()) {
            std::string missing = fwdGotos.begin()->first;
            throw std::runtime_error(
                "goto на несуществующую метку '" + missing + "'");
        }
    }
};


// ============================================================
// ЧАСТЬ 6. ИНТЕРПРЕТАТОР (СТЕКОВАЯ ВИРТУАЛЬНАЯ МАШИНА)
//
// Выполняет программу в ПОЛИЗ.
// pc — индекс текущей инструкции.
// st — стек значений.
// ============================================================

class Interpreter {
    const std::vector<PolizOp>& code;
    SymTable&                   sym;
    std::stack<Val>             st;

    Val pop() {
        if (st.empty())
            throw std::runtime_error("Внутренняя ошибка: стек пуст");
        Val v = st.top(); st.pop(); return v;
    }
    void push(Val v) { st.push(v); }

    long long toBool(const Val& v) {
        if (std::holds_alternative<long long>(v)) return asInt(v) != 0 ? 1LL : 0LL;
        if (std::holds_alternative<double>(v))    return asReal(v) != 0.0 ? 1LL : 0LL;
        return asStr(v).empty() ? 0LL : 1LL;
    }

    Val arith(const Val& L, const Val& R, OpCode op) {
        // Строковая конкатенация
        if (op == OpCode::ADD &&
            std::holds_alternative<std::string>(L) &&
            std::holds_alternative<std::string>(R))
            return asStr(L) + asStr(R);

        // Если хотя бы один real → результат real
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
        // Оба целые
        long long l = asInt(L), r = asInt(R);
        switch (op) {
            case OpCode::ADD: return l + r;
            case OpCode::SUB: return l - r;
            case OpCode::MUL: return l * r;
            case OpCode::DIV:
                if (r == 0) throw std::runtime_error("Деление на ноль");
                return l / r;
            case OpCode::MOD:
                if (r == 0) throw std::runtime_error("Деление на ноль (остаток)");
                return l % r;
            default:
                throw std::runtime_error("Неизвестная арифметическая операция");
        }
    }

    Val compare(const Val& L, const Val& R, OpCode op) {
        long long res = 0;
        if (std::holds_alternative<std::string>(L)) {
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
                    Val v = st.top();   // не снимаем (результат присваивания = значение)
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
                case OpCode::MOD: {
                    Val R = pop(); Val L = pop();
                    push(arith(L, R, op.code));
                    break;
                }

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

                case OpCode::AND: {
                    // Обычные вычисления: оба операнда уже на стеке
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
                    continue;

                case OpCode::JZ: {
                    Val v = pop();
                    if (toBool(v) == 0) { pc = static_cast<int>(op.ival); continue; }
                    break;
                }

                case OpCode::JNZ: {
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


// ============================================================
// ЧАСТЬ 7. ГЛАВНАЯ ФУНКЦИЯ
// ============================================================

int main(int argc, char* argv[]) {
    std::string src;

    if (argc >= 2) {
        std::ifstream f(argv[1]);
        if (!f) {
            std::cerr << "Ошибка: не удалось открыть файл '"
                      << argv[1] << "'" << std::endl;
            return 1;
        }
        std::ostringstream ss; ss << f.rdbuf();
        src = ss.str();
    } else {
        std::ostringstream ss; ss << std::cin.rdbuf();
        src = ss.str();
    }

    try {
        // Фаза 1: лексический анализ
        Lexer lexer(src);
        std::vector<Token> tokens = lexer.tokenize();

        // Фаза 2: синтаксический + семантический анализ + генерация ПОЛИЗ
        SymTable             sym;
        std::vector<PolizOp> code;
        Parser parser(tokens, sym, code);
        parser.parse();

        // Фаза 3: интерпретация
        Interpreter interp(code, sym);
        interp.run();

    } catch (const std::exception& e) {
        std::cerr << std::endl << "[ОШИБКА] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
