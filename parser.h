// parser.h
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "symbol_table.h"
#include "poliz.h"
#include <vector>
#include <stack>
#include <functional>

//============================================================================
// Класс синтаксического анализатора (рекурсивный спуск)
// Реализует нисходящий разбор с генерацией ПОЛИЗа
//============================================================================

class Parser {
private:
    //-----------------------------------------------------------------------
    // Компоненты
    //-----------------------------------------------------------------------
    
    Lexer lexer;                    // лексический анализатор
    SymbolTable symTable;           // таблица символов
    Poliz poliz;                    // генерируемый ПОЛИЗ
    
    // Текущий токен
    Token currentToken;
    
    // Флаги и счётчики
    bool hadError;                  // была ли ошибка
    std::vector<std::string> errors;  // список ошибок
    
    //-----------------------------------------------------------------------
    // Стеки для семантических действий (генерации ПОЛИЗа)
    //-----------------------------------------------------------------------
    
    // Стек для хранения адресов переменных при разборе выражений
    std::stack<int> addrStack;
    
    // Стек для хранения типов при разборе выражений
    std::stack<DataType> typeStack;
    
    // Стек для отложенных переходов (if, while, for)
    struct JumpInfo {
        int position;       // позиция в ПОЛИЗе
        std::string label;  // имя метки (если есть)
    };
    std::stack<JumpInfo> jumpStack;
    
    //-----------------------------------------------------------------------
    // Вспомогательные методы
    //-----------------------------------------------------------------------
    
    // Получение следующего токена
    void advance() {
        currentToken = lexer.nextToken();
    }
    
    // Проверка текущего токена
    bool match(TokenType type) {
        if (currentToken.type == type) {
            advance();
            return true;
        }
        return false;
    }
    
    // Ожидание конкретного токена
    void expect(TokenType type, const std::string& errorMsg);
    
    // Сообщение об ошибке
    void error(const std::string& msg);
    void errorAt(const Token& token, const std::string& msg);
    
    //-----------------------------------------------------------------------
    // Семантические действия (проверки и генерация)
    //-----------------------------------------------------------------------
    
    // Проверка, объявлена ли переменная
    void checkDeclared(const std::string& name);
    
    // Проверка типа переменной при использовании
    DataType checkVariableType(const std::string& name);
    
    // Проверка совместимости типов для бинарной операции
    void checkBinaryOp(DataType left, DataType right, TokenType op);
    
    // Проверка, что выражение целочисленное (для условий)
    void checkIntegerCondition();
    
    // Генерация кода для бинарной операции
    void generateBinaryOp(TokenType op);
    
    //-----------------------------------------------------------------------
    // Синтаксические правила (рекурсивный спуск)
    // Каждый метод возвращает DataType результата (для выражений)
    //-----------------------------------------------------------------------
    
    // Программа в целом
    void program();
    
    // Раздел описаний
    void descriptions();
    void description();
    void variableList(DataType type);
    void variable(DataType type);
    
    // Раздел операторов
    void operators();
    void statement();
    
    // Конкретные операторы
    void ifStatement();         // I.1: if (выражение) оператор
    void whileStatement();
    void forStatement();        // II.3: for I = E1 step E2 until E3 do S
    void gotoStatement();       // III.1: goto идентификатор;
    void readStatement();
    void writeStatement();
    void compoundStatement();
    void assignmentStatement(); // идентификатор = выражение;
    
    // Выражения (каждый возвращает тип результата)
    DataType expression();           // верхний уровень
    DataType assignmentExpression(); // с учётом присваивания
    DataType logicalOrExpression();  // or (полное вычисление)
    DataType logicalAndExpression(); // and (полное вычисление)
    DataType relationalExpression();
    DataType additiveExpression();
    DataType multiplicativeExpression();
    DataType unaryExpression();
    DataType primaryExpression();
    
public:
    //-----------------------------------------------------------------------
    // Конструкторы
    //-----------------------------------------------------------------------
    
    explicit Parser(const std::string& source);
    ~Parser() = default;
    
    //-----------------------------------------------------------------------
    // Основной метод
    //-----------------------------------------------------------------------
    
    bool parse();
    
    //-----------------------------------------------------------------------
    // Получение результатов
    //-----------------------------------------------------------------------
    
    const std::vector<std::string>& getErrors() const { return errors; }
    SymbolTable& getSymbolTable() { return symTable; }
    Poliz& getPoliz() { return poliz; }
    bool hasErrors() const { return hadError; }
    
    //-----------------------------------------------------------------------
    // Сброс для повторного использования
    //-----------------------------------------------------------------------
    
    void reset();
};

#endif // PARSER_H