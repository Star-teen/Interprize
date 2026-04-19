// interpreter.cpp
#include "interpreter.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

using namespace std;

//============================================================================
// Реализация StackValue
//============================================================================

StackValue::StackValue() 
    : type(DataType::UNDEFINED), intValue(0), realValue(0.0) {}

StackValue::StackValue(int v) 
    : type(DataType::INT), intValue(v), realValue(static_cast<double>(v)) {}

StackValue::StackValue(double v) 
    : type(DataType::REAL), intValue(static_cast<int>(v)), realValue(v) {}

StackValue::StackValue(const string& v) 
    : type(DataType::STRING), intValue(0), realValue(0.0), stringValue(v) {}

StackValue::StackValue(const StackValue& other)
    : type(other.type), intValue(other.intValue), 
      realValue(other.realValue), stringValue(other.stringValue) {}

StackValue& StackValue::operator=(const StackValue& other) {
    if (this != &other) {
        type = other.type;
        intValue = other.intValue;
        realValue = other.realValue;
        stringValue = other.stringValue;
    }
    return *this;
}

bool StackValue::toBool() const {
    switch (type) {
        case DataType::INT:
            return intValue != 0;
        case DataType::REAL:
            return realValue != 0.0;
        case DataType::STRING:
            return !stringValue.empty();
        default:
            return false;
    }
}

string StackValue::toString() const {
    switch (type) {
        case DataType::INT:
            return to_string(intValue);
        case DataType::REAL: {
            ostringstream oss;
            oss << fixed << setprecision(6) << realValue;
            string s = oss.str();
            // Убираем лишние нули в конце
            while (s.length() > 1 && s.back() == '0') s.pop_back();
            if (s.back() == '.') s.pop_back();
            return s;
        }
        case DataType::STRING:
            return stringValue;
        default:
            return "?";
    }
}

void StackValue::print() const {
    cout << toString();
}

//============================================================================
// Реализация Interpreter
//============================================================================

Interpreter::Interpreter(const Poliz& p, SymbolTable& st)
    : poliz(p), symTable(st), ip(0), running(false), 
      debugMode(false), maxSteps(10000), currentStep(0) {
    setStandardIO();
}

//============================================================================
// Работа со стеком
//============================================================================

void Interpreter::push(const StackValue& v) {
    stack.push_back(v);
}

StackValue Interpreter::pop() {
    if (stack.empty()) {
        throw runtime_error("Ошибка: стек пуст (попытка извлечь значение)");
    }
    StackValue v = stack.back();
    stack.pop_back();
    return v;
}

StackValue Interpreter::peek() const {
    if (stack.empty()) {
        throw runtime_error("Ошибка: стек пуст (попытка посмотреть вершину)");
    }
    return stack.back();
}

void Interpreter::clearStack() {
    stack.clear();
}

//============================================================================
// Проверка совместимости типов (Таблица №2)
//============================================================================

bool Interpreter::isBinaryOpCompatible(PolizCmd op, DataType left, DataType right) {
    // Операции отношения: int/int, real/real, string/string, а также смешанные int/real
    bool isRelOp = (op == PolizCmd::LESS || op == PolizCmd::GREATER ||
                    op == PolizCmd::LE || op == PolizCmd::GE ||
                    op == PolizCmd::EQ || op == PolizCmd::NE);
    
    // Логические операции: только int
    bool isLogicalOp = (op == PolizCmd::AND || op == PolizCmd::OR);
    
    if (isLogicalOp) {
        return (left == DataType::INT && right == DataType::INT);
    }
    
    if (isRelOp) {
        // int/int, real/real, int/real, real/int, string/string
        if (left == DataType::STRING && right == DataType::STRING) return true;
        if ((left == DataType::INT || left == DataType::REAL) &&
            (right == DataType::INT || right == DataType::REAL)) return true;
        return false;
    }
    
    // Арифметические операции
    if (op == PolizCmd::ADD || op == PolizCmd::SUB || 
        op == PolizCmd::MUL || op == PolizCmd::DIV) {
        // Конкатенация строк
        if (op == PolizCmd::ADD && left == DataType::STRING && right == DataType::STRING) {
            return true;
        }
        // Числовые операции
        if ((left == DataType::INT || left == DataType::REAL) &&
            (right == DataType::INT || right == DataType::REAL)) {
            return true;
        }
        return false;
    }
    
    // Операция MOD (только int)
    if (op == PolizCmd::MOD) {
        return (left == DataType::INT && right == DataType::INT);
    }
    
    return false;
}

DataType Interpreter::getBinaryOpResultType(PolizCmd op, DataType left, DataType right) {
    bool isRelOp = (op == PolizCmd::LESS || op == PolizCmd::GREATER ||
                    op == PolizCmd::LE || op == PolizCmd::GE ||
                    op == PolizCmd::EQ || op == PolizCmd::NE);
    
    bool isLogicalOp = (op == PolizCmd::AND || op == PolizCmd::OR);
    
    if (isRelOp || isLogicalOp) {
        return DataType::INT;  // результат отношений и логики — int (0/1)
    }
    
    // Конкатенация строк
    if (op == PolizCmd::ADD && left == DataType::STRING && right == DataType::STRING) {
        return DataType::STRING;
    }
    
    // Арифметика: если хоть один операнд real, результат real
    if ((left == DataType::REAL || right == DataType::REAL) &&
        (op == PolizCmd::ADD || op == PolizCmd::SUB || 
         op == PolizCmd::MUL || op == PolizCmd::DIV)) {
        return DataType::REAL;
    }
    
    return DataType::INT;
}

//============================================================================
// Бинарные и унарные операции
//============================================================================

StackValue Interpreter::applyBinaryOp(PolizCmd op, const StackValue& left, const StackValue& right) {
    if (!isBinaryOpCompatible(op, left.type, right.type)) {
        ostringstream oss;
        oss << "Несовместимые типы в операции: " 
            << dataTypeToString(left.type) << " и " 
            << dataTypeToString(right.type);
        throw runtime_error(oss.str());
    }
    
    // Операции отношения
    switch (op) {
        case PolizCmd::LESS:
            if (left.type == DataType::STRING && right.type == DataType::STRING)
                return StackValue((left.stringValue < right.stringValue) ? 1 : 0);
            if (left.type == DataType::INT && right.type == DataType::INT)
                return StackValue((left.intValue < right.intValue) ? 1 : 0);
            if (left.type == DataType::REAL && right.type == DataType::REAL)
                return StackValue((left.realValue < right.realValue) ? 1 : 0);
            if ((left.type == DataType::INT || left.type == DataType::REAL) &&
                (right.type == DataType::INT || right.type == DataType::REAL)) {
                double l = (left.type == DataType::INT) ? left.intValue : left.realValue;
                double r = (right.type == DataType::INT) ? right.intValue : right.realValue;
                return StackValue((l < r) ? 1 : 0);
            }
            break;
            
        case PolizCmd::GREATER:
            if (left.type == DataType::STRING && right.type == DataType::STRING)
                return StackValue((left.stringValue > right.stringValue) ? 1 : 0);
            if (left.type == DataType::INT && right.type == DataType::INT)
                return StackValue((left.intValue > right.intValue) ? 1 : 0);
            if (left.type == DataType::REAL && right.type == DataType::REAL)
                return StackValue((left.realValue > right.realValue) ? 1 : 0);
            if ((left.type == DataType::INT || left.type == DataType::REAL) &&
                (right.type == DataType::INT || right.type == DataType::REAL)) {
                double l = (left.type == DataType::INT) ? left.intValue : left.realValue;
                double r = (right.type == DataType::INT) ? right.intValue : right.realValue;
                return StackValue((l > r) ? 1 : 0);
            }
            break;
            
        case PolizCmd::LE:
            if (left.type == DataType::STRING && right.type == DataType::STRING)
                return StackValue((left.stringValue <= right.stringValue) ? 1 : 0);
            if (left.type == DataType::INT && right.type == DataType::INT)
                return StackValue((left.intValue <= right.intValue) ? 1 : 0);
            if (left.type == DataType::REAL && right.type == DataType::REAL)
                return StackValue((left.realValue <= right.realValue) ? 1 : 0);
            if ((left.type == DataType::INT || left.type == DataType::REAL) &&
                (right.type == DataType::INT || right.type == DataType::REAL)) {
                double l = (left.type == DataType::INT) ? left.intValue : left.realValue;
                double r = (right.type == DataType::INT) ? right.intValue : right.realValue;
                return StackValue((l <= r) ? 1 : 0);
            }
            break;
            
        case PolizCmd::GE:
            if (left.type == DataType::STRING && right.type == DataType::STRING)
                return StackValue((left.stringValue >= right.stringValue) ? 1 : 0);
            if (left.type == DataType::INT && right.type == DataType::INT)
                return StackValue((left.intValue >= right.intValue) ? 1 : 0);
            if (left.type == DataType::REAL && right.type == DataType::REAL)
                return StackValue((left.realValue >= right.realValue) ? 1 : 0);
            if ((left.type == DataType::INT || left.type == DataType::REAL) &&
                (right.type == DataType::INT || right.type == DataType::REAL)) {
                double l = (left.type == DataType::INT) ? left.intValue : left.realValue;
                double r = (right.type == DataType::INT) ? right.intValue : right.realValue;
                return StackValue((l >= r) ? 1 : 0);
            }
            break;
            
        case PolizCmd::EQ:
            if (left.type == DataType::STRING && right.type == DataType::STRING)
                return StackValue((left.stringValue == right.stringValue) ? 1 : 0);
            if (left.type == DataType::INT && right.type == DataType::INT)
                return StackValue((left.intValue == right.intValue) ? 1 : 0);
            if (left.type == DataType::REAL && right.type == DataType::REAL)
                return StackValue((left.realValue == right.realValue) ? 1 : 0);
            if ((left.type == DataType::INT || left.type == DataType::REAL) &&
                (right.type == DataType::INT || right.type == DataType::REAL)) {
                double l = (left.type == DataType::INT) ? left.intValue : left.realValue;
                double r = (right.type == DataType::INT) ? right.intValue : right.realValue;
                return StackValue((l == r) ? 1 : 0);
            }
            break;
            
        case PolizCmd::NE:
            if (left.type == DataType::STRING && right.type == DataType::STRING)
                return StackValue((left.stringValue != right.stringValue) ? 1 : 0);
            if (left.type == DataType::INT && right.type == DataType::INT)
                return StackValue((left.intValue != right.intValue) ? 1 : 0);
            if (left.type == DataType::REAL && right.type == DataType::REAL)
                return StackValue((left.realValue != right.realValue) ? 1 : 0);
            if ((left.type == DataType::INT || left.type == DataType::REAL) &&
                (right.type == DataType::INT || right.type == DataType::REAL)) {
                double l = (left.type == DataType::INT) ? left.intValue : left.realValue;
                double r = (right.type == DataType::INT) ? right.intValue : right.realValue;
                return StackValue((l != r) ? 1 : 0);
            }
            break;
            
        // Логические операции
        case PolizCmd::AND:
            return StackValue((left.toBool() && right.toBool()) ? 1 : 0);
        case PolizCmd::OR:
            return StackValue((left.toBool() || right.toBool()) ? 1 : 0);
            
        // Арифметические операции
        case PolizCmd::ADD:
            if (left.type == DataType::STRING && right.type == DataType::STRING)
                return StackValue(left.stringValue + right.stringValue);
            if (left.type == DataType::INT && right.type == DataType::INT)
                return StackValue(left.intValue + right.intValue);
            if ((left.type == DataType::INT || left.type == DataType::REAL) &&
                (right.type == DataType::INT || right.type == DataType::REAL)) {
                double l = (left.type == DataType::INT) ? left.intValue : left.realValue;
                double r = (right.type == DataType::INT) ? right.intValue : right.realValue;
                return StackValue(l + r);
            }
            break;
            
        case PolizCmd::SUB:
            if (left.type == DataType::INT && right.type == DataType::INT)
                return StackValue(left.intValue - right.intValue);
            if ((left.type == DataType::INT || left.type == DataType::REAL) &&
                (right.type == DataType::INT || right.type == DataType::REAL)) {
                double l = (left.type == DataType::INT) ? left.intValue : left.realValue;
                double r = (right.type == DataType::INT) ? right.intValue : right.realValue;
                return StackValue(l - r);
            }
            break;
            
        case PolizCmd::MUL:
            if (left.type == DataType::INT && right.type == DataType::INT)
                return StackValue(left.intValue * right.intValue);
            if ((left.type == DataType::INT || left.type == DataType::REAL) &&
                (right.type == DataType::INT || right.type == DataType::REAL)) {
                double l = (left.type == DataType::INT) ? left.intValue : left.realValue;
                double r = (right.type == DataType::INT) ? right.intValue : right.realValue;
                return StackValue(l * r);
            }
            break;
            
        case PolizCmd::DIV:
            if (left.type == DataType::INT && right.type == DataType::INT) {
                if (right.intValue == 0) throw runtime_error("Деление на ноль");
                return StackValue(left.intValue / right.intValue);
            }
            if ((left.type == DataType::INT || left.type == DataType::REAL) &&
                (right.type == DataType::INT || right.type == DataType::REAL)) {
                double l = (left.type == DataType::INT) ? left.intValue : left.realValue;
                double r = (right.type == DataType::INT) ? right.intValue : right.realValue;
                if (r == 0.0) throw runtime_error("Деление на ноль");
                return StackValue(l / r);
            }
            break;
            
        case PolizCmd::MOD:
            if (left.type == DataType::INT && right.type == DataType::INT) {
                if (right.intValue == 0) throw runtime_error("Остаток от деления на ноль");
                return StackValue(left.intValue % right.intValue);
            }
            break;
            
        default:
            break;
    }
    
    throw runtime_error("Неподдерживаемая бинарная операция");
}

StackValue Interpreter::applyUnaryOp(PolizCmd op, const StackValue& operand) {
    switch (op) {
        case PolizCmd::NEG:
            if (operand.type == DataType::INT)
                return StackValue(-operand.intValue);
            if (operand.type == DataType::REAL)
                return StackValue(-operand.realValue);
            throw runtime_error("Унарный минус не применим к данному типу");
            
        case PolizCmd::NOT:
            return StackValue(operand.toBool() ? 0 : 1);
            
        default:
            throw runtime_error("Неподдерживаемая унарная операция");
    }
}

//============================================================================
// Выполнение одной инструкции
//============================================================================

void Interpreter::executeInstruction(const PolizInstruction& instr) {
    switch (instr.cmd) {
        // ----- Константы -----
        case PolizCmd::PUSH_INT:
            push(StackValue(instr.intValue));
            break;
            
        case PolizCmd::PUSH_REAL:
            push(StackValue(instr.realValue));
            break;
            
        case PolizCmd::PUSH_STRING:
            push(StackValue(instr.strValue));
            break;
            
        // ----- Переменные -----
        case PolizCmd::PUSH_VAR:
            if (instr.address >= 0) {
                DataType type = symTable.getTypeByAddress(instr.address);
                if (type == DataType::INT) {
                    push(StackValue(symTable.getIntValueByAddress(instr.address)));
                } else if (type == DataType::REAL) {
                    push(StackValue(symTable.getRealValueByAddress(instr.address)));
                } else if (type == DataType::STRING) {
                    push(StackValue(symTable.getStringValueByAddress(instr.address)));
                } else {
                    throw runtime_error("Переменная не инициализирована");
                }
            } else {
                // Поиск по имени
                DataType type = symTable.getVariableType(instr.strValue);
                if (type == DataType::INT) {
                    push(StackValue(symTable.getIntValue(instr.strValue)));
                } else if (type == DataType::REAL) {
                    push(StackValue(symTable.getRealValue(instr.strValue)));
                } else if (type == DataType::STRING) {
                    push(StackValue(symTable.getStringValue(instr.strValue)));
                } else {
                    throw runtime_error("Переменная '" + instr.strValue + "' не найдена");
                }
            }
            break;
            
        case PolizCmd::POP_VAR:
            {
                StackValue val = pop();
                if (instr.address >= 0) {
                    DataType varType = symTable.getTypeByAddress(instr.address);
                    if (varType == DataType::INT && val.type == DataType::INT) {
                        symTable.setIntValueByAddress(instr.address, val.intValue);
                    } else if (varType == DataType::REAL && 
                               (val.type == DataType::INT || val.type == DataType::REAL)) {
                        double v = (val.type == DataType::INT) ? val.intValue : val.realValue;
                        symTable.setRealValueByAddress(instr.address, v);
                    } else if (varType == DataType::STRING && val.type == DataType::STRING) {
                        symTable.setStringValueByAddress(instr.address, val.stringValue);
                    } else {
                        throw runtime_error("Несоответствие типов при присваивании");
                    }
                } else {
                    // Поиск по имени
                    DataType varType = symTable.getVariableType(instr.strValue);
                    if (varType == DataType::INT && val.type == DataType::INT) {
                        symTable.setVariableValue(instr.strValue, val.intValue);
                    } else if (varType == DataType::REAL && 
                               (val.type == DataType::INT || val.type == DataType::REAL)) {
                        double v = (val.type == DataType::INT) ? val.intValue : val.realValue;
                        symTable.setVariableValue(instr.strValue, v);
                    } else if (varType == DataType::STRING && val.type == DataType::STRING) {
                        symTable.setVariableValue(instr.strValue, val.stringValue);
                    } else {
                        throw runtime_error("Несоответствие типов при присваивании");
                    }
                }
            }
            break;
            
        // ----- Бинарные операции -----
        case PolizCmd::ADD:
        case PolizCmd::SUB:
        case PolizCmd::MUL:
        case PolizCmd::DIV:
        case PolizCmd::MOD:
        case PolizCmd::LESS:
        case PolizCmd::GREATER:
        case PolizCmd::LE:
        case PolizCmd::GE:
        case PolizCmd::EQ:
        case PolizCmd::NE:
        case PolizCmd::AND:
        case PolizCmd::OR:
            {
                StackValue right = pop();
                StackValue left = pop();
                push(applyBinaryOp(instr.cmd, left, right));
            }
            break;
            
        // ----- Унарные операции -----
        case PolizCmd::NEG:
        case PolizCmd::NOT:
            {
                StackValue operand = pop();
                push(applyUnaryOp(instr.cmd, operand));
            }
            break;
            
        // ----- Управление -----
        case PolizCmd::LABEL:
            // Ничего не делаем
            break;
            
        case PolizCmd::GOTO:
            ip = instr.intValue;
            return;  // не увеличиваем ip
            
        case PolizCmd::GOTO_IF_FALSE:
            {
                StackValue cond = pop();
                if (!cond.toBool()) {
                    ip = instr.intValue;
                    return;
                }
            }
            break;
            
        case PolizCmd::GOTO_IF_TRUE:
            {
                StackValue cond = pop();
                if (cond.toBool()) {
                    ip = instr.intValue;
                    return;
                }
            }
            break;
            
        // ----- Ввод/вывод -----
        case PolizCmd::READ:
            {
                string input;
                if (inputCallback) {
                    input = inputCallback();
                } else {
                    getline(cin, input);
                }
                
                if (instr.address >= 0) {
                    DataType type = symTable.getTypeByAddress(instr.address);
                    if (type == DataType::INT) {
                        symTable.setIntValueByAddress(instr.address, stoi(input));
                    } else if (type == DataType::REAL) {
                        symTable.setRealValueByAddress(instr.address, stod(input));
                    } else {
                        symTable.setStringValueByAddress(instr.address, input);
                    }
                } else {
                    DataType type = symTable.getVariableType(instr.strValue);
                    if (type == DataType::INT) {
                        symTable.setVariableValue(instr.strValue, stoi(input));
                    } else if (type == DataType::REAL) {
                        symTable.setVariableValue(instr.strValue, stod(input));
                    } else {
                        symTable.setVariableValue(instr.strValue, input);
                    }
                }
            }
            break;
            
        case PolizCmd::WRITE:
            {
                StackValue val = pop();
                string output = val.toString();
                if (outputCallback) {
                    outputCallback(output);
                } else {
                    cout << output;
                }
            }
            break;
            
        case PolizCmd::HALT:
            running = false;
            break;
            
        case PolizCmd::NOP:
            // Ничего не делаем
            break;
            
        default:
            throw runtime_error("Неизвестная команда ПОЛИЗа");
    }
    
    ip++;
}

//============================================================================
// Отладка
//============================================================================

void Interpreter::printState() const {
    cout << "\n=== Состояние ВМ ===" << endl;
    cout << "IP: " << ip << " / " << poliz.size() << endl;
    cout << "Стек (" << stack.size() << "): ";
    for (size_t i = 0; i < stack.size(); i++) {
        if (i > 0) cout << ", ";
        cout << stack[i].toString();
    }
    cout << endl;
    
    if (ip < poliz.size()) {
        cout << "Следующая инструкция: " << poliz[ip].toString() << endl;
    }
    cout << "===================" << endl;
}

//============================================================================
// Управление выполнением
//============================================================================

bool Interpreter::step() {
    if (!running) return false;
    if (ip >= poliz.size()) {
        running = false;
        return false;
    }
    
    if (debugMode) {
        printState();
    }
    
    try {
        executeInstruction(poliz[ip]);
        currentStep++;
        
        if (maxSteps > 0 && currentStep > maxSteps) {
            throw runtime_error("Превышено максимальное количество шагов (возможно зацикливание)");
        }
        
        return running;
    } catch (const exception& e) {
        cerr << "\nОшибка выполнения на шаге " << currentStep << ": " << e.what() << endl;
        if (debugMode) {
            cerr << "IP = " << ip << ", инструкция: " << poliz[ip].toString() << endl;
        }
        running = false;
        throw;
    }
}

void Interpreter::run(istream& input) {
    // Настройка ввода/вывода
    if (!inputCallback) {
        // Временная установка для стандартного ввода
        inputCallback = [&input]() {
            string line;
            getline(input, line);
            return line;
        };
    }
    
    reset();
    running = true;
    currentStep = 0;
    
    while (running && ip < poliz.size()) {
        if (!step()) break;
    }
    
    if (debugMode && !running) {
        cout << "\n=== Выполнение завершено ===" << endl;
    }
}

void Interpreter::reset() {
    clearStack();
    ip = 0;
    running = false;
    currentStep = 0;
}

void Interpreter::setStandardIO() {
    outputCallback = [](const string& s) { cout << s; };
    inputCallback = []() { 
        string line;
        getline(cin, line);
        return line;
    };
}