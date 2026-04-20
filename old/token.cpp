#include "token.h"
#include <map>
#include <set>
#include <sstream>
#include <iomanip>
#include <vector>

using namespace std;

// Преобразование TokenType в строку (для отладки)

string tokenTypeToString(TokenType type) {
    switch (type) {
        // Ключевые слова
        case TokenType::LEX_PROGRAM:   return "LEX_PROGRAM";
        case TokenType::LEX_INT:       return "LEX_INT";
        case TokenType::LEX_STRING:    return "LEX_STRING";
        case TokenType::LEX_REAL:      return "LEX_REAL";
        case TokenType::LEX_IF:        return "LEX_IF";
        case TokenType::LEX_ELSE:        return "LEX_ELSE";
        case TokenType::LEX_WHILE:     return "LEX_WHILE";
        case TokenType::LEX_READ:      return "LEX_READ";
        case TokenType::LEX_WRITE:     return "LEX_WRITE";
        case TokenType::LEX_FOR:       return "LEX_FOR";
        case TokenType::LEX_STEP:      return "LEX_STEP";
        case TokenType::LEX_UNTIL:     return "LEX_UNTIL";
        case TokenType::LEX_DO:        return "LEX_DO";
        case TokenType::LEX_GOTO:      return "LEX_GOTO";
        case TokenType::LEX_AND:       return "LEX_AND";
        case TokenType::LEX_OR:        return "LEX_OR";
        case TokenType::LEX_NOT:       return "LEX_NOT";
        
        // Операторы
        case TokenType::OP_ASSIGN:    return "OP_ASSIGN";
        case TokenType::OP_PLUS:      return "OP_PLUS";
        case TokenType::OP_MINUS:     return "OP_MINUS";
        case TokenType::OP_MUL:       return "OP_MUL";
        case TokenType::OP_DIV:       return "OP_DIV";
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
        case TokenType::END_OF_FILE:  return "END_OF_FILE";
        case TokenType::ERROR:        return "ERROR";
        
        default:                      return "UNKNOWN";
    }
}

// таблицы служебных слов, операторов и разделителей

// Карта: строка -> тип токена для служебных слов
static const map<string, TokenType> keywordMap = {
    {"program", TokenType::LEX_PROGRAM},
    {"int",     TokenType::LEX_INT},
    {"string",  TokenType::LEX_STRING},
    {"real",    TokenType::LEX_REAL},
    {"if",      TokenType::LEX_IF},
    {"else",      TokenType::LEX_ELSE},
    {"while",   TokenType::LEX_WHILE},
    {"read",    TokenType::LEX_READ},
    {"write",   TokenType::LEX_WRITE},
    {"for",     TokenType::LEX_FOR},
    {"step",    TokenType::LEX_STEP},
    {"until",   TokenType::LEX_UNTIL},
    {"do",      TokenType::LEX_DO},
    {"goto",    TokenType::LEX_GOTO},
    {"and",     TokenType::LEX_AND},
    {"or",      TokenType::LEX_OR},
    {"not",     TokenType::LEX_NOT}
};

// Карта: строка -> тип токена для операторов и разделителей
static const map<string, TokenType> operatorMap = {
    {"=",  TokenType::OP_ASSIGN},
    {"+",  TokenType::OP_PLUS},
    {"-",  TokenType::OP_MINUS},
    {"*",  TokenType::OP_MUL},
    {"/",  TokenType::OP_DIV},
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

// Множество всех служебных слов
static const set<string> keywordSet = []() {
    set<string> s;
    for (const auto& p : keywordMap) s.insert(p.first);
    return s;
}();

// методы Token

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

void Token::init(TokenType t, const string& lex, int ln, int col) {
    type = t;
    lexeme = lex;
    line = ln;
    column = col;
    
    // значения по умолчанию
    intValue = 0;
    realValue = 0.0;
    stringValue.clear();
}

// Конструкторы

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

// Методы проверки

bool Token::isOneOf(const vector<TokenType>& types) const {
    for (TokenType t : types) {
        if (type == t) return true;
    }
    return false;
}

bool Token::isKeyword() const {
    return type >= TokenType::LEX_PROGRAM && type <= TokenType::LEX_NOT;
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

// Получение значения в виде строки

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

// Вывод и отладка

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

ostream& operator<<(ostream& os, const Token& token) {
    os << token.toString();
    return os;
}