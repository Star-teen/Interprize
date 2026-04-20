#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <iostream>
#include <vector>

enum class TokenType {
    LEX_PROGRAM,     // program
    LEX_INT,         // int
    LEX_STRING,      // string
    LEX_REAL,        // real
    LEX_IF,          // if
    LEX_ELSE,        // else
    LEX_WHILE,       // while
    LEX_READ,        // read
    LEX_WRITE,       // write
    LEX_FOR,         // for
    LEX_STEP,        // step
    LEX_UNTIL,       // until
    LEX_DO,          // do
    LEX_GOTO,        // goto
    LEX_AND,         // and
    LEX_OR,          // or
    LEX_NOT,         // not
    
    // Операторы
    OP_ASSIGN,      // =
    OP_PLUS,        // +
    OP_MINUS,       // -
    OP_MUL,         // *
    OP_DIV,         // /
    OP_LT,          // <
    OP_GT,          // >
    OP_LE,          // <=
    OP_GE,          // >=
    OP_EQ,          // ==
    OP_NE,          // !=
    
    // Разделители
    SEP_SEMICOLON,  // ;
    SEP_COMMA,      // ,
    SEP_LPAREN,     // (
    SEP_RPAREN,     // )
    SEP_LBRACE,     // {
    SEP_RBRACE,     // }
    SEP_COLON,      // :
    
    // Иное
    IDENTIFIER,     // идентификатор (имя переменной, метки)
    INT_CONST,      // целочисленная константа
    REAL_CONST,     // вещественная константа
    STRING_CONST,   // строковая константа
    END_OF_FILE,    // конец файла
    ERROR           // ошибочный токен
};

// Преобразование TokenType в строку (для отладки)
std::string tokenTypeToString(TokenType type);

// Класс токена — одна лексема
class Token {
public:
    TokenType   type;       // тип токена
    std::string lexeme;     // исходный текст токена (для отладки)
    int         line;       // номер строки в исходном файле
    int         column;     // номер колонки в строке
    
    int         intValue;       // для INT_CONST
    double      realValue;      // для REAL_CONST
    std::string stringValue;    // для STRING_CONST и IDENTIFIER
        
    // Конструктор по умолчанию — создаёт пустой токен с типом ERROR
    Token();
    
    // Конструктор для простых токенов
    // Пример: Token(TokenType::SEP_SEMICOLON, ";", 5, 10)
    Token(TokenType t, const std::string& lex, int ln, int col);
    
    // Конструктор для целочисленной константы
    Token(int val, int ln, int col);
    
    // Конструктор для вещественной константы
    Token(double val, int ln, int col);
    
    // Конструктор для строковой константы
    Token(const std::string& val, int ln, int col);
    
    // Конструктор для идентификатора
    static Token identifier(const std::string& name, int ln, int col);
        
    // Проверка типа токена
    bool is(TokenType t) const { return type == t; }
    bool isOneOf(const std::vector<TokenType>& types) const;
    
    // Проверка, является ли токен служебным словом
    bool isKeyword() const;
    
    // Проверка, является ли токен оператором
    bool isOperator() const;
    
    // Проверка, является ли токен разделителем
    bool isSeparator() const;
    
    // Проверка, является ли токен константой
    bool isConstant() const;
    
    // Получение значения в виде строки
    std::string getValueAsString() const;
    
    // Вывод и отладка
    std::string toString() const;
        
    // Проверка, является ли строка служебным словом
    static bool isKeyword(const std::string& s);
    
    // Получение типа токена для служебного слова (или IDENTIFIER)
    static TokenType getKeywordType(const std::string& s);
    
    // Получение типа токена для оператора/разделителя
    static TokenType getOperatorType(const std::string& s);
    
private:
    // Инициализация (метод для конструкторов)
    void init(TokenType t, const std::string& lex, int ln, int col);
};

std::ostream& operator<<(std::ostream& os, const Token& token);

#endif // TOKEN_H