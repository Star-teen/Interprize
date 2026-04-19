// value.cpp
#include "value.h"
#include <cmath>
#include <stdexcept>

using namespace std;

//============================================================================
// Конструкторы
//============================================================================

Value::Value() 
    : type(DataType::UNDEFINED), intValue(0), realValue(0.0) {}

Value::Value(int v) 
    : type(DataType::INT), intValue(v), realValue(static_cast<double>(v)) {}

Value::Value(double v) 
    : type(DataType::REAL), intValue(static_cast<int>(v)), realValue(v) {}

Value::Value(const string& v) 
    : type(DataType::STRING), intValue(0), realValue(0.0), stringValue(v) {}

Value::Value(const Value& other)
    : type(other.type), intValue(other.intValue), 
      realValue(other.realValue), stringValue(other.stringValue) {}

//============================================================================
// Операторы присваивания
//============================================================================

Value& Value::operator=(int v) {
    type = DataType::INT;
    intValue = v;
    realValue = static_cast<double>(v);
    stringValue.clear();
    return *this;
}

Value& Value::operator=(double v) {
    type = DataType::REAL;
    intValue = static_cast<int>(v);
    realValue = v;
    stringValue.clear();
    return *this;
}

Value& Value::operator=(const string& v) {
    type = DataType::STRING;
    intValue = 0;
    realValue = 0.0;
    stringValue = v;
    return *this;
}

Value& Value::operator=(const Value& other) {
    if (this != &other) {
        type = other.type;
        intValue = other.intValue;
        realValue = other.realValue;
        stringValue = other.stringValue;
    }
    return *this;
}

//============================================================================
// Доступ к значению
//============================================================================

int Value::asInt() const {
    switch (type) {
        case DataType::INT:   return intValue;
        case DataType::REAL:  return static_cast<int>(realValue);
        case DataType::STRING: return 0;
        default:              return 0;
    }
}

double Value::asReal() const {
    switch (type) {
        case DataType::INT:   return static_cast<double>(intValue);
        case DataType::REAL:  return realValue;
        case DataType::STRING: return 0.0;
        default:              return 0.0;
    }
}

string Value::asString() const {
    switch (type) {
        case DataType::INT:    return to_string(intValue);
        case DataType::REAL: {
            ostringstream oss;
            oss << fixed << setprecision(6) << realValue;
            string s = oss.str();
            while (s.length() > 1 && s.back() == '0') s.pop_back();
            if (s.back() == '.') s.pop_back();
            return s;
        }
        case DataType::STRING: return stringValue;
        default:               return "";
    }
}

//============================================================================
// Преобразования
//============================================================================

bool Value::toBool() const {
    switch (type) {
        case DataType::INT:    return intValue != 0;
        case DataType::REAL:   return realValue != 0.0;
        case DataType::STRING: return !stringValue.empty();
        default:               return false;
    }
}

string Value::toString() const {
    return asString();
}

string Value::debugString() const {
    switch (type) {
        case DataType::INT:    return "int(" + to_string(intValue) + ")";
        case DataType::REAL:   return "real(" + to_string(realValue) + ")";
        case DataType::STRING: return "string(\"" + stringValue + "\")";
        default:               return "undefined";
    }
}

//============================================================================
// Арифметические операции (с проверкой типов по Таблице №2)
//============================================================================

Value Value::operator+(const Value& other) const {
    // Конкатенация строк
    if (type == DataType::STRING && other.type == DataType::STRING) {
        return Value(stringValue + other.stringValue);
    }
    
    // Числовое сложение
    if ((type == DataType::INT || type == DataType::REAL) &&
        (other.type == DataType::INT || other.type == DataType::REAL)) {
        double result = asReal() + other.asReal();
        // Если оба операнда были int и результат целый, возвращаем int
        if (type == DataType::INT && other.type == DataType::INT) {
            return Value(static_cast<int>(result));
        }
        return Value(result);
    }
    
    throw runtime_error("Недопустимые типы для операции +");
}

Value Value::operator-(const Value& other) const {
    if ((type == DataType::INT || type == DataType::REAL) &&
        (other.type == DataType::INT || other.type == DataType::REAL)) {
        double result = asReal() - other.asReal();
        if (type == DataType::INT && other.type == DataType::INT) {
            return Value(static_cast<int>(result));
        }
        return Value(result);
    }
    throw runtime_error("Недопустимые типы для операции -");
}

Value Value::operator*(const Value& other) const {
    if ((type == DataType::INT || type == DataType::REAL) &&
        (other.type == DataType::INT || other.type == DataType::REAL)) {
        double result = asReal() * other.asReal();
        if (type == DataType::INT && other.type == DataType::INT) {
            return Value(static_cast<int>(result));
        }
        return Value(result);
    }
    throw runtime_error("Недопустимые типы для операции *");
}

Value Value::operator/(const Value& other) const {
    if ((type == DataType::INT || type == DataType::REAL) &&
        (other.type == DataType::INT || other.type == DataType::REAL)) {
        double divisor = other.asReal();
        if (divisor == 0.0) {
            throw runtime_error("Деление на ноль");
        }
        double result = asReal() / divisor;
        if (type == DataType::INT && other.type == DataType::INT) {
            return Value(static_cast<int>(result));
        }
        return Value(result);
    }
    throw runtime_error("Недопустимые типы для операции /");
}

Value Value::operator%(const Value& other) const {
    if (type == DataType::INT && other.type == DataType::INT) {
        if (other.intValue == 0) {
            throw runtime_error("Остаток от деления на ноль");
        }
        return Value(intValue % other.intValue);
    }
    throw runtime_error("Операция % применима только к целым числам");
}

Value Value::operator-() const {
    if (type == DataType::INT) {
        return Value(-intValue);
    }
    if (type == DataType::REAL) {
        return Value(-realValue);
    }
    throw runtime_error("Унарный минус не применим к данному типу");
}

//============================================================================
// Логические операции
//============================================================================

Value Value::operator&&(const Value& other) const {
    bool result = toBool() && other.toBool();
    return Value(result ? 1 : 0);
}

Value Value::operator||(const Value& other) const {
    bool result = toBool() || other.toBool();
    return Value(result ? 1 : 0);
}

Value Value::operator!() const {
    return Value(toBool() ? 0 : 1);
}

//============================================================================
// Операции отношения
//============================================================================

Value Value::operator<(const Value& other) const {
    bool result = false;
    
    // Строки
    if (type == DataType::STRING && other.type == DataType::STRING) {
        result = stringValue < other.stringValue;
    }
    // Числа (включая смешанные int/real)
    else if ((type == DataType::INT || type == DataType::REAL) &&
             (other.type == DataType::INT || other.type == DataType::REAL)) {
        result = asReal() < other.asReal();
    }
    else {
        throw runtime_error("Недопустимые типы для операции <");
    }
    
    return Value(result ? 1 : 0);
}

Value Value::operator>(const Value& other) const {
    bool result = false;
    
    if (type == DataType::STRING && other.type == DataType::STRING) {
        result = stringValue > other.stringValue;
    }
    else if ((type == DataType::INT || type == DataType::REAL) &&
             (other.type == DataType::INT || other.type == DataType::REAL)) {
        result = asReal() > other.asReal();
    }
    else {
        throw runtime_error("Недопустимые типы для операции >");
    }
    
    return Value(result ? 1 : 0);
}

Value Value::operator<=(const Value& other) const {
    bool result = false;
    
    if (type == DataType::STRING && other.type == DataType::STRING) {
        result = stringValue <= other.stringValue;
    }
    else if ((type == DataType::INT || type == DataType::REAL) &&
             (other.type == DataType::INT || other.type == DataType::REAL)) {
        result = asReal() <= other.asReal();
    }
    else {
        throw runtime_error("Недопустимые типы для операции <=");
    }
    
    return Value(result ? 1 : 0);
}

Value Value::operator>=(const Value& other) const {
    bool result = false;
    
    if (type == DataType::STRING && other.type == DataType::STRING) {
        result = stringValue >= other.stringValue;
    }
    else if ((type == DataType::INT || type == DataType::REAL) &&
             (other.type == DataType::INT || other.type == DataType::REAL)) {
        result = asReal() >= other.asReal();
    }
    else {
        throw runtime_error("Недопустимые типы для операции >=");
    }
    
    return Value(result ? 1 : 0);
}

Value Value::operator==(const Value& other) const {
    bool result = false;
    
    if (type != other.type) {
        // Разные типы — всегда false, кроме случаев int/real сравнения
        if ((type == DataType::INT && other.type == DataType::REAL) ||
            (type == DataType::REAL && other.type == DataType::INT)) {
            result = asReal() == other.asReal();
        } else {
            result = false;
        }
    } else {
        switch (type) {
            case DataType::INT:    result = intValue == other.intValue; break;
            case DataType::REAL:   result = realValue == other.realValue; break;
            case DataType::STRING: result = stringValue == other.stringValue; break;
            default:               result = false; break;
        }
    }
    
    return Value(result ? 1 : 0);
}

Value Value::operator!=(const Value& other) const {
    Value eq = (*this == other);
    return Value(eq.toBool() ? 0 : 1);
}

//============================================================================
// Вывод
//============================================================================

void Value::print() const {
    cout << toString();
}

string Value::debugString() const {
    ostringstream oss;
    oss << "Value{";
    switch (type) {
        case DataType::INT:    oss << "int:" << intValue; break;
        case DataType::REAL:   oss << "real:" << realValue; break;
        case DataType::STRING: oss << "string:\"" << stringValue << "\""; break;
        default:               oss << "undefined"; break;
    }
    oss << "}";
    return oss.str();
}