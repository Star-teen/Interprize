#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <stack>
#include <cctype>
#include <algorithm>
#include <climits>

using namespace std;

// Конфигурация (защита от уязвимостей)
const int MAX_IDENT_LEN = 255;           // максимальная длина идентификатора
const int MAX_COMMENT_LEN = 65536;       // максимальная длина комментария
const int MAX_RECURSION_DEPTH = 1000;    // максимальная глубина рекурсии парсера
const int MAX_STACK_DEPTH = 10000;       // максимальная глубина стека значений
const int MAX_EXECUTION_STEPS = 10000000;// максимальное число шагов интерпретации

// Типы лексем
enum type_of_lex {
    LEX_NULL,
    
    // Служебные слова
    LEX_AND, LEX_BEGIN, LEX_DO, LEX_ELSE, LEX_END,
    LEX_IF, LEX_INT, LEX_NOT, LEX_OR, LEX_PROGRAM,
    LEX_READ, LEX_THEN, LEX_WHILE, LEX_WRITE,
    LEX_FOR, LEX_STEP, LEX_UNTIL, LEX_GOTO,
    LEX_REAL, LEX_STRING,
    
    // Разделители и операторы
    LEX_SEMICOLON, LEX_COMMA, LEX_ASSIGN, LEX_COLON,
    LEX_LPAREN, LEX_RPAREN, LEX_LBRACE, LEX_RBRACE,
    LEX_EQ, LEX_LT, LEX_GT, LEX_LEQ, LEX_NEQ, LEX_GEQ,
    LEX_PLUS, LEX_MINUS, LEX_TIMES, LEX_SLASH,
    
    // Операнды
    LEX_NUM, LEX_REAL_NUM, LEX_STRING_LIT, LEX_ID,
    
    // Служебные лексемы ПОЛИЗа
    POLIZ_LABEL, POLIZ_ADDRESS, POLIZ_GO, POLIZ_FGO, POLIZ_UMINUS,
    
    LEX_FIN
};

const int TOKEN_OFFSET = LEX_SEMICOLON;  // первый токен разделителя

// Лексема

class Lex {
    type_of_lex t_lex;
    int v_lex;
public:
    Lex(type_of_lex t = LEX_NULL, int v = 0) : t_lex(t), v_lex(v) {}
    type_of_lex get_type() const { return t_lex; }
    int get_value() const { return v_lex; }
    friend ostream& operator<<(ostream& s, const Lex& l) {
        s << '(' << l.t_lex << ',' << l.v_lex << ')';
        return s;
    }
};


// Значение (для интерпретатора)

struct Value {
    type_of_lex type;   // LEX_INT, LEX_REAL, LEX_STRING
    union {
        int int_val;
        double real_val;
        string* str_val;
    } data;
    
    Value() : type(LEX_NULL) { data.int_val = 0; }
    Value(int v) : type(LEX_INT) { data.int_val = v; }
    Value(double v) : type(LEX_REAL) { data.real_val = v; }
    Value(const string& v) : type(LEX_STRING) { data.str_val = new string(v); }
    
    Value(const Value& other) {
        type = other.type;
        if (type == LEX_STRING) { data.str_val = new string(*other.data.str_val);
        } else { data = other.data;}
    }
    
    ~Value() {if (type == LEX_STRING) delete data.str_val;}
    
    Value& operator=(const Value& other) {
        if (this == &other) return *this;
        if (type == LEX_STRING) delete data.str_val;
        type = other.type;
        if (type == LEX_STRING) { data.str_val = new string(*other.data.str_val);
        } else {data = other.data;}
        return *this;
    }
};


// Идентификатор (таблица символов)

class Ident {
    string name;
    type_of_lex type;      // LEX_INT, LEX_REAL, LEX_STRING
    bool declared;         // описан ли
    bool initialized;      // проинициализирован ли
    bool is_label;         // является ли меткой
    bool label_defined;    // определена ли метка (встретилась в коде)
    int label_addr;        // адрес метки (позиция в ПОЛИЗе)
    vector<int> forward_refs; // список позиций в ПОЛИЗе для отложенных адресов
    
    // Значения переменных
    int int_val;
    double real_val;
    string* str_val;       // указатель, чтобы не копировать строки
    
public:
    Ident() : declared(false), initialized(false), is_label(false), 
              label_defined(false), label_addr(-1), type(LEX_INT), 
              int_val(0), real_val(0.0), str_val(nullptr) {}
    
    Ident(const string& n) : name(n), declared(false), initialized(false), 
              is_label(false), label_defined(false), label_addr(-1),
              type(LEX_INT), int_val(0), real_val(0.0), str_val(nullptr) {}
    
    ~Ident() { delete str_val; }
    
    // Геттеры
    const string& get_name() const { return name; }
    type_of_lex get_type() const { return type; }
    bool get_declared() const { return declared; }
    bool get_initialized() const { return initialized; }
    bool get_is_label() const { return is_label; }
    bool get_label_defined() const { return label_defined; }
    int get_label_addr() const { return label_addr; }
    const vector<int>& get_forward_refs() const { return forward_refs; }
    
    // Сеттеры
    void put_type(type_of_lex t) { type = t; }
    void put_declared() { declared = true; }
    void put_initialized() { initialized = true; }
    void put_is_label(bool b) { is_label = b; }
    void put_label_defined(bool b) { label_defined = b; }
    void put_label_addr(int addr) { label_addr = addr; }
    void add_forward_ref(int pos) { forward_refs.push_back(pos); }
    void clear_forward_refs() { forward_refs.clear(); }
    
    // Значения
    int get_int() const { return int_val; }
    void put_int(int v) { int_val = v; initialized = true; }
    
    double get_real() const { return real_val; }
    void put_real(double v) { real_val = v; initialized = true; }
    
    string* get_str() const { return str_val; }
    void put_str(const string& v) { 
        if (str_val) delete str_val;
        str_val = new string(v); 
        initialized = true;
    }
};

vector<Ident> TID;  // таблица идентификаторов


// Поиск/добавление идентификатора в таблицу

int put(const string& buf) {
    auto it = find_if(TID.begin(), TID.end(), [&buf](const Ident& id) { return id.get_name() == buf; });
    if (it != TID.end()) return it - TID.begin();
    TID.push_back(Ident(buf));
    return TID.size() - 1;
}

// Прототипы функций (объявления)
int put_string(const string& s);
string get_string_value(int idx);
int put_real(double d);
double get_real_value(int idx);

// Лексический анализатор (Scanner)

class Scanner {
    FILE* fp;
    char c;
    int comment_len;
    
    int look(const string& buf, const char* list[]) {
        int i = 0;
        while (list[i]) {
            if (buf == list[i]) return i; 
            i++;
        }
        return 0;
    }
    
    void gc() {
        int ch = fgetc(fp);
        if (ch == EOF) { c = '@';
        } else { c = (char)ch; }
    }
    
    char peek() {
        int ch = fgetc(fp);
        if (ch == EOF) return '@';
        ungetc(ch, fp);
        return (char)ch;
    }
    
public:
    static const char* TW[];
    static const char* TD[];
    
    Scanner(const char* program) : comment_len(0) {
        fp = fopen(program, "r");
        if (fp == NULL) throw "Can't open file";
    }
    
    ~Scanner() {if (fp) fclose(fp);}
    
    Lex get_lex();
};

const char* Scanner::TW[] = {
    "and", "begin", "do", "else", "end", "for", "goto", "if", "int",
    "not", "or", "program", "read", "real", "step", "string", "then",
    "until", "while", "write", NULL
};

const char* Scanner::TD[] = {
    "@", ";", ",", "=", "==", "(", ")", "{", "}", "<", ">", "+", "-",
    "*", "/", "<=", "!=", ">=", "/*", "*/", NULL
};

Lex Scanner::get_lex() {
    enum state { H, IDENT, NUMB, REAL, STRING, COM, ALE, NEQ };
    state CS = H;
    string buf;
    int d = 0, j;
    bool has_dot = false;
    
    do {
        gc();
        
        switch (CS) {
            case H:
                if (c == ' ' || c == '\n' || c == '\r' || c == '\t');
                else if (isalpha(c)) {
                    buf.clear();
                    buf.push_back(c);
                    CS = IDENT;
                }
                else if (isdigit(c)) {
                    d = c - '0';
                    has_dot = false;
                    CS = NUMB;
                }
                else if (c == '/') {
                    if (peek() == '*') {
                        gc(); // прочитать '*'
                        comment_len = 0;
                        CS = COM;
                    } else {
                        buf = "/";
                        j = look(buf, TD);
                        return Lex((type_of_lex)(j + TOKEN_OFFSET), j);
                    }
                }
                else if (c == '"') {
                    buf.clear();
                    CS = STRING;
                }
                else if (c == '<' || c == '>') {
                    buf.clear();
                    buf.push_back(c);
                    CS = ALE;
                }
                else if (c == '@') { return Lex(LEX_FIN);}
                else if (c == '!') {
                    buf.clear();
                    buf.push_back(c);
                    CS = NEQ;
                }
                else {
                    buf.clear();
                    buf.push_back(c);
                    if ((j = look(buf, TD))) { return Lex((type_of_lex)(j + TOKEN_OFFSET), j);}
                    else {
                        string err = "Unknown character: ";
                        err += c;
                        throw err.c_str();
                    }
                }
                break;
                
            case IDENT:
                if (isalpha(c) || isdigit(c)) { buf.push_back(c);}
                else {
                    ungetc(c, fp);
                    if ((j = look(buf, TW))) { return Lex((type_of_lex)j, j);}
                    else {
                        j = put(buf);
                        return Lex(LEX_ID, j);
                    }
                }
                break;
                
            case NUMB:
                if (isdigit(c)) {d = d * 10 + (c - '0');}
                else if (c == '.') {
                    has_dot = true;
                    CS = REAL;
                } else {
                    ungetc(c, fp);
                    return Lex(LEX_NUM, d);
                }
                break;
                
            case REAL: {
                double real_val = d;
                double frac = 0.0;
                double divisor = 1.0;
                while (isdigit(c)) {
                    frac = frac * 10 + (c - '0');
                    divisor *= 10;
                    gc();
                }
                real_val = real_val + frac / divisor;
                if (c == '.') {throw "Invalid real number: multiple dots";}
                ungetc(c, fp);
                int idx = put_real(real_val);
                return Lex(LEX_REAL_NUM, idx);
            }
            
            case COM:
                if (c == '*' && peek() == '/') {
                    gc(); // прочитать '/'
                    CS = H;
                    comment_len = 0;
                } else if (c == '@') { throw "Unclosed comment";
                } else {
                    comment_len++;
                    if (comment_len > MAX_COMMENT_LEN) {throw "Comment too long";}}
                break;
                
            case STRING:
                if (c == '"') {
                    int idx = put_string(buf);
                    return Lex(LEX_STRING_LIT, idx);
                } else if (c == '\\') {
                    gc();
                    switch (c) {
                        case 'n': buf += '\n'; break;
                        case 't': buf += '\t'; break;
                        case '"': buf += '"'; break;
                        case '\\': buf += '\\'; break;
                        default: buf += c; break;
                    }
                } else if (c == '@') { throw "Unclosed string literal";
                } else { buf += c; }
                break;
                
            case ALE:
                if (c == '=') {
                    buf.push_back(c);
                    j = look(buf, TD);
                    return Lex((type_of_lex)(j + TOKEN_OFFSET), j);
                }
                else {
                    ungetc(c, fp);
                    j = look(buf, TD);
                    return Lex((type_of_lex)(j + TOKEN_OFFSET), j);
                }
                break;
                
            case NEQ:
                if (c == '=') {
                    buf.push_back(c);
                    j = look(buf, TD);
                    return Lex(LEX_NEQ, j);
                } else { throw "Invalid '!' usage: expected '!='";}
                break;
        }
    } while (true);
}


// Синтаксический и семантический анализатор с генерацией ПОЛИЗа

// Таблицы для строк и вещественных чисел
vector<string> string_table;
vector<double> real_table;

int put_string(const string& s) {
    string_table.push_back(s);
    return string_table.size() - 1;
}

string get_string_value(int idx) {
    if (idx < 0 || idx >= (int)string_table.size()) return "";
    return string_table[idx];
}

int put_real(double d) {
    real_table.push_back(d);
    return real_table.size() - 1;
}

double get_real_value(int idx) {
    if (idx < 0 || idx >= (int)real_table.size()) return 0.0;
    return real_table[idx];
}

// Вспомогательная структура для хранения констант при инициализации
struct InitValue {
    type_of_lex type;
    int int_val;
    double real_val;
    string* str_val;
    
    InitValue() : type(LEX_NULL), int_val(0), real_val(0.0), str_val(nullptr) {}
    ~InitValue() { delete str_val; }
};

class Parser {
private:
    Lex curr_lex;
    type_of_lex c_type;
    int c_val;
    Scanner scan;
    stack<type_of_lex> st_lex;
    int recursion_depth;
    int temp_counter;      // для генерации временных переменных
    
    void gl() {
        curr_lex = scan.get_lex();
        c_type = curr_lex.get_type();
        c_val = curr_lex.get_value();
    }
    
    void enter() {if (++recursion_depth > MAX_RECURSION_DEPTH) throw "Parser recursion depth exceeded";}
    
    void leave() { recursion_depth--; }
    
    template<typename T>
    void from_st(T& st, typename T::value_type& x) {
        if (st.empty()) throw "Semantic stack underflow";
        x = st.top();
        st.pop();
    }
    
    // Проверка идентификатора (для использования в выражениях)
    void check_id() {
        if (!TID[c_val].get_declared()) throw "Variable not declared";
        st_lex.push(TID[c_val].get_type());
    }
    
    void check_id_in_read() {if (!TID[c_val].get_declared())throw "Variable not declared in read";}
    
    // Проверка типов для бинарной операции
    void check_op() {
        type_of_lex t1, t2, op, result_type;
        
        if (st_lex.size() < 3) throw "Not enough operands on semantic stack";
        
        from_st(st_lex, t2);
        from_st(st_lex, op);
        from_st(st_lex, t1);
        
        // Арифметика
        if (op == LEX_PLUS || op == LEX_MINUS || op == LEX_TIMES || op == LEX_SLASH) {
            if ((t1 == LEX_INT || t1 == LEX_REAL) && (t2 == LEX_INT || t2 == LEX_REAL)) {
                result_type = (t1 == LEX_REAL || t2 == LEX_REAL) ? LEX_REAL : LEX_INT;
            }
            else if (t1 == LEX_STRING && t2 == LEX_STRING && op == LEX_PLUS) {
                result_type = LEX_STRING;  // конкатенация строк
            }
            else {throw "Arithmetic operation requires numeric operands";}
        }
        // Сравнения
        else if (op == LEX_EQ || op == LEX_NEQ || op == LEX_LT || op == LEX_GT || op == LEX_LEQ || op == LEX_GEQ) {
            if ((t1 == LEX_INT || t1 == LEX_REAL) && (t2 == LEX_INT || t2 == LEX_REAL)) {
                result_type = LEX_INT;  // 0 или 1
            }
            else if (t1 == LEX_STRING && t2 == LEX_STRING) {
                result_type = LEX_INT;
            }
            else {throw "Comparison requires compatible types";}
        }
        // Логические операции (операнды - целые, результат - целый)
        else if (op == LEX_AND || op == LEX_OR) {
            if (t1 == LEX_INT && t2 == LEX_INT) { 
                result_type = LEX_INT;
            } else { throw "Logical operation requires integer operands"; }
        }
        else {throw "Unknown operator in check_op";}
        
        st_lex.push(result_type);
        poliz.push_back(Lex(op));
    }
    
    // Проверка условия (должно быть целым, 0 = false)
    void check_condition() {
        if (st_lex.empty()) throw "Not enough operands for condition";
        if (st_lex.top() != LEX_INT) throw "Condition must be integer (0=false, non-0=true)";
        st_lex.pop();
    }
    
    // Проверка типов для присваивания
    void check_assign() {
        type_of_lex t1, t2;
        if (st_lex.size() < 2) throw "Not enough operands for assignment";
        from_st(st_lex, t2);  // тип правого операнда
        from_st(st_lex, t1);  // тип левого операнда (переменной)
        
        // Допустимые преобразования: int -> real
        if (t1 == LEX_REAL && t2 == LEX_INT) { st_lex.push(LEX_REAL); }
        else if (t1 == t2) { st_lex.push(t1); }
        else { throw "Assignment type mismatch"; }
    }
    
    // Добавление переменной в TID
    void add_variable(const string& name, type_of_lex var_type, bool has_init, const InitValue& init_val) {
        int idx = put(name);
        if (TID[idx].get_declared()) { throw "Variable '" + name + "' declared twice";}
        TID[idx].put_type(var_type);
        TID[idx].put_declared();
        
        if (has_init) {
            if (var_type == LEX_REAL && init_val.type == LEX_INT) {
                TID[idx].put_real((double)init_val.int_val);
            }
            else if (var_type == init_val.type) {
                switch (var_type) {
                    case LEX_INT: TID[idx].put_int(init_val.int_val); break;
                    case LEX_REAL: TID[idx].put_real(init_val.real_val); break;
                    case LEX_STRING: TID[idx].put_str(*init_val.str_val); break;
                    default: break;
                }
            }
            else {throw "Initializer type mismatch for " + name;}
            TID[idx].put_initialized();
        }
    }
    
    // Создание временной переменной
    int create_temp_var(type_of_lex type) {
        string name = "__temp_" + to_string(temp_counter++);
        int idx = put(name);
        TID[idx].put_declared();
        TID[idx].put_type(type);
        return idx;
    }
    
public:
    vector<Lex> poliz;
    
    Parser(const char* program) : scan(program), recursion_depth(0), temp_counter(0) {}
    
    void analyze() {
        gl();
        P();
        if (c_type != LEX_FIN) throw "Expected end of program";
    //    cout << "Syntax OK, POLIZ size: " << poliz.size() << endl;
    }
    
    // Грамматика
    void P();   // program
    void D();   // declaration
    void S();   // statement
    void E();   // expression
    void E1();  // + - or
    void T();   // * /
    void F();   // factor
};


// Реализация правил грамматики


void Parser::P() {
    if (c_type == LEX_PROGRAM) { gl();
    } else { throw "Expected 'program'"; }
    
    // Описания: { описание ; }
    if (c_type == LEX_LBRACE) {
        gl();
        while (c_type != LEX_RBRACE && c_type != LEX_FIN) {
            D();
            if (c_type != LEX_SEMICOLON) throw "Expected ';' after declaration";
            gl();
        }
        if (c_type == LEX_RBRACE) gl();
        else throw "Expected '}' after declarations";
    } else { throw "Expected '{' after program"; }
    
    // Операторы: { оператор }
    if (c_type == LEX_LBRACE) {
        gl();
        while (c_type != LEX_RBRACE && c_type != LEX_FIN) { S(); }
        if (c_type == LEX_RBRACE) gl();
        else throw "Expected '}' after statements";
    } else { throw "Expected '{' before statements"; }
}

void Parser::D() {
    type_of_lex var_type;
    if (c_type == LEX_INT) { var_type = LEX_INT;} 
    else if (c_type == LEX_REAL) { var_type = LEX_REAL; } 
    else if (c_type == LEX_STRING) { var_type = LEX_STRING; } 
    else { throw "Expected type (int, real, string)";}
    gl();
    
    do {
        if (c_type != LEX_ID) throw "Expected identifier";
        string var_name = TID[c_val].get_name();
        gl();
        
        bool has_init = false;
        InitValue init_val;
        
        if (c_type == LEX_ASSIGN) {
            has_init = true;
            gl();
            
            if (var_type == LEX_INT) {
                if (c_type == LEX_NUM) {
                    init_val.type = LEX_INT;
                    init_val.int_val = c_val;
                    gl();
                } else if (c_type == LEX_REAL_NUM) {
                    init_val.type = LEX_INT;
                    init_val.int_val = (int)get_real_value(c_val);
                    gl();
                } else { throw "Expected integer constant"; }
            }
            else if (var_type == LEX_REAL) {
                if (c_type == LEX_NUM) {
                    init_val.type = LEX_INT;
                    init_val.int_val = c_val;
                    gl();
                } else if (c_type == LEX_REAL_NUM) {
                    init_val.type = LEX_REAL;
                    init_val.real_val = get_real_value(c_val);
                    gl();
                } else { throw "Expected numeric constant"; }
            }
            else if (var_type == LEX_STRING) {
                if (c_type == LEX_STRING_LIT) {
                    init_val.type = LEX_STRING;
                    init_val.str_val = new string(get_string_value(c_val));
                    gl();
                } else { throw "Expected string constant"; }
            }
        }
        
        add_variable(var_name, var_type, has_init, init_val);
        
    } while (c_type == LEX_COMMA);
}

void Parser::S() {
    //Метка: идентификатор : оператор
    if (c_type == LEX_ID) {
        // Проверяем, что следующий токен - ':'
        // Сохранили позицию
        int saved_pos = poliz.size();
        type_of_lex saved_type = c_type;
        int saved_val = c_val;
        Lex saved_lex = curr_lex;
        
        // Временный просмотр вперёд
        gl(); // читаем после ID
        if (c_type == LEX_COLON) { // Если : Это метка
            string label_name = TID[saved_val].get_name();
            int label_idx = saved_val;
            gl(); // прочитать ':'
            
            // Добавить или найти метку
            if (label_idx >= (int)TID.size()) { label_idx = put(label_name); }
            
            if (TID[label_idx].get_is_label() && TID[label_idx].get_label_defined()) { throw "Label '" + label_name + "' already defined"; }
            
            TID[label_idx].put_is_label(true);
            TID[label_idx].put_label_defined(true);
            TID[label_idx].put_label_addr(poliz.size());
            
            // Заполнить forward-ссылки
            for (int pos : TID[label_idx].get_forward_refs()) { poliz[pos] = Lex(POLIZ_LABEL, poliz.size()); }
            TID[label_idx].clear_forward_refs();
            
            poliz.push_back(Lex(POLIZ_LABEL, poliz.size()));
            S();  // разбор помеченного оператора
            return;
        } else {
            // Не метка — возвращаемся назад
            c_type = saved_type;
            c_val = saved_val;
            curr_lex = saved_lex;
        }
    }
    
    // === Присваивание ===
    if (c_type == LEX_ID) {
        check_id();
        poliz.push_back(Lex(POLIZ_ADDRESS, c_val));
        gl();
        
        if (c_type != LEX_ASSIGN) throw "Expected '='";
        gl();
        
        E();
        check_assign();
        poliz.push_back(Lex(LEX_ASSIGN));
        return;
    }
    
    //  if (expr) stmt [else stmt] 
    if (c_type == LEX_IF) {
        gl();
        if (c_type != LEX_LPAREN) throw "Expected '(' after if";
        gl();
        
        int false_label = poliz.size();
        poliz.push_back(Lex());  // место для !F
        E();
        check_condition();
        if (c_type != LEX_RPAREN) throw "Expected ')' after condition";
        gl();
        
        poliz.push_back(Lex(POLIZ_FGO));
        poliz[false_label] = Lex(POLIZ_LABEL, poliz.size());
        
        S();  // then-ветка
        
        if (c_type == LEX_ELSE) {
            gl();
            int go_label = poliz.size();
            poliz.push_back(Lex());  // место для GO
            poliz.push_back(Lex(POLIZ_GO));
            poliz[go_label] = Lex(POLIZ_LABEL, poliz.size());
            S();  // else-ветка
        }
        return;
    }
    
    // while (expr) stmt
    if (c_type == LEX_WHILE) {
        int start_label = poliz.size();
        gl();
        if (c_type != LEX_LPAREN) throw "Expected '(' after while";
        gl();
        
        int cond_label = poliz.size();
        poliz.push_back(Lex());  // место для !F
        E();
        check_condition();
        if (c_type != LEX_RPAREN) throw "Expected ')' after condition";
        gl();
        
        poliz.push_back(Lex(POLIZ_FGO));
        poliz[cond_label] = Lex(POLIZ_LABEL, poliz.size());
        
        S();  // тело
        
        poliz.push_back(Lex(POLIZ_GO));
        poliz.push_back(Lex(POLIZ_LABEL, start_label));
        return;
    }
    
    // === for I = E1 step E2 until E3 do stmt ===
    if (c_type == LEX_FOR) {
        gl();
        if (c_type != LEX_ID) throw "Expected identifier after for";
        int var_idx = c_val;
        string var_name = TID[var_idx].get_name();
        gl();
        
        if (!TID[var_idx].get_declared()) throw "Variable '" + var_name + "' not declared";
        if (TID[var_idx].get_type() != LEX_INT) throw "For loop variable must be integer";
        if (c_type != LEX_ASSIGN) throw "Expected '=' after for variable";
        gl();
        
        // 1. Инициализация: I = E1
        poliz.push_back(Lex(POLIZ_ADDRESS, var_idx));
        E();
        check_assign();
        poliz.push_back(Lex(LEX_ASSIGN));
        
        if (c_type != LEX_STEP) throw "Expected 'step'";
        gl();
        
        // 2. Шаг: сохраняем во временную переменную
        int step_temp = create_temp_var(LEX_INT);
        poliz.push_back(Lex(POLIZ_ADDRESS, step_temp));
        E();
        if (st_lex.top() != LEX_INT) throw "Step must be integer";
        st_lex.pop();
        poliz.push_back(Lex(LEX_ASSIGN));
        
        if (c_type != LEX_UNTIL) throw "Expected 'until'";
        gl();
        
        // 3. Конечное значение: сохраняем во временную переменную
        int limit_temp = create_temp_var(LEX_INT);
        poliz.push_back(Lex(POLIZ_ADDRESS, limit_temp));
        E();
        if (st_lex.top() != LEX_INT) throw "Until value must be integer";
        st_lex.pop();
        poliz.push_back(Lex(LEX_ASSIGN));
        
        if (c_type != LEX_DO) throw "Expected 'do'";
        gl();
        
        // 4. Метка начала цикла
        int start_label = poliz.size();
        
        // 5. Проверка условия выхода (упрощённо: считаем шаг положительным)
        //    Для отрицательного шага нужно использовать >=
        poliz.push_back(Lex(POLIZ_ADDRESS, var_idx));
        poliz.push_back(Lex(POLIZ_ADDRESS, limit_temp));
        poliz.push_back(Lex(LEX_LEQ));
        
        int exit_label = poliz.size();
        poliz.push_back(Lex());  // место для !F
        poliz.push_back(Lex(POLIZ_FGO));
        
        // 6. Тело
        S();
        
        // 7. Инкремент: I = I + step
        poliz.push_back(Lex(POLIZ_ADDRESS, var_idx));
        poliz.push_back(Lex(POLIZ_ADDRESS, var_idx));
        poliz.push_back(Lex(POLIZ_ADDRESS, step_temp));
        poliz.push_back(Lex(LEX_PLUS));
        poliz.push_back(Lex(LEX_ASSIGN));
        
        // 8. Переход на начало
        poliz.push_back(Lex(POLIZ_GO));
        poliz.push_back(Lex(POLIZ_LABEL, start_label));
        
        // 9. Метка выхода
        poliz[exit_label] = Lex(POLIZ_LABEL, poliz.size());
        return;
    }
    
    // === goto label; ===
    if (c_type == LEX_GOTO) {
        gl();
        if (c_type != LEX_ID) throw "Expected label after goto";
        string label_name = TID[c_val].get_name();
        int label_idx = c_val;
        gl();
        if (c_type != LEX_SEMICOLON) throw "Expected ';' after goto";
        gl();
        
        if (label_idx >= (int)TID.size()) {label_idx = put(label_name);}
        
        if (!TID[label_idx].get_is_label()) {
            TID[label_idx].put_is_label(true);
            TID[label_idx].put_label_defined(false);
        }
        
        int go_pos = poliz.size();
        poliz.push_back(Lex(POLIZ_GO));
        poliz.push_back(Lex());  // место для адреса
        
        if (TID[label_idx].get_label_defined()) { 
            poliz[go_pos + 1] = Lex(POLIZ_LABEL, TID[label_idx].get_label_addr());
        } else {TID[label_idx].add_forward_ref(go_pos + 1);}
        return;
    }
    
    // === read(id); ===
    if (c_type == LEX_READ) {
        gl();
        if (c_type != LEX_LPAREN) throw "Expected '(' after read";
        gl();
        if (c_type != LEX_ID) throw "Expected identifier in read";
        check_id_in_read();
        poliz.push_back(Lex(POLIZ_ADDRESS, c_val));
        gl();
        if (c_type != LEX_RPAREN) throw "Expected ')' after read";
        gl();
        if (c_type != LEX_SEMICOLON) throw "Expected ';' after read";
        gl();
        poliz.push_back(Lex(LEX_READ));
        return;
    }
    
    // === write(expr, expr, ...); ===
    if (c_type == LEX_WRITE) {
        gl();
        if (c_type != LEX_LPAREN) throw "Expected '(' after write";
        gl();
        do {
            E();
            st_lex.pop();
            poliz.push_back(Lex(LEX_WRITE));
        } while (c_type == LEX_COMMA);
        if (c_type != LEX_RPAREN) throw "Expected ')' after write list";
        gl();
        if (c_type != LEX_SEMICOLON) throw "Expected ';' after write";
        gl();
        return;
    }
    
    // === Составной оператор { ... } ===
    if (c_type == LEX_LBRACE) {
        gl();
        while (c_type != LEX_RBRACE && c_type != LEX_FIN) S();
        if (c_type == LEX_RBRACE) gl();
        else throw "Expected '}'";
        return;
    }
    
    // === Пустой оператор ===
    if (c_type == LEX_SEMICOLON) {
        gl();
        return;
    }
    
    // === Оператор-выражение ===
    E();
    if (c_type != LEX_SEMICOLON) throw "Expected ';' after expression statement";
    gl();
}

// Выражения с учётом приоритетов
void Parser::E() {
    E1();
    if (c_type == LEX_EQ || c_type == LEX_LT || c_type == LEX_GT || c_type == LEX_LEQ || c_type == LEX_NEQ || c_type == LEX_GEQ) {
        type_of_lex op = c_type;
        st_lex.push(op);
        gl();
        E1();
        check_op();
    }
}

void Parser::E1() {
    T();
    while (c_type == LEX_PLUS || c_type == LEX_MINUS || c_type == LEX_OR) {
        type_of_lex op = c_type;
        st_lex.push(op);
        gl();
        T();
        check_op();
    }
}

void Parser::T() {
    F();
    while (c_type == LEX_TIMES || c_type == LEX_SLASH) {
        type_of_lex op = c_type;
        st_lex.push(op);
        gl();
        F();
        check_op();
    }
}

void Parser::F() {
    if (c_type == LEX_ID) {
        check_id();
        poliz.push_back(curr_lex);
        gl();
    }
    else if (c_type == LEX_NUM) {
        st_lex.push(LEX_INT);
        poliz.push_back(curr_lex);
        gl();
    }
    else if (c_type == LEX_REAL_NUM) {
        st_lex.push(LEX_REAL);
        poliz.push_back(curr_lex);
        gl();
    }
    else if (c_type == LEX_STRING_LIT) {
        st_lex.push(LEX_STRING);
        poliz.push_back(curr_lex);
        gl();
    }
    else if (c_type == LEX_NOT) {
        gl();
        enter();
        F();
        leave();
        if (st_lex.top() != LEX_INT) throw "'not' requires integer operand";
        poliz.push_back(Lex(LEX_NOT));
    }
    else if (c_type == LEX_MINUS) {
        gl();
        F();
        if (st_lex.top() != LEX_INT && st_lex.top() != LEX_REAL) throw "Unary minus requires numeric operand";
        poliz.push_back(Lex(POLIZ_UMINUS));
    }
    else if (c_type == LEX_LPAREN) {
        gl();
        enter();
        E();
        leave();
        if (c_type != LEX_RPAREN) throw "Expected ')'";
        gl();
    } else {throw "Unexpected token in factor";}
}


// Интерпретатор ПОЛИЗа (Executer) — ИСПРАВЛЕННЫЙ

class Executer {
private:
    stack<Value> args;
    int steps;

    void check_stack_depth() {if (args.size() > MAX_STACK_DEPTH) throw "Stack overflow";}
    
    void check_bounds(int idx, int size, const string& context) {
        if (idx < 0 || idx >= size) throw "Index out of bounds in " + context;}
    
    // Проверки переполнения для целых чисел
    void check_overflow_add(int a, int b) {
        if ((b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b))
            throw "Integer overflow in addition";
        }
    
    void check_overflow_mul(int a, int b) {
        if (a != 0 && b != 0) {
            if ((a > 0 && b > 0 && a > INT_MAX / b) ||
                (a < 0 && b < 0 && a > INT_MAX / b) ||
                (a > 0 && b < 0 && b < INT_MIN / a) ||
                (a < 0 && b > 0 && a < INT_MIN / b))
                throw "Integer overflow in multiplication";}
    }
    
    void check_overflow_sub(int a, int b) {
        if ((b > 0 && a < INT_MIN + b) ||
            (b < 0 && a > INT_MAX + b))
            throw "Integer overflow in subtraction";
    }
    
    template<typename T>
    void from_st(T& st, typename T::value_type& x) {
        if (st.empty()) throw "Execution stack underflow";
        x = st.top();
        st.pop();
    }
    
public:
    Executer() : steps(0) {}
    
    void execute(vector<Lex>& poliz) {
        int index = 0;
        int size = poliz.size();
        steps = 0;
        
        while (index < size) {
            steps++;
            if (steps > MAX_EXECUTION_STEPS) throw "Execution limit exceeded";
            
            Lex pc_el = poliz[index];
            
            switch (pc_el.get_type()) {
                // Операнды
                case LEX_NUM:
                    check_stack_depth();
                    args.push(Value(pc_el.get_value()));
                    break;
                    
                case LEX_REAL_NUM: {
                    double val = get_real_value(pc_el.get_value());
                    check_stack_depth();
                    args.push(Value(val));
                    break;
                }
                
                case LEX_STRING_LIT: {
                    string val = get_string_value(pc_el.get_value());
                    check_stack_depth();
                    args.push(Value(val));
                    break;
                }
                
                case POLIZ_ADDRESS:
                case POLIZ_LABEL:
                    check_stack_depth();
                    args.push(Value(pc_el.get_value()));
                    break;
                
                // Переменная (взять значение)
                case LEX_ID: {
                    int idx = pc_el.get_value();
                    check_bounds(idx, TID.size(), "LEX_ID");
                    if (!TID[idx].get_initialized()) throw "Indefinite identifier: " + TID[idx].get_name();
                    check_stack_depth();
                    if (TID[idx].get_type() == LEX_INT)         args.push(Value(TID[idx].get_int()));
                    else if (TID[idx].get_type() == LEX_REAL)   args.push(Value(TID[idx].get_real()));
                    else if (TID[idx].get_type() == LEX_STRING) args.push(Value(*TID[idx].get_str()));
                    else throw "Unknown variable type";
                    break;
                }
                
                // Унарные операции
                case POLIZ_UMINUS: {
                    Value v;
                    from_st(args, v);
                    if (v.type == LEX_INT) {
                        check_stack_depth();
                        args.push(Value(-v.data.int_val));
                    } else if (v.type == LEX_REAL) {
                        check_stack_depth();
                        args.push(Value(-v.data.real_val));
                    } else {throw "Unary minus requires numeric operand"; }
                    break;
                }
                
                case LEX_NOT: {
                    Value v;
                    from_st(args, v);
                    if (v.type != LEX_INT) throw "'not' requires integer";
                    check_stack_depth();
                    args.push(Value(v.data.int_val ? 0 : 1));
                    break;
                }
                
                // Логические операции
                case LEX_OR: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    if (a.type != LEX_INT || b.type != LEX_INT) throw "'or' requires integer operands";
                    check_stack_depth();
                    args.push(Value((a.data.int_val || b.data.int_val) ? 1 : 0));
                    break;
                }
                
                case LEX_AND: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    if (a.type != LEX_INT || b.type != LEX_INT) throw "'and' requires integer operands";
                    check_stack_depth();
                    args.push(Value((a.data.int_val && b.data.int_val) ? 1 : 0));
                    break;
                }
                
                // Арифметика
                case LEX_PLUS: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    
                    if (a.type == LEX_INT && b.type == LEX_INT) {
                        check_overflow_add(a.data.int_val, b.data.int_val);
                        check_stack_depth();
                        args.push(Value(a.data.int_val + b.data.int_val));
                    }
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) {
                        check_stack_depth();
                        args.push(Value(a.data.real_val + b.data.real_val));
                    }
                    else if (a.type == LEX_INT && b.type == LEX_REAL) {
                        check_stack_depth();
                        args.push(Value((double)a.data.int_val + b.data.real_val));
                    }
                    else if (a.type == LEX_REAL && b.type == LEX_INT) {
                        check_stack_depth();
                        args.push(Value(a.data.real_val + (double)b.data.int_val));
                    }
                    else if (a.type == LEX_STRING && b.type == LEX_STRING) {
                        check_stack_depth();
                        args.push(Value(*a.data.str_val + *b.data.str_val));
                    }
                    else { throw "Invalid operands for +"; }
                    break;
                }
                
                case LEX_MINUS: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    if (a.type == LEX_INT && b.type == LEX_INT) {
                        check_overflow_sub(a.data.int_val, b.data.int_val);
                        check_stack_depth();
                        args.push(Value(a.data.int_val - b.data.int_val));
                    }
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) {
                        check_stack_depth();
                        args.push(Value(a.data.real_val - b.data.real_val));
                    }
                    else if (a.type == LEX_INT && b.type == LEX_REAL) {
                        check_stack_depth();
                        args.push(Value((double)a.data.int_val - b.data.real_val));
                    }
                    else if (a.type == LEX_REAL && b.type == LEX_INT) {
                        check_stack_depth();
                        args.push(Value(a.data.real_val - (double)b.data.int_val));
                    }
                    else { throw "Invalid operands for -"; }
                    break;
                }
                
                case LEX_TIMES: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    if (a.type == LEX_INT && b.type == LEX_INT) {
                        check_overflow_mul(a.data.int_val, b.data.int_val);
                        check_stack_depth();
                        args.push(Value(a.data.int_val * b.data.int_val));
                    }
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) {
                        check_stack_depth();
                        args.push(Value(a.data.real_val * b.data.real_val));
                    }
                    else if (a.type == LEX_INT && b.type == LEX_REAL) {
                        check_stack_depth();
                        args.push(Value((double)a.data.int_val * b.data.real_val));
                    }
                    else if (a.type == LEX_REAL && b.type == LEX_INT) {
                        check_stack_depth();
                        args.push(Value(a.data.real_val * (double)b.data.int_val));
                    }
                    else {throw "Invalid operands for *";}
                    break;
                }
                
                case LEX_SLASH: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    
                    // Проверка деления на ноль
                    if (b.type == LEX_INT && b.data.int_val == 0) throw "Division by zero";
                    if (b.type == LEX_REAL && b.data.real_val == 0.0) throw "Division by zero";
                    
                    if (a.type == LEX_INT && b.type == LEX_INT) {
                        check_stack_depth();
                        args.push(Value(a.data.int_val / b.data.int_val));
                    }
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) {
                        check_stack_depth();
                        args.push(Value(a.data.real_val / b.data.real_val));
                    }
                    else if (a.type == LEX_INT && b.type == LEX_REAL) {
                        check_stack_depth();
                        args.push(Value((double)a.data.int_val / b.data.real_val));
                    }
                    else if (a.type == LEX_REAL && b.type == LEX_INT) {
                        check_stack_depth();
                        args.push(Value(a.data.real_val / (double)b.data.int_val));
                    }
                    else { throw "Invalid operands for /"; }
                    break;
                }
                
                // Сравнения
                case LEX_EQ: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    bool res = false;
                    if (a.type == LEX_INT && b.type == LEX_INT) res = (a.data.int_val == b.data.int_val);
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) res = (a.data.real_val == b.data.real_val);
                    else if (a.type == LEX_INT && b.type == LEX_REAL) res = ((double)a.data.int_val == b.data.real_val);
                    else if (a.type == LEX_REAL && b.type == LEX_INT) res = (a.data.real_val == (double)b.data.int_val);
                    else if (a.type == LEX_STRING && b.type == LEX_STRING) res = (*a.data.str_val == *b.data.str_val);
                    else throw "Invalid operands for ==";
                    check_stack_depth();
                    args.push(Value(res ? 1 : 0));
                    break;
                }
                
                case LEX_NEQ: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    bool res = false;
                    if (a.type == LEX_INT && b.type == LEX_INT) res = (a.data.int_val != b.data.int_val);
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) res = (a.data.real_val != b.data.real_val);
                    else if (a.type == LEX_INT && b.type == LEX_REAL) res = ((double)a.data.int_val != b.data.real_val);
                    else if (a.type == LEX_REAL && b.type == LEX_INT) res = (a.data.real_val != (double)b.data.int_val);
                    else if (a.type == LEX_STRING && b.type == LEX_STRING) res = (*a.data.str_val != *b.data.str_val);
                    else throw "Invalid operands for !=";
                    check_stack_depth();
                    args.push(Value(res ? 1 : 0));
                    break;
                }
                
                case LEX_LT: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    bool res = false;
                    if (a.type == LEX_INT && b.type == LEX_INT) res = (a.data.int_val < b.data.int_val);
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) res = (a.data.real_val < b.data.real_val);
                    else if (a.type == LEX_INT && b.type == LEX_REAL) res = ((double)a.data.int_val < b.data.real_val);
                    else if (a.type == LEX_REAL && b.type == LEX_INT) res = (a.data.real_val < (double)b.data.int_val);
                    else if (a.type == LEX_STRING && b.type == LEX_STRING) res = (*a.data.str_val < *b.data.str_val);
                    else throw "Invalid operands for <";
                    check_stack_depth();
                    args.push(Value(res ? 1 : 0));
                    break;
                }
                
                case LEX_GT: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    bool res = false;
                    if (a.type == LEX_INT && b.type == LEX_INT) res = (a.data.int_val > b.data.int_val);
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) res = (a.data.real_val > b.data.real_val);
                    else if (a.type == LEX_INT && b.type == LEX_REAL) res = ((double)a.data.int_val > b.data.real_val);
                    else if (a.type == LEX_REAL && b.type == LEX_INT) res = (a.data.real_val > (double)b.data.int_val);
                    else if (a.type == LEX_STRING && b.type == LEX_STRING) res = (*a.data.str_val > *b.data.str_val);
                    else throw "Invalid operands for >";
                    check_stack_depth();
                    args.push(Value(res ? 1 : 0));
                    break;
                }
                
                case LEX_LEQ: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    bool res = false;
                    if (a.type == LEX_INT && b.type == LEX_INT) res = (a.data.int_val <= b.data.int_val);
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) res = (a.data.real_val <= b.data.real_val);
                    else if (a.type == LEX_INT && b.type == LEX_REAL) res = ((double)a.data.int_val <= b.data.real_val);
                    else if (a.type == LEX_REAL && b.type == LEX_INT) res = (a.data.real_val <= (double)b.data.int_val);
                    else if (a.type == LEX_STRING && b.type == LEX_STRING) res = (*a.data.str_val <= *b.data.str_val);
                    else throw "Invalid operands for <=";
                    check_stack_depth();
                    args.push(Value(res ? 1 : 0));
                    break;
                }
                
                case LEX_GEQ: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    bool res = false;
                    if (a.type == LEX_INT && b.type == LEX_INT) res = (a.data.int_val >= b.data.int_val);
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) res = (a.data.real_val >= b.data.real_val);
                    else if (a.type == LEX_INT && b.type == LEX_REAL) res = ((double)a.data.int_val >= b.data.real_val);
                    else if (a.type == LEX_REAL && b.type == LEX_INT) res = (a.data.real_val >= (double)b.data.int_val);
                    else if (a.type == LEX_STRING && b.type == LEX_STRING) res = (*a.data.str_val >= *b.data.str_val);
                    else throw "Invalid operands for >=";
                    check_stack_depth();
                    args.push(Value(res ? 1 : 0));
                    break;
                }
                
                // Переходы
                case POLIZ_GO: {
                    Value addr;
                    from_st(args, addr);
                    if (addr.type != LEX_INT) throw "Invalid jump address";
                    int i = addr.data.int_val;
                    if (i < 0 || i >= size) throw "Jump to invalid address";
                    index = i - 1;
                    break;
                }
                
                case POLIZ_FGO: {
                    Value addr, cond;
                    from_st(args, addr);
                    from_st(args, cond);
                    if (addr.type != LEX_INT) throw "Invalid jump address";
                    if (cond.type != LEX_INT) throw "Condition must be integer";
                    if (cond.data.int_val == 0) {
                        int i = addr.data.int_val;
                        if (i < 0 || i >= size) throw "Jump to invalid address";
                        index = i - 1;
                    }
                    break;
                }
                
                // Присваивание
                case LEX_ASSIGN: {
                    Value val, addr_val;
                    from_st(args, val);
                    from_st(args, addr_val);
                    if (addr_val.type != LEX_INT) throw "Invalid lvalue";
                    int idx = addr_val.data.int_val;
                    check_bounds(idx, TID.size(), "LEX_ASSIGN");
                    
                    if (TID[idx].get_type() == LEX_INT && val.type == LEX_INT) {
                        TID[idx].put_int(val.data.int_val);
                        args.push(Value(val.data.int_val));
                    }
                    else if (TID[idx].get_type() == LEX_INT && val.type == LEX_REAL) {
                        TID[idx].put_int((int)val.data.real_val);
                        args.push(Value((int)val.data.real_val));
                    }
                    else if (TID[idx].get_type() == LEX_REAL && val.type == LEX_INT) {
                        TID[idx].put_real((double)val.data.int_val);
                        args.push(Value((double)val.data.int_val));
                    }
                    else if (TID[idx].get_type() == LEX_REAL && val.type == LEX_REAL) {
                        TID[idx].put_real(val.data.real_val);
                        args.push(Value(val.data.real_val));
                    }
                    else if (TID[idx].get_type() == LEX_STRING && val.type == LEX_STRING) {
                        TID[idx].put_str(*val.data.str_val);
                        args.push(Value(*val.data.str_val));
                    }
                    else {throw "Assignment type mismatch";}
                    break;
                }
                
                // Ввод
                case LEX_READ: {
                    Value addr_val;
                    from_st(args, addr_val);
                    if (addr_val.type != LEX_INT) throw "Invalid read address";
                    int idx = addr_val.data.int_val;
                    check_bounds(idx, TID.size(), "LEX_READ");
                    
                    if (TID[idx].get_type() == LEX_INT) {
                        int val;
                        cout << "Enter integer for " << TID[idx].get_name() << ": ";
                        cin >> val;
                        TID[idx].put_int(val);
                    }
                    else if (TID[idx].get_type() == LEX_REAL) {
                        double val;
                        cout << "Enter real number for " << TID[idx].get_name() << ": ";
                        cin >> val;
                        TID[idx].put_real(val);
                    }
                    else if (TID[idx].get_type() == LEX_STRING) {
                        string val;
                        cout << "Enter string for " << TID[idx].get_name() << ": ";
                        cin >> ws;
                        getline(cin, val);
                        TID[idx].put_str(val);
                    }
                    break;
                }
                
                // Вывод
                case LEX_WRITE: {
                    Value v;
                    from_st(args, v);
                    if (v.type == LEX_INT)          cout << v.data.int_val;
                    else if (v.type == LEX_REAL)    cout << v.data.real_val;
                    else if (v.type == LEX_STRING)  cout << *v.data.str_val;
                    else throw "Unknown type in write";
                    cout << endl;
                    break;
                }
                
                default:    throw "Unknown POLIZ element";
            }
            index++;
        }
        cout << "\n Execution finished successfully " << endl;
    }
};


// Главный класс интерпретатора

class Interpreter {
    Parser pars;
    Executer exec;
public:
    Interpreter(const char* program) : pars(program) {}
    
    void interpretation() {
        pars.analyze();
        exec.execute(pars.poliz);
    }
};


// Точка входа

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <source_file>" << endl;
        return 1;
    }
    
    try {
        Interpreter I(argv[1]);
        I.interpretation();
        return 0;
    }
    catch (const char* msg) {cerr << "Error: " << msg << endl;}
    catch (const string& msg) {cerr << "Error: " << msg << endl;}
    catch (const exception& e) {cerr << "System error: " << e.what() << endl;}
    catch (...) {cerr << "Unknown error occurred" << endl;}
    return 1;
}