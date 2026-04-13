// value.h
#ifndef VALUE_H
#define VALUE_H

#include "symbol_table.h"
#include <string>
#include <sstream>
#include <iomanip>

//============================================================================
// Класс Value — вариантное представление значения
// Используется в интерпретаторе для работы со стеком
//============================================================================

class Value {
private:
    DataType type;
    int intValue;
    double realValue;
    std::string stringValue;
    
public:
    //========================================================================
    // Конструкторы
    //========================================================================
    
    Value();
    explicit Value(int v);
    explicit Value(double v);
    explicit Value(const std::string& v);
    Value(const Value& other);
    
    //========================================================================
    // Операторы присваивания
    //========================================================================
    
    Value& operator=(int v);
    Value& operator=(double v);
    Value& operator=(const std::string& v);
    Value& operator=(const Value& other);
    
    //========================================================================
    // Доступ к типу и значению
    //========================================================================
    
    DataType getType() const { return type; }
    
    int asInt() const;
    double asReal() const;
    std::string asString() const;
    
    //========================================================================
    // Преобразования
    //========================================================================
    
    bool toBool() const;           // преобразование в логическое значение
    std::string toString() const;  // преобразование в строку (для вывода)
    
    //========================================================================
    // Арифметические операции (для удобства)
    //========================================================================
    
    Value operator+(const Value& other) const;
    Value operator-(const Value& other) const;
    Value operator*(const Value& other) const;
    Value operator/(const Value& other) const;
    Value operator%(const Value& other) const;
    Value operator-() const;        // унарный минус
    
    //========================================================================
    // Логические операции
    //========================================================================
    
    Value operator&&(const Value& other) const;  // логическое И
    Value operator||(const Value& other) const;  // логическое ИЛИ
    Value operator!() const;                      // логическое НЕ
    
    //========================================================================
    // Операции отношения
    //========================================================================
    
    Value operator<(const Value& other) const;
    Value operator>(const Value& other) const;
    Value operator<=(const Value& other) const;
    Value operator>=(const Value& other) const;
    Value operator==(const Value& other) const;
    Value operator!=(const Value& other) const;
    
    //========================================================================
    // Вывод
    //========================================================================
    
    void print() const;
    std::string debugString() const;
};

#endif // VALUE_H