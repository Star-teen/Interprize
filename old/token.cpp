// token.cpp
#include "token.h"
#include <map>
#include <set>
#include <sstream>
#include <iomanip>

using namespace std;

//============================================================================
// Преобразование TokenType в строку (для отладки)
//============================================================================

string tokenTypeToString(TokenType type) {
    switch (type) {
        // Ключевые слова
        case TokenType::KW_PROGRAM:   return "KW_PROGRAM";
        case TokenType::KW_INT:       return "KW_INT";
        case TokenType::KW_STRING:    return "KW_STRING";
        case TokenType::KW_REAL:      return "KW_REAL";
        case TokenType::KW_IF:        return "KW_IF";
        case TokenType::KW_WHILE:     return "KW_WHILE";
        case TokenType::KW_READ:      return "KW_READ";
        case TokenType::KW_WRITE:     return "KW_WRITE";
        case TokenType::KW_FOR:       return "KW_FOR";
        case TokenType::KW_STEP:      return "KW_STEP";
        case TokenType::KW_UNTIL:     return "KW_UNTIL";
        case TokenType::KW_DO:        return "KW_DO";
        case TokenType::KW_GOTO:      return "KW_GOTO";
        case TokenType::KW_AND:       return "KW_AND";
        case TokenType::KW_OR:        return "KW_OR";
        case TokenType::KW_NOT:       return "KW_NOT";
        
        // Операторы
        case TokenType::OP_ASSIGN:    return "OP_ASSIGN";
        case TokenType::OP_PLUS:      return "OP_PLUS";
        case TokenType::OP_MINUS:     return "OP_MINUS";
        case TokenType::OP_MUL:       return "OP_MUL";
        case TokenType::OP_DIV:       return "OP_DIV";
        case TokenType::OP_MOD:       return "OP_MOD";
        case TokenType::OP_LT:        return "OP_LT";
        case TokenType::OP_GT:        return "OP_GT";
        case TokenType::OP_LE:        return "OP_LE";
        case TokenType::OP_GE:        return "OP_GE";
        case TokenType::OP_EQ:        return "OP_EQ";
        case TokenType::OP_NE:        return "OP_NE";
        
        // Разделители
        case TokenType::SEP_SEMICOLON: return "SEP_SEMICOLON";
        case TokenType::SEP_COMMA:     return "SEP_COMMA";
        case TokenType::SEP_LPAREN:    return "SEP_LPAREN";
        case TokenType::SEP_RPAREN:    return "SEP_RPAREN";
        case TokenType::SEP_LBRACE:    return "SEP_LBRACE";
        case TokenType::SEP_RBRACE:    return "SEP_RBRACE";
        case TokenType::SEP_COLON:     return "SEP_COLON";
        
        // Прочие
        case TokenType::IDENTIFIER:   return "IDENTIFIER";
        case TokenType::INT_CONST:    return "INT_CONST";
        case TokenType::REAL_CONST:   return "REAL_CONST";
        case TokenType::STRING_CONST: return "STRING_CONST";
        
        // Служебные
        case TokenType::END_OF_FILE:  return "END_OF_FILE";
        case TokenType::ERROR:        return "ERROR";
        
        default:                      return "UNKNOWN";
    }
}

//============================================================================
// Статические таблицы служебных слов, операторов и разделителей
//============================================================================

// Карта: строка -> тип токена для служебных слов
static const map<string, TokenType> keywordMap = {
    {"program", TokenType::KW_PROGRAM},
    {"int",     TokenType::KW_INT},
    {"string",  TokenType::KW_STRING},
    {"real",    TokenType::KW_REAL},
    {"if",      TokenType::KW_IF},
    {"while",   TokenType::KW_WHILE},
    {"read",    TokenType::KW_READ},
    {"write",   TokenType::KW_WRITE},
    {"for",     TokenType::KW_FOR},
    {"step",    TokenType::KW_STEP},
    {"until",   TokenType::KW_UNTIL},
    {"do",      TokenType::KW_DO},
    {"goto",    TokenType::KW_GOTO},
    {"and",     TokenType::KW_AND},
    {"or",      TokenType::KW_OR},
    {"not",     TokenType::KW_NOT}
};

// Карта: строка -> тип токена для операторов и разделителей
static const map<string, TokenType> operatorMap = {
    {"=",  TokenType::OP_ASSIGN},
    {"+",  TokenType::OP_PLUS},
    {"-",  TokenType::OP_MINUS},
    {"*",  TokenType::OP_MUL},
    {"/",  TokenType::OP_DIV},
    {"%",  TokenType::OP_MOD},
    {"<",  TokenType::OP_LT},
    {">",  TokenType::OP_GT},
    {"<=", TokenType::OP_LE},
    {">=", TokenType::OP_GE},
    {"==", TokenType::OP_EQ},
    {"!=", TokenType::OP_NE},
    {";",  TokenType::SEP_SEMICOLON},
    {",",  TokenType::SEP_COMMA},
    {"(",  TokenType::SEP_LPAREN},
    {")",  TokenType::SEP_RPAREN},
    {"{",  TokenType::SEP_LBRACE},
    {"}",  TokenType::SEP_RBRACE},
    {":",  TokenType::SEP_COLON}
};

// Множество всех служебных слов (для быстрой проверки)
static const set<string> keywordSet = []() {
    set<string> s;
    for (const auto& p : keywordMap) s.insert(p.first);
    return s;
}();

//============================================================================
// Статические методы Token
//============================================================================

bool Token::isKeyword(const string& s) {
    return keywordSet.find(s) != keywordSet.end();
}

TokenType Token::getKeywordType(const string& s) {
    auto it = keywordMap.find(s);
    if (it != keywordMap.end()) {
        return it->second;
    }
    return TokenType::IDENTIFIER;
}

TokenType Token::getOperatorType(const string& s) {
    auto it = operatorMap.find(s);
    if (it != operatorMap.end()) {
        return it->second;
    }
    return TokenType::ERROR;
}

//============================================================================
// Приватный вспомогательный метод
//============================================================================

void Token::init(TokenType t, const string& lex, int ln, int col) {
    type = t;
    lexeme = lex;
    line = ln;
    column = col;
    
    // Инициализация значений по умолчанию
    intValue = 0;
    realValue = 0.0;
    stringValue.clear();
}

//============================================================================
// Конструкторы
//============================================================================

Token::Token() {
    init(TokenType::ERROR, "", 0, 0);
}

Token::Token(TokenType t, const string& lex, int ln, int col) {
    init(t, lex, ln, col);
    
    // Если токен — идентификатор, сохраняем его имя в stringValue
    if (t == TokenType::IDENTIFIER) {
        stringValue = lex;
    }
}

Token::Token(int val, int ln, int col) {
    init(TokenType::INT_CONST, to_string(val), ln, col);
    intValue = val;
}

Token::Token(double val, int ln, int col) {
    // Преобразуем double в строку с нужным форматом
    ostringstream oss;
    oss << fixed << setprecision(6) << val;
    string str = oss.str();
    // Убираем лишние нули в конце
    while (str.length() > 1 && str.back() == '0') str.pop_back();
    if (str.back() == '.') str.pop_back();
    
    init(TokenType::REAL_CONST, str, ln, col);
    realValue = val;
}

Token::Token(const string& val, int ln, int col) {
    init(TokenType::STRING_CONST, "\"" + val + "\"", ln, col);
    stringValue = val;
}

Token Token::identifier(const string& name, int ln, int col) {
    Token t;
    t.init(TokenType::IDENTIFIER, name, ln, col);
    t.stringValue = name;
    return t;
}

//============================================================================
// Методы проверки
//============================================================================

bool Token::isOneOf(const vector<TokenType>& types) const {
    for (TokenType t : types) {
        if (type == t) return true;
    }
    return false;
}

bool Token::isKeyword() const {
    return type >= TokenType::KW_PROGRAM && type <= TokenType::KW_NOT;
}

bool Token::isOperator() const {
    return type >= TokenType::OP_ASSIGN && type <= TokenType::OP_NE;
}

bool Token::isSeparator() const {
    return type >= TokenType::SEP_SEMICOLON && type <= TokenType::SEP_COLON;
}

bool Token::isConstant() const {
    return type == TokenType::INT_CONST ||
           type == TokenType::REAL_CONST ||
           type == TokenType::STRING_CONST;
}

//============================================================================
// Получение значения в виде строки
//============================================================================

string Token::getValueAsString() const {
    switch (type) {
        case TokenType::INT_CONST:
            return to_string(intValue);
        case TokenType::REAL_CONST: {
            ostringstream oss;
            oss << fixed << setprecision(6) << realValue;
            string s = oss.str();
            while (s.length() > 1 && s.back() == '0') s.pop_back();
            if (s.back() == '.') s.pop_back();
            return s;
        }
        case TokenType::STRING_CONST:
        case TokenType::IDENTIFIER:
            return stringValue;
        default:
            return lexeme;
    }
}

//============================================================================
// Вывод и отладка
//============================================================================

string Token::toString() const {
    ostringstream oss;
    oss << "Token{"
        << "type=" << tokenTypeToString(type)
        << ", lexeme='" << lexeme << "'"
        << ", line=" << line
        << ", col=" << column;
    
    // Добавляем значение в зависимости от типа
    if (type == TokenType::INT_CONST) {
        oss << ", value=" << intValue;
    } else if (type == TokenType::REAL_CONST) {
        oss << ", value=" << realValue;
    } else if (type == TokenType::STRING_CONST || type == TokenType::IDENTIFIER) {
        oss << ", value='" << stringValue << "'";
    }
    
    oss << "}";
    return oss.str();
}

void Token::print() const {
    cout << toString() << endl;
}

//============================================================================
// Оператор вывода
//============================================================================

ostream& operator<<(ostream& os, const Token& token) {
    os << token.toString();
    return os;
}