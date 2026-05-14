#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <stack>
#include <cctype>
#include <algorithm>
#include <climits>

using namespace std;

// Конфигурация
const int MAX_IDENT_LEN = 255;
const int MAX_COMMENT_LEN = 65536;
const int MAX_RECURSION_DEPTH = 1000;
const int MAX_STACK_DEPTH = 10000;
const int MAX_EXECUTION_STEPS = 10000000;

// Типы лексем
enum type_of_lex {
    LEX_NULL,
    
    // Ключевые слова
    LEX_AND, LEX_BEGIN, LEX_DO, LEX_ELSE, LEX_END,
    LEX_FOR, LEX_GOTO, LEX_IF, LEX_INT,
    LEX_NOT, LEX_OR, LEX_PROGRAM, LEX_READ, LEX_REAL,
    LEX_STEP, LEX_STRING, LEX_THEN, LEX_UNTIL, LEX_WHILE, LEX_WRITE,
    
    // Разделители и операторы
    LEX_SEMICOLON, LEX_COMMA, LEX_ASSIGN, LEX_COLON,
    LEX_LPAREN, LEX_RPAREN, LEX_LBRACE, LEX_RBRACE,
    LEX_EQ, LEX_LT, LEX_GT, LEX_LEQ, LEX_NEQ, LEX_GEQ,
    LEX_PLUS, LEX_MINUS, LEX_TIMES, LEX_SLASH,
    
    // Литералы
    LEX_NUM, LEX_REAL_NUM, LEX_STRING_LIT, LEX_ID,
    
    // Служебные для ПОЛИЗа
    POLIZ_LABEL, POLIZ_ADDRESS, POLIZ_GO, POLIZ_FGO, POLIZ_UMINUS, POLIZ_POP,
    
    LEX_FIN
};

// Лексема
class Lex {
    type_of_lex t_lex;
    int v_lex;
    int line;
    int column;
public:
    Lex(type_of_lex t = LEX_NULL, int v = 0, int l = 1, int c = 1) 
        : t_lex(t), v_lex(v), line(l), column(c) {}
    
    type_of_lex get_type() const { return t_lex; }
    int get_value() const { return v_lex; }
    int get_line() const { return line; }
    int get_column() const { return column; }
    
    void set_type(type_of_lex t) { t_lex = t; }
    void set_value(int v) { v_lex = v; }
    
    friend ostream& operator<<(ostream& s, const Lex& l) {
        s << '(' << l.t_lex << ',' << l.v_lex << ')';
        return s;
    }
};

// Значение для интерпретатора
struct Value {
    type_of_lex type;
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
        if (type == LEX_STRING) {
            data.str_val = new string(*other.data.str_val);
        } else {
            data = other.data;
        }
    }
    
    ~Value() {
        if (type == LEX_STRING) delete data.str_val;
    }
    
    Value& operator=(const Value& other) {
        if (this == &other) return *this;
        if (type == LEX_STRING) delete data.str_val;
        type = other.type;
        if (type == LEX_STRING) {
            data.str_val = new string(*other.data.str_val);
        } else {
            data = other.data;
        }
        return *this;
    }
};

// Идентификатор
class Ident {
    string name;
    type_of_lex type;
    bool declared;
    bool initialized;
    bool is_label;
    bool label_defined;
    int label_addr;
    vector<int> forward_refs;
    
    int int_val;
    double real_val;
    string* str_val;
    
public:
    Ident() : declared(false), initialized(false), is_label(false), 
              label_defined(false), label_addr(-1), type(LEX_INT), 
              int_val(0), real_val(0.0), str_val(nullptr) {}
    
    Ident(const string& n) : name(n), declared(false), initialized(false), 
              is_label(false), label_defined(false), label_addr(-1),
              type(LEX_INT), int_val(0), real_val(0.0), str_val(nullptr) {}
    
    ~Ident() { delete str_val; }
    
    const string& get_name() const { return name; }
    type_of_lex get_type() const { return type; }
    bool get_declared() const { return declared; }
    bool get_initialized() const { return initialized; }
    bool get_is_label() const { return is_label; }
    bool get_label_defined() const { return label_defined; }
    int get_label_addr() const { return label_addr; }
    const vector<int>& get_forward_refs() const { return forward_refs; }
    
    void put_type(type_of_lex t) { type = t; }
    void put_declared() { declared = true; }
    void put_initialized() { initialized = true; }
    void put_is_label(bool b) { is_label = b; }
    void put_label_defined(bool b) { label_defined = b; }
    void put_label_addr(int addr) { label_addr = addr; }
    void add_forward_ref(int pos) { forward_refs.push_back(pos); }
    void clear_forward_refs() { forward_refs.clear(); }
    
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

vector<Ident> TID;

int put(const string& buf) {
    auto it = find_if(TID.begin(), TID.end(), 
        [&buf](const Ident& id) { return id.get_name() == buf; });
    if (it != TID.end()) return it - TID.begin();
    TID.push_back(Ident(buf));
    return TID.size() - 1;
}

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

// ============================================================================
// Лексический анализатор
// ============================================================================

class Scanner {
    FILE* fp;
    char c;
    int current_line;
    int current_col;
    
    void update_position() {
        if (c == '\n') {
            current_line++;
            current_col = 1;
        } else {
            current_col++;
        }
    }
    
    void gc() {
        int ch = fgetc(fp);
        if (ch == EOF) {
            c = '@';
        } else {
            c = (char)ch;
            update_position();
        }
    }
    
    char peek() {
        int ch = fgetc(fp);
        if (ch == EOF) return '@';
        ungetc(ch, fp);
        return (char)ch;
    }
    
public:
    Scanner(const char* program) : current_line(1), current_col(1) {
        fp = fopen(program, "r");
        if (fp == NULL) throw "Can't open file";
    }
    
    ~Scanner() {
        if (fp) fclose(fp);
    }
    
    Lex get_lex();
};

Lex Scanner::get_lex() {
    enum state { H, IDENT, NUMB, REAL, STRING, COM, ALE, NEQ };
    state CS = H;
    string buf;
    int d = 0;
    
    do {
        gc();
        
        switch (CS) {
            case H:
                if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
                    // пропускаем
                }
                else if (isalpha(c)) {
                    buf.clear();
                    buf.push_back(c);
                    CS = IDENT;
                }
                else if (isdigit(c)) {
                    d = c - '0';
                    CS = NUMB;
                }
                else if (c == '/') {
                    if (peek() == '*') {
                        gc();
                        CS = COM;
                    } else {
                        return Lex(LEX_SLASH, 0, current_line, current_col);
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
                else if (c == '@') {
                    return Lex(LEX_FIN, 0, current_line, current_col);
                }
                else if (c == '!') {
                    buf.clear();
                    buf.push_back(c);
                    CS = NEQ;
                }
                else {
                    switch (c) {
                        case ';': return Lex(LEX_SEMICOLON, 0, current_line, current_col);
                        case ',': return Lex(LEX_COMMA, 0, current_line, current_col);
                        case '=': return Lex(LEX_ASSIGN, 0, current_line, current_col);
                        case '(': return Lex(LEX_LPAREN, 0, current_line, current_col);
                        case ')': return Lex(LEX_RPAREN, 0, current_line, current_col);
                        case '{': return Lex(LEX_LBRACE, 0, current_line, current_col);
                        case '}': return Lex(LEX_RBRACE, 0, current_line, current_col);
                        case '+': return Lex(LEX_PLUS, 0, current_line, current_col);
                        case '-': return Lex(LEX_MINUS, 0, current_line, current_col);
                        case '*': return Lex(LEX_TIMES, 0, current_line, current_col);
                        default: {
                            string err = "Unknown character: ";
                            err += c;
                            throw err.c_str();
                        }
                    }
                }
                break;
                
            case IDENT:
                if (isalpha(c) || isdigit(c)) {
                    buf.push_back(c);
                } else {
                    ungetc(c, fp);
                    current_col--;
                    
                    // Ключевые слова
                    if (buf == "and") return Lex(LEX_AND, 0, current_line, current_col - buf.length());
                    if (buf == "do") return Lex(LEX_DO, 0, current_line, current_col - buf.length());
                    if (buf == "else") return Lex(LEX_ELSE, 0, current_line, current_col - buf.length());
                    if (buf == "for") return Lex(LEX_FOR, 0, current_line, current_col - buf.length());
                    if (buf == "goto") return Lex(LEX_GOTO, 0, current_line, current_col - buf.length());
                    if (buf == "if") return Lex(LEX_IF, 0, current_line, current_col - buf.length());
                    if (buf == "int") return Lex(LEX_INT, 0, current_line, current_col - buf.length());
                    if (buf == "not") return Lex(LEX_NOT, 0, current_line, current_col - buf.length());
                    if (buf == "or") return Lex(LEX_OR, 0, current_line, current_col - buf.length());
                    if (buf == "program") return Lex(LEX_PROGRAM, 0, current_line, current_col - buf.length());
                    if (buf == "read") return Lex(LEX_READ, 0, current_line, current_col - buf.length());
                    if (buf == "real") return Lex(LEX_REAL, 0, current_line, current_col - buf.length());
                    if (buf == "step") return Lex(LEX_STEP, 0, current_line, current_col - buf.length());
                    if (buf == "string") return Lex(LEX_STRING, 0, current_line, current_col - buf.length());
                    if (buf == "until") return Lex(LEX_UNTIL, 0, current_line, current_col - buf.length());
                    if (buf == "while") return Lex(LEX_WHILE, 0, current_line, current_col - buf.length());
                    if (buf == "write") return Lex(LEX_WRITE, 0, current_line, current_col - buf.length());
                    
                    int idx = put(buf);
                    return Lex(LEX_ID, idx, current_line, current_col - buf.length());
                }
                break;
                
            case NUMB:
                if (isdigit(c)) {
                    d = d * 10 + (c - '0');
                } else if (c == '.') {
                    double real_val = d;
                    double frac = 0.0;
                    double divisor = 1.0;
                    gc();
                    while (isdigit(c)) {
                        frac = frac * 10 + (c - '0');
                        divisor *= 10;
                        gc();
                    }
                    real_val = real_val + frac / divisor;
                    ungetc(c, fp);
                    current_col--;
                    int idx = put_real(real_val);
                    return Lex(LEX_REAL_NUM, idx, current_line, current_col - 1);
                } else {
                    ungetc(c, fp);
                    current_col--;
                    return Lex(LEX_NUM, d, current_line, current_col - 1);
                }
                break;
                
            case STRING:
                if (c == '"') {
                    int idx = put_string(buf);
                    return Lex(LEX_STRING_LIT, idx, current_line, current_col - buf.length() - 1);
                } else if (c == '\\') {
                    gc();
                    switch (c) {
                        case 'n': buf += '\n'; break;
                        case 't': buf += '\t'; break;
                        case '"': buf += '"'; break;
                        case '\\': buf += '\\'; break;
                        default: buf += c; break;
                    }
                } else if (c == '@') {
                    throw "Unclosed string literal";
                } else {
                    buf += c;
                }
                break;
                
            case COM:
                if (c == '*' && peek() == '/') {
                    gc();
                    CS = H;
                } else if (c == '@') {
                    throw "Unclosed comment";
                }
                break;
                
            case ALE:
                if (c == '=') {
                    if (buf == "<") return Lex(LEX_LEQ, 0, current_line, current_col - 1);
                    if (buf == ">") return Lex(LEX_GEQ, 0, current_line, current_col - 1);
                } else {
                    ungetc(c, fp);
                    current_col--;
                    if (buf == "<") return Lex(LEX_LT, 0, current_line, current_col - 1);
                    if (buf == ">") return Lex(LEX_GT, 0, current_line, current_col - 1);
                }
                break;
                
            case NEQ:
                if (c == '=') {
                    return Lex(LEX_NEQ, 0, current_line, current_col - 1);
                } else {
                    ungetc(c, fp);
                    current_col--;
                    throw "Expected '!='";
                }
                break;
        }
    } while (true);
}

// ============================================================================
// Парсер
// ============================================================================

class Parser {
private:
    Lex curr_lex;
    type_of_lex c_type;
    int c_val;
    int c_line;
    int c_col;
    Scanner scan;
    stack<type_of_lex> st_lex;
    int recursion_depth;
    int temp_counter;
    vector<vector<int>> goto_places;
    
    void gl() {
        curr_lex = scan.get_lex();
        c_type = curr_lex.get_type();
        c_val = curr_lex.get_value();
        c_line = curr_lex.get_line();
        c_col = curr_lex.get_column();
        cout << "DEBUG gl(): type=" << c_type << ", val=" << c_val << ", line=" << c_line << ", col=" << c_col << endl;
    }
    
    void error(const string& msg) {
        throw msg + " at line " + to_string(c_line) + ", col " + to_string(c_col);
    }
    
    void enter() {
        if (++recursion_depth > MAX_RECURSION_DEPTH) 
            error("Parser recursion depth exceeded");
    }
    
    void leave() { recursion_depth--; }
    
    template<typename T>
    void from_st(T& st, typename T::value_type& x) {
        if (st.empty()) error("Semantic stack underflow");
        x = st.top();
        st.pop();
    }
    
    void check_id() {
        if (c_val < 0 || c_val >= (int)TID.size() || !TID[c_val].get_declared()) 
            error("Variable not declared");
        st_lex.push(TID[c_val].get_type());
    }
    
    void check_id_in_read() {
        if (!TID[c_val].get_declared()) error("Variable not declared in read");
    }
    
    void check_op() {
        type_of_lex t1, t2, op, result_type = LEX_NULL;
        
        if (st_lex.size() < 3) error("Not enough operands on semantic stack");
        
        from_st(st_lex, t2);
        from_st(st_lex, op);
        from_st(st_lex, t1);
        
        if (op == LEX_PLUS || op == LEX_MINUS || op == LEX_TIMES || op == LEX_SLASH) {
            if ((t1 == LEX_INT || t1 == LEX_REAL) && (t2 == LEX_INT || t2 == LEX_REAL)) {
                result_type = (t1 == LEX_REAL || t2 == LEX_REAL) ? LEX_REAL : LEX_INT;
            }
            else if (t1 == LEX_STRING && t2 == LEX_STRING && op == LEX_PLUS) {
                result_type = LEX_STRING;
            }
            else error("Arithmetic operation requires numeric operands");
        }
        else if (op == LEX_EQ || op == LEX_NEQ || op == LEX_LT || op == LEX_GT || 
                 op == LEX_LEQ || op == LEX_GEQ) {
            if ((t1 == LEX_INT || t1 == LEX_REAL) && (t2 == LEX_INT || t2 == LEX_REAL)) {
                result_type = LEX_INT;  // нет boolean, результат int (0 или 1)
            }
            else if (t1 == LEX_STRING && t2 == LEX_STRING) {
                result_type = LEX_INT;
            }
            else error("Comparison requires compatible types");
        }
        else if (op == LEX_AND || op == LEX_OR) {
            if (t1 == LEX_INT && t2 == LEX_INT) {
                result_type = LEX_INT;
            } else error("Logical operation requires integer operands");
        }
        else error("Unknown operator in check_op");
        
        st_lex.push(result_type);
        poliz.push_back(Lex(op));
    }
    
    void check_condition() {
        if (st_lex.empty() || st_lex.top() != LEX_INT) 
            error("Condition must be integer (0=false, non-0=true)");
        st_lex.pop();
    }
    
    void check_assign() {
        type_of_lex t1, t2;
        if (st_lex.size() < 2) error("Not enough operands for assignment");
        from_st(st_lex, t2);
        from_st(st_lex, t1);
        
        if (t1 == LEX_REAL && t2 == LEX_INT) { 
            st_lex.push(LEX_REAL); 
        }
        else if (t1 == t2) { 
            st_lex.push(t1); 
        }
        else { 
            error("Assignment type mismatch"); 
        }
    }
    
    void pop_expression_type() {
        if (st_lex.empty()) error("Expression type stack underflow");
        st_lex.pop();
    }
    
    void add_variable(const string& name, type_of_lex var_type) {
        int idx = put(name);
        if (TID[idx].get_declared()) error("Variable '" + name + "' declared twice");
        TID[idx].put_type(var_type);
        TID[idx].put_declared();
        if (var_type == LEX_INT) TID[idx].put_int(0);
        else if (var_type == LEX_REAL) TID[idx].put_real(0.0);
        else if (var_type == LEX_STRING) TID[idx].put_str("");
    }
    
    int create_temp_var(type_of_lex type) {
        string name = "__temp_" + to_string(temp_counter++);
        int idx = put(name);
        TID[idx].put_declared();
        TID[idx].put_type(type);
        return idx;
    }
    
    void ensure_goto_storage(int id) {
        if (id >= (int)goto_places.size()) {
            goto_places.resize(id + 1);
        }
    }
    
    void declare_label(int id) {
        ensure_goto_storage(id);
        
        if (TID[id].get_declared()) {
            error("Label '" + TID[id].get_name() + "' already declared");
        }
        
        int label_pos = poliz.size();
        TID[id].put_declared();
        TID[id].put_is_label(true);
        TID[id].put_label_defined(true);
        TID[id].put_label_addr(label_pos);
        
        for (int pos : TID[id].get_forward_refs()) {
            poliz[pos] = Lex(POLIZ_LABEL, label_pos);
        }
        TID[id].clear_forward_refs();
        
        poliz.push_back(Lex(POLIZ_LABEL, label_pos));
    }
    
    void add_goto_to_label(int id) {
        ensure_goto_storage(id);
        
        if (!TID[id].get_declared()) {
            TID[id].put_declared();
            TID[id].put_is_label(true);
            TID[id].put_label_defined(false);
        }
        
        int go_pos = poliz.size();
        poliz.push_back(Lex(POLIZ_GO));
        poliz.push_back(Lex());
        
        if (TID[id].get_label_defined()) {
            poliz[go_pos + 1] = Lex(POLIZ_LABEL, TID[id].get_label_addr());
        } else {
            TID[id].add_forward_ref(go_pos + 1);
        }
    }
    
    void check_gotos() {
        for (int id = 0; id < (int)goto_places.size(); ++id) {
            if (!goto_places[id].empty()) {
                error("Undefined label: " + TID[id].get_name());
            }
        }
    }
    
public:
    vector<Lex> poliz;
    
    Parser(const char* program) : scan(program), recursion_depth(0), temp_counter(0) {}
    
    void analyze() {
        gl();
        P();
        if (c_type != LEX_FIN) error("Expected end of program");
        check_gotos();
    }
    
    // Грамматика
    void P();
    void D();
    void S();
    void E_assign();
    void E();
    void E1();
    void T();
    void F();
};

void Parser::P() {
    if (c_type == LEX_PROGRAM) gl();
    else error("Expected 'program'");
    
    if (c_type != LEX_LBRACE) error("Expected '{' after program");
    gl();
    
    // Описания
    while (c_type == LEX_INT || c_type == LEX_REAL || c_type == LEX_STRING) {
        D();
        if (c_type != LEX_SEMICOLON) error("Expected ';' after declaration");
        gl();
    }
    
    if (c_type != LEX_RBRACE) error("Expected '}' after declarations");
    gl();
    
    // Операторы
    if (c_type != LEX_LBRACE) error("Expected '{' before statements");
    gl();
    
    while (c_type != LEX_RBRACE && c_type != LEX_FIN) {
        S();
    }
    
    if (c_type != LEX_RBRACE) error("Expected '}' after statements");
    gl();
}

void Parser::D() {
    type_of_lex var_type;
    if (c_type == LEX_INT) var_type = LEX_INT;
    else if (c_type == LEX_REAL) var_type = LEX_REAL;
    else if (c_type == LEX_STRING) var_type = LEX_STRING;
    else error("Expected type (int, real, string)");
    gl();
    
    if (c_type != LEX_ID) error("Expected identifier");
    string var_name = TID[c_val].get_name();
    gl();
    
    add_variable(var_name, var_type);
    
    while (c_type == LEX_COMMA) {
        gl();
        if (c_type != LEX_ID) error("Expected identifier");
        string next_var_name = TID[c_val].get_name();
        gl();
        add_variable(next_var_name, var_type);
    }
}

void Parser::E_assign() {
    E1();
    
    // Операции сравнения
    while (c_type == LEX_EQ || c_type == LEX_LT || c_type == LEX_GT || 
           c_type == LEX_LEQ || c_type == LEX_NEQ || c_type == LEX_GEQ) {
        type_of_lex op = c_type;
        st_lex.push(op);
        gl();
        E1();
        check_op();
    }
}

void Parser::E() {
    E_assign();
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
        if (st_lex.top() != LEX_INT) error("'not' requires integer operand");
        poliz.push_back(Lex(LEX_NOT));
    }
    else if (c_type == LEX_MINUS) {
        gl();
        F();
        if (st_lex.top() != LEX_INT && st_lex.top() != LEX_REAL) 
            error("Unary minus requires numeric operand");
        poliz.push_back(Lex(POLIZ_UMINUS));
    }
    else if (c_type == LEX_LPAREN) {
        gl();
        enter();
        E();
        leave();
        if (c_type != LEX_RPAREN) error("Expected ')'");
        gl();
    } else {
        error("Unexpected token in factor");
    }
}

void Parser::S() {
    // Метка: идентификатор : оператор
    if (c_type == LEX_ID) {
        int saved_val = c_val;
        Lex saved_lex = curr_lex;
        
        gl();
        if (c_type == LEX_COLON) {
            declare_label(saved_val);
            gl();
            S();
            return;
        } else {
            // Не метка - возвращаем
            c_type = saved_lex.get_type();
            c_val = saved_lex.get_value();
            curr_lex = saved_lex;
        }
    }
    
    // ПРИСВАИВАНИЕ (обрабатываем до оператора-выражения)
    if (c_type == LEX_ID) {
        int var_idx = c_val;
        
        // Проверяем, что переменная объявлена
        check_id();
        
        // Сохраняем идентификатор
        Lex id_lex = curr_lex;
        
        // Читаем идентификатор
        gl();
        
        // Если следующий токен - присваивание
        if (c_type == LEX_ASSIGN) {
            // Читаем '='
            gl();
            
            // Сохраняем адрес переменной в ПОЛИЗе
            poliz.push_back(Lex(POLIZ_ADDRESS, var_idx));
            
            // Разбираем правую часть выражения
            E_assign();  // теперь E_assign() не содержит '='
            
            // Проверяем совместимость типов
            check_assign();
            
            // Добавляем оператор присваивания в ПОЛИЗ
            poliz.push_back(Lex(LEX_ASSIGN));
            
            // Очищаем стек типов
            while (!st_lex.empty()) st_lex.pop();
            
            // Проверяем точку с запятой
            if (c_type != LEX_SEMICOLON) error("Expected ';' after assignment");
            gl();
            return;
        } else {
            // Не присваивание - возвращаем идентификатор обратно в поток
            c_type = LEX_ID;
            c_val = var_idx;
            curr_lex = id_lex;
            // Продолжаем к оператору-выражению
        }
    }
    
    // Условный оператор if (без else)
    if (c_type == LEX_IF) {
        gl();
        if (c_type != LEX_LPAREN) error("Expected '(' after if");
        gl();
        
        E();
        check_condition();
        
        if (c_type != LEX_RPAREN) error("Expected ')' after condition");
        gl();
        
        int false_label = poliz.size();
        poliz.push_back(Lex());
        poliz.push_back(Lex(POLIZ_FGO));
        
        S();
        
        poliz[false_label] = Lex(POLIZ_LABEL, poliz.size());
        return;
    }
    
    // Цикл while
    if (c_type == LEX_WHILE) {
        int start_label = poliz.size();
        gl();
        if (c_type != LEX_LPAREN) error("Expected '(' after while");
        gl();
        
        E();
        check_condition();
        
        if (c_type != LEX_RPAREN) error("Expected ')' after condition");
        gl();
        
        int exit_label = poliz.size();
        poliz.push_back(Lex());
        poliz.push_back(Lex(POLIZ_FGO));
        
        S();
        
        poliz.push_back(Lex(POLIZ_LABEL, start_label));
        poliz.push_back(Lex(POLIZ_GO));
        
        poliz[exit_label] = Lex(POLIZ_LABEL, poliz.size());
        return;
    }
    
    // Цикл for (II.3: for param = E1 step E2 until E3 do S)
    if (c_type == LEX_FOR) {
        gl();
        if (c_type != LEX_ID) error("Expected identifier after for");
        int var_idx = c_val;
        string var_name = TID[var_idx].get_name();
        gl();
        
        if (!TID[var_idx].get_declared()) error("Variable '" + var_name + "' not declared");
        if (TID[var_idx].get_type() != LEX_INT) error("For loop variable must be integer");
        if (c_type != LEX_ASSIGN) error("Expected '=' after for variable");
        gl();
        
        poliz.push_back(Lex(POLIZ_ADDRESS, var_idx));
        E();
        check_assign();
        poliz.push_back(Lex(LEX_ASSIGN));
        pop_expression_type();
        poliz.push_back(Lex(POLIZ_POP));
        
        if (c_type != LEX_STEP) error("Expected 'step'");
        gl();
        
        int step_temp = create_temp_var(LEX_INT);
        poliz.push_back(Lex(POLIZ_ADDRESS, step_temp));
        E();
        if (st_lex.top() != LEX_INT) error("Step must be integer");
        pop_expression_type();
        poliz.push_back(Lex(LEX_ASSIGN));
        poliz.push_back(Lex(POLIZ_POP));
        
        if (c_type != LEX_UNTIL) error("Expected 'until'");
        gl();
        
        int limit_temp = create_temp_var(LEX_INT);
        poliz.push_back(Lex(POLIZ_ADDRESS, limit_temp));
        E();
        if (st_lex.top() != LEX_INT) error("Until value must be integer");
        pop_expression_type();
        poliz.push_back(Lex(LEX_ASSIGN));
        poliz.push_back(Lex(POLIZ_POP));
        
        if (c_type != LEX_DO) error("Expected 'do'");
        gl();
        
        int start_label = poliz.size();
        
        poliz.push_back(Lex(POLIZ_ADDRESS, var_idx));
        poliz.push_back(Lex(POLIZ_ADDRESS, limit_temp));
        poliz.push_back(Lex(LEX_LEQ));
        
        int exit_label = poliz.size();
        poliz.push_back(Lex());
        poliz.push_back(Lex(POLIZ_FGO));
        
        S();
        
        poliz.push_back(Lex(POLIZ_ADDRESS, var_idx));
        poliz.push_back(Lex(POLIZ_ADDRESS, var_idx));
        poliz.push_back(Lex(POLIZ_ADDRESS, step_temp));
        poliz.push_back(Lex(LEX_PLUS));
        poliz.push_back(Lex(LEX_ASSIGN));
        poliz.push_back(Lex(POLIZ_POP));
        
        poliz.push_back(Lex(POLIZ_LABEL, start_label));
        poliz.push_back(Lex(POLIZ_GO));
        
        poliz[exit_label] = Lex(POLIZ_LABEL, poliz.size());
        return;
    }
    
    // goto
    if (c_type == LEX_GOTO) {
        gl();
        if (c_type != LEX_ID) error("Expected label after goto");
        int label_idx = c_val;
        gl();
        if (c_type != LEX_SEMICOLON) error("Expected ';' after goto");
        gl();
        
        add_goto_to_label(label_idx);
        return;
    }
    
    // read
    if (c_type == LEX_READ) {
        gl();
        if (c_type != LEX_LPAREN) error("Expected '(' after read");
        gl();
        if (c_type != LEX_ID) error("Expected identifier in read");
        check_id_in_read();
        poliz.push_back(Lex(POLIZ_ADDRESS, c_val));
        gl();
        if (c_type != LEX_RPAREN) error("Expected ')' after read");
        gl();
        if (c_type != LEX_SEMICOLON) error("Expected ';' after read");
        gl();
        poliz.push_back(Lex(LEX_READ));
        return;
    }
    
    // write
    if (c_type == LEX_WRITE) {
        gl();
        if (c_type != LEX_LPAREN) error("Expected '(' after write");
        gl();
        do {
            E();
            pop_expression_type();
            poliz.push_back(Lex(LEX_WRITE));
        } while (c_type == LEX_COMMA);
        if (c_type != LEX_RPAREN) error("Expected ')' after write list");
        gl();
        if (c_type != LEX_SEMICOLON) error("Expected ';' after write");
        gl();
        return;
    }
    
    // Составной оператор
    if (c_type == LEX_LBRACE) {
        gl();
        while (c_type != LEX_RBRACE && c_type != LEX_FIN) {
            S();
        }
        if (c_type == LEX_RBRACE) gl();
        else error("Expected '}'");
        return;
    }
    
    // Пустой оператор
    if (c_type == LEX_SEMICOLON) {
        gl();
        return;
    }
    
    // Оператор-выражение
    E();
    pop_expression_type();
    poliz.push_back(Lex(POLIZ_POP));
    if (c_type != LEX_SEMICOLON) error("Expected ';' after expression");
    gl();
}

// ============================================================================
// Интерпретатор
// ============================================================================

class Executer {
private:
    stack<Value> args;
    int steps;

    void check_stack_depth() {
        if (args.size() > MAX_STACK_DEPTH) throw "Stack overflow";
    }
    
    void check_bounds(int idx, int size, const string& context) {
        if (idx < 0 || idx >= size) throw "Index out of bounds in " + context;
    }
    
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
                throw "Integer overflow in multiplication";
        }
    }
    
    void check_overflow_sub(int a, int b) {
        if ((b > 0 && a < INT_MIN + b) || (b < 0 && a > INT_MAX + b))
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
                
                case LEX_ID: {
                    int idx = pc_el.get_value();
                    check_bounds(idx, TID.size(), "LEX_ID");
                    if (!TID[idx].get_initialized()) 
                        throw "Indefinite identifier: " + TID[idx].get_name();
                    check_stack_depth();
                    if (TID[idx].get_type() == LEX_INT)         
                        args.push(Value(TID[idx].get_int()));
                    else if (TID[idx].get_type() == LEX_REAL)   
                        args.push(Value(TID[idx].get_real()));
                    else if (TID[idx].get_type() == LEX_STRING) 
                        args.push(Value(*TID[idx].get_str()));
                    else throw "Unknown variable type";
                    break;
                }
                
                case POLIZ_UMINUS: {
                    Value v;
                    from_st(args, v);
                    if (v.type == LEX_INT) {
                        check_stack_depth();
                        args.push(Value(-v.data.int_val));
                    } else if (v.type == LEX_REAL) {
                        check_stack_depth();
                        args.push(Value(-v.data.real_val));
                    } else {
                        throw "Unary minus requires numeric operand";
                    }
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
                
                case LEX_OR: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    if (a.type != LEX_INT || b.type != LEX_INT) 
                        throw "'or' requires integer operands";
                    check_stack_depth();
                    args.push(Value((a.data.int_val || b.data.int_val) ? 1 : 0));
                    break;
                }
                
                case LEX_AND: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    if (a.type != LEX_INT || b.type != LEX_INT) 
                        throw "'and' requires integer operands";
                    check_stack_depth();
                    args.push(Value((a.data.int_val && b.data.int_val) ? 1 : 0));
                    break;
                }
                
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
                    else { throw "Invalid operands for *"; }
                    break;
                }
                
                case LEX_SLASH: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    
                    if ((b.type == LEX_INT && b.data.int_val == 0) ||
                        (b.type == LEX_REAL && b.data.real_val == 0.0)) 
                        throw "Division by zero";
                    
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
                
                case LEX_EQ: {
                    Value b, a;
                    from_st(args, b);
                    from_st(args, a);
                    bool res = false;
                    if (a.type == LEX_INT && b.type == LEX_INT) 
                        res = (a.data.int_val == b.data.int_val);
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) 
                        res = (a.data.real_val == b.data.real_val);
                    else if (a.type == LEX_INT && b.type == LEX_REAL) 
                        res = ((double)a.data.int_val == b.data.real_val);
                    else if (a.type == LEX_REAL && b.type == LEX_INT) 
                        res = (a.data.real_val == (double)b.data.int_val);
                    else if (a.type == LEX_STRING && b.type == LEX_STRING) 
                        res = (*a.data.str_val == *b.data.str_val);
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
                    if (a.type == LEX_INT && b.type == LEX_INT) 
                        res = (a.data.int_val != b.data.int_val);
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) 
                        res = (a.data.real_val != b.data.real_val);
                    else if (a.type == LEX_INT && b.type == LEX_REAL) 
                        res = ((double)a.data.int_val != b.data.real_val);
                    else if (a.type == LEX_REAL && b.type == LEX_INT) 
                        res = (a.data.real_val != (double)b.data.int_val);
                    else if (a.type == LEX_STRING && b.type == LEX_STRING) 
                        res = (*a.data.str_val != *b.data.str_val);
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
                    if (a.type == LEX_INT && b.type == LEX_INT) 
                        res = (a.data.int_val < b.data.int_val);
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) 
                        res = (a.data.real_val < b.data.real_val);
                    else if (a.type == LEX_INT && b.type == LEX_REAL) 
                        res = ((double)a.data.int_val < b.data.real_val);
                    else if (a.type == LEX_REAL && b.type == LEX_INT) 
                        res = (a.data.real_val < (double)b.data.int_val);
                    else if (a.type == LEX_STRING && b.type == LEX_STRING) 
                        res = (*a.data.str_val < *b.data.str_val);
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
                    if (a.type == LEX_INT && b.type == LEX_INT) 
                        res = (a.data.int_val > b.data.int_val);
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) 
                        res = (a.data.real_val > b.data.real_val);
                    else if (a.type == LEX_INT && b.type == LEX_REAL) 
                        res = ((double)a.data.int_val > b.data.real_val);
                    else if (a.type == LEX_REAL && b.type == LEX_INT) 
                        res = (a.data.real_val > (double)b.data.int_val);
                    else if (a.type == LEX_STRING && b.type == LEX_STRING) 
                        res = (*a.data.str_val > *b.data.str_val);
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
                    if (a.type == LEX_INT && b.type == LEX_INT) 
                        res = (a.data.int_val <= b.data.int_val);
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) 
                        res = (a.data.real_val <= b.data.real_val);
                    else if (a.type == LEX_INT && b.type == LEX_REAL) 
                        res = ((double)a.data.int_val <= b.data.real_val);
                    else if (a.type == LEX_REAL && b.type == LEX_INT) 
                        res = (a.data.real_val <= (double)b.data.int_val);
                    else if (a.type == LEX_STRING && b.type == LEX_STRING) 
                        res = (*a.data.str_val <= *b.data.str_val);
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
                    if (a.type == LEX_INT && b.type == LEX_INT) 
                        res = (a.data.int_val >= b.data.int_val);
                    else if (a.type == LEX_REAL && b.type == LEX_REAL) 
                        res = (a.data.real_val >= b.data.real_val);
                    else if (a.type == LEX_INT && b.type == LEX_REAL) 
                        res = ((double)a.data.int_val >= b.data.real_val);
                    else if (a.type == LEX_REAL && b.type == LEX_INT) 
                        res = (a.data.real_val >= (double)b.data.int_val);
                    else if (a.type == LEX_STRING && b.type == LEX_STRING) 
                        res = (*a.data.str_val >= *b.data.str_val);
                    else throw "Invalid operands for >=";
                    check_stack_depth();
                    args.push(Value(res ? 1 : 0));
                    break;
                }
                
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
                    else { throw "Assignment type mismatch"; }
                    break;
                }
                
                case LEX_READ: {
                    Value addr_val;
                    from_st(args, addr_val);
                    if (addr_val.type != LEX_INT) throw "Invalid read address";
                    int idx = addr_val.data.int_val;
                    check_bounds(idx, TID.size(), "LEX_READ");
                    
                    if (TID[idx].get_type() == LEX_INT) {
                        int val;
                        cout << "Enter integer: ";
                        cin >> val;
                        TID[idx].put_int(val);
                    }
                    else if (TID[idx].get_type() == LEX_REAL) {
                        double val;
                        cout << "Enter real number: ";
                        cin >> val;
                        TID[idx].put_real(val);
                    }
                    else if (TID[idx].get_type() == LEX_STRING) {
                        string val;
                        cout << "Enter string: ";
                        cin >> ws;
                        getline(cin, val);
                        TID[idx].put_str(val);
                    }
                    break;
                }
                
                case LEX_WRITE: {
                    Value v;
                    from_st(args, v);
                    if (v.type == LEX_INT) cout << v.data.int_val;
                    else if (v.type == LEX_REAL) cout << v.data.real_val;
                    else if (v.type == LEX_STRING) cout << *v.data.str_val;
                    else throw "Unknown type in write";
                    cout << endl;
                    break;
                }
                
                case POLIZ_POP: {
                    Value ignored;
                    from_st(args, ignored);
                    break;
                }
                
                default:
                    throw "Unknown POLIZ element";
            }
            index++;
        }
    }
};

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
    catch (const char* msg) {
        cerr << "Error: " << msg << endl;
    }
    catch (const string& msg) {
        cerr << "Error: " << msg << endl;
    }
    catch (const exception& e) {
        cerr << "System error: " << e.what() << endl;
    }
    catch (...) {
        cerr << "Unknown error occurred" << endl;
    }
    return 1;
}