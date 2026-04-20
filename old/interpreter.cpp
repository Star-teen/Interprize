// interpreter.cpp
#include "interpreter.h"
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <algorithm>

using namespace std;

//============================================================================
// Конструктор
//============================================================================

Interpreter::Interpreter(const Poliz& p, SymbolTable& st)
    : poliz(p)
    , symTable(st)
    , ip(0)
    , running(false)
    , debugMode(false)
    , maxSteps(10000)
    , currentStep(0)
    , singleStepMode(false)
{
    setStandardIO();
}

//============================================================================
// Работа со стеком
//============================================================================

void Interpreter::push(const Value& v) {
    stack.push_back(v);
    
    if (debugMode) {
        cout << "  push: " << v.debugString() << endl;
    }
}

Value Interpreter::pop() {
    if (stack.empty()) {
        throw runtime_error("Ошибка: стек пуст (попытка извлечь значение)");
    }
    Value v = stack.back();
    stack.pop_back();
    
    if (debugMode) {
        cout << "  pop: " << v.debugString() << endl;
    }
    
    return v;
}

Value Interpreter::peek() const {
    if (stack.empty()) {
        throw runtime_error("Ошибка: стек пуст (попытка посмотреть вершину)");
    }
    return stack.back();
}

void Interpreter::clearStack() {
    stack.clear();
    if (debugMode) {
        cout << "  стек очищен" << endl;
    }
}

void Interpreter::printStack() const {
    cout << "Стек (" << stack.size() << "): ";
    for (size_t i = 0; i < stack.size(); ++i) {
        if (i > 0) cout << ", ";
        cout << stack[i].toString();
    }
    cout << endl;
}

//============================================================================
// Отладочные методы
//============================================================================

void Interpreter::printState() const {
    cout << "\n┌─────────────────────────────────────────┐" << endl;
    cout << "│ Состояние виртуальной машины              │" << endl;
    cout << "├─────────────────────────────────────────┤" << endl;
    cout << "│ IP: " << setw(4) << ip << " / " << setw(4) << poliz.size() << "                    │" << endl;
    cout << "│ Шаг: " << setw(4) << currentStep << "                              │" << endl;
    cout << "├─────────────────────────────────────────┤" << endl;
    
    // Вывод стека
    cout << "│ Стек: ";
    if (stack.empty()) {
        cout << "(пуст)";
    } else {
        for (size_t i = 0; i < stack.size() && i < 10; ++i) {
            if (i > 0) cout << " ";
            cout << stack[i].toString();
            if (i < stack.size() - 1) cout << ",";
        }
        if (stack.size() > 10) cout << "...";
    }
    cout << string(max(0, 35 - (int)stack.size() * 5), ' ') << "│" << endl;
    
    // Следующая инструкция
    cout << "├─────────────────────────────────────────┤" << endl;
    if (ip < poliz.size()) {
        string instr = poliz[ip].toString();
        cout << "│ next: " << instr;
        cout << string(max(0, 35 - (int)instr.length()), ' ') << "│" << endl;
    } else {
        cout << "│ next: END                              │" << endl;
    }
    cout << "└─────────────────────────────────────────┘" << endl;
}

void Interpreter::waitForUser() const {
    if (singleStepMode) {
        cout << "Нажмите Enter для продолжения..." << endl;
        cin.get();
    }
}

//============================================================================
// Выполнение одной инструкции
//============================================================================

void Interpreter::executeInstruction(const PolizInstruction& instr) {
    switch (instr.cmd) {
        //-------------------------------------------------------------------
        // Константы
        //-------------------------------------------------------------------
        case PolizCmd::PUSH_INT:
            push(Value(instr.intValue));
            break;
            
        case PolizCmd::PUSH_REAL:
            push(Value(instr.realValue));
            break;
            
        case PolizCmd::PUSH_STRING:
            push(Value(instr.strValue));
            break;
            
        //-------------------------------------------------------------------
        // Переменные
        //-------------------------------------------------------------------
        case PolizCmd::PUSH_VAR:
            if (instr.address >= 0) {
                DataType type = symTable.getTypeByAddress(instr.address);
                switch (type) {
                    case DataType::INT:
                        push(Value(symTable.getIntValueByAddress(instr.address)));
                        break;
                    case DataType::REAL:
                        push(Value(symTable.getRealValueByAddress(instr.address)));
                        break;
                    case DataType::STRING:
                        push(Value(symTable.getStringValueByAddress(instr.address)));
                        break;
                    default:
                        throw runtime_error("Переменная по адресу " + 
                                          to_string(instr.address) + " не инициализирована");
                }
            } else {
                DataType type = symTable.getVariableType(instr.strValue);
                switch (type) {
                    case DataType::INT:
                        push(Value(symTable.getIntValue(instr.strValue)));
                        break;
                    case DataType::REAL:
                        push(Value(symTable.getRealValue(instr.strValue)));
                        break;
                    case DataType::STRING:
                        push(Value(symTable.getStringValue(instr.strValue)));
                        break;
                    default:
                        throw runtime_error("Переменная '" + instr.strValue + 
                                          "' не объявлена или не инициализирована");
                }
            }
            break;
            
        case PolizCmd::POP_VAR:
            {
                Value val = pop();
                if (instr.address >= 0) {
                    DataType varType = symTable.getTypeByAddress(instr.address);
                    switch (varType) {
                        case DataType::INT:
                            if (val.getType() == DataType::INT) {
                                symTable.setIntValueByAddress(instr.address, val.asInt());
                            } else if (val.getType() == DataType::REAL) {
                                symTable.setIntValueByAddress(instr.address, 
                                                              static_cast<int>(val.asReal()));
                            } else {
                                throw runtime_error("Несоответствие типов: int = " + 
                                                  dataTypeToString(val.getType()));
                            }
                            break;
                        case DataType::REAL:
                            symTable.setRealValueByAddress(instr.address, val.asReal());
                            break;
                        case DataType::STRING:
                            if (val.getType() == DataType::STRING) {
                                symTable.setStringValueByAddress(instr.address, val.asString());
                            } else {
                                throw runtime_error("Несоответствие типов: string = " + 
                                                  dataTypeToString(val.getType()));
                            }
                            break;
                        default:
                            throw runtime_error("Неизвестный тип переменной");
                    }
                } else {
                    DataType varType = symTable.getVariableType(instr.strValue);
                    switch (varType) {
                        case DataType::INT:
                            if (val.getType() == DataType::INT) {
                                symTable.setVariableValue(instr.strValue, val.asInt());
                            } else if (val.getType() == DataType::REAL) {
                                symTable.setVariableValue(instr.strValue, 
                                                          static_cast<int>(val.asReal()));
                            } else {
                                throw runtime_error("Несоответствие типов: int = " + 
                                                  dataTypeToString(val.getType()));
                            }
                            break;
                        case DataType::REAL:
                            symTable.setVariableValue(instr.strValue, val.asReal());
                            break;
                        case DataType::STRING:
                            if (val.getType() == DataType::STRING) {
                                symTable.setVariableValue(instr.strValue, val.asString());
                            } else {
                                throw runtime_error("Несоответствие типов: string = " + 
                                                  dataTypeToString(val.getType()));
                            }
                            break;
                        default:
                            throw runtime_error("Переменная '" + instr.strValue + 
                                              "' не объявлена");
                    }
                }
            }
            break;
            
        //-------------------------------------------------------------------
        // Арифметические операции
        //-------------------------------------------------------------------
        case PolizCmd::ADD:
            {
                Value right = pop();
                Value left = pop();
                push(left + right);
            }
            break;
            
        case PolizCmd::SUB:
            {
                Value right = pop();
                Value left = pop();
                push(left - right);
            }
            break;
            
        case PolizCmd::MUL:
            {
                Value right = pop();
                Value left = pop();
                push(left * right);
            }
            break;
            
        case PolizCmd::DIV:
            {
                Value right = pop();
                Value left = pop();
                push(left / right);
            }
            break;
            
        case PolizCmd::NEG:
            {
                Value operand = pop();
                push(-operand);
            }
            break;
            
        //-------------------------------------------------------------------
        // Логические операции (полное вычисление)
        //-------------------------------------------------------------------
        case PolizCmd::AND:
            {
                Value right = pop();
                Value left = pop();
                push(left && right);
            }
            break;
            
        case PolizCmd::OR:
            {
                Value right = pop();
                Value left = pop();
                push(left || right);
            }
            break;
            
        case PolizCmd::NOT:
            {
                Value operand = pop();
                push(!operand);
            }
            break;
            
        //-------------------------------------------------------------------
        // Операции отношения
        //-------------------------------------------------------------------
        case PolizCmd::LESS:
            {
                Value right = pop();
                Value left = pop();
                push(left < right);
            }
            break;
            
        case PolizCmd::GREATER:
            {
                Value right = pop();
                Value left = pop();
                push(left > right);
            }
            break;
            
        case PolizCmd::LE:
            {
                Value right = pop();
                Value left = pop();
                push(left <= right);
            }
            break;
            
        case PolizCmd::GE:
            {
                Value right = pop();
                Value left = pop();
                push(left >= right);
            }
            break;
            
        case PolizCmd::EQ:
            {
                Value right = pop();
                Value left = pop();
                push(left == right);
            }
            break;
            
        case PolizCmd::NE:
            {
                Value right = pop();
                Value left = pop();
                push(left != right);
            }
            break;
            
        //-------------------------------------------------------------------
        // Управление потоком
        //-------------------------------------------------------------------
        case PolizCmd::LABEL:
            // Ничего не делаем, метка нужна только для адресации
            break;
            
        case PolizCmd::GOTO:
            if (debugMode) {
                cout << "  безусловный переход на " << instr.intValue << endl;
            }
            ip = instr.intValue;
            return;  // не увеличиваем ip
            
        case PolizCmd::GOTO_IF_FALSE:
            {
                Value cond = pop();
                if (debugMode) {
                    cout << "  условный переход (false): cond=" << cond.toBool() 
                         << ", target=" << instr.intValue << endl;
                }
                if (!cond.toBool()) {
                    ip = instr.intValue;
                    return;
                }
            }
            break;
            
        case PolizCmd::GOTO_IF_TRUE:
            {
                Value cond = pop();
                if (debugMode) {
                    cout << "  условный переход (true): cond=" << cond.toBool() 
                         << ", target=" << instr.intValue << endl;
                }
                if (cond.toBool()) {
                    ip = instr.intValue;
                    return;
                }
            }
            break;
            
        //-------------------------------------------------------------------
        // Ввод/вывод
        //-------------------------------------------------------------------
        case PolizCmd::READ:
            {
                string input;
                if (inputCallback) {
                    input = inputCallback();
                } else {
                    if (!getline(cin, input)) {
                        throw runtime_error("Ошибка чтения ввода");
                    }
                }
                
                // Удаляем возможный символ возврата каретки
                if (!input.empty() && input.back() == '\r') {
                    input.pop_back();
                }
                
                if (debugMode) {
                    cout << "  read: \"" << input << "\" -> ";
                }
                
                if (instr.address >= 0) {
                    DataType type = symTable.getTypeByAddress(instr.address);
                    switch (type) {
                        case DataType::INT:
                            symTable.setIntValueByAddress(instr.address, stoi(input));
                            if (debugMode) cout << "int(" << stoi(input) << ")" << endl;
                            break;
                        case DataType::REAL:
                            symTable.setRealValueByAddress(instr.address, stod(input));
                            if (debugMode) cout << "real(" << stod(input) << ")" << endl;
                            break;
                        case DataType::STRING:
                            symTable.setStringValueByAddress(instr.address, input);
                            if (debugMode) cout << "string(\"" << input << "\")" << endl;
                            break;
                        default:
                            throw runtime_error("Неизвестный тип для read");
                    }
                } else {
                    DataType type = symTable.getVariableType(instr.strValue);
                    switch (type) {
                        case DataType::INT:
                            symTable.setVariableValue(instr.strValue, stoi(input));
                            break;
                        case DataType::REAL:
                            symTable.setVariableValue(instr.strValue, stod(input));
                            break;
                        case DataType::STRING:
                            symTable.setVariableValue(instr.strValue, input);
                            break;
                        default:
                            throw runtime_error("Переменная '" + instr.strValue + 
                                              "' не объявлена для read");
                    }
                }
            }
            break;
            
        case PolizCmd::WRITE:
            {
                Value val = pop();
                string output = val.toString();
                if (debugMode) {
                    cout << "  write: " << output << endl;
                }
                if (outputCallback) {
                    outputCallback(output);
                } else {
                    cout << output;
                }
            }
            break;
            
        //-------------------------------------------------------------------
        // Завершение
        //-------------------------------------------------------------------
        case PolizCmd::HALT:
            if (debugMode) {
                cout << "  HALT: выполнение завершено" << endl;
            }
            running = false;
            break;
            
        case PolizCmd::NOP:
            if (debugMode) {
                cout << "  NOP" << endl;
            }
            break;
            
        default:
            throw runtime_error("Неизвестная команда ПОЛИЗа: " + 
                              to_string(static_cast<int>(instr.cmd)));
    }
    
    ip++;
}

//============================================================================
// Пошаговое выполнение
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
            throw runtime_error("Превышено максимальное количество шагов (" + 
                              to_string(maxSteps) + "). Возможно зацикливание.");
        }
        
        if (debugMode && singleStepMode) {
            waitForUser();
        }
        
        return running;
    } catch (const exception& e) {
        stringstream ss;
        ss << "\nОшибка выполнения на шаге " << currentStep << " (IP=" << ip << "): " << e.what();
        throw runtime_error(ss.str());
    }
}

//============================================================================
// Запуск интерпретации
//============================================================================

void Interpreter::run(istream& input) {
    // Временно заменяем колбэк ввода, если не установлен пользовательский
    function<string()> oldInputCallback = inputCallback;
    if (!inputCallback) {
        inputCallback = [&input]() {
            string line;
            if (!getline(input, line)) {
                return string("");
            }
            return line;
        };
    }
    
    reset();
    running = true;
    currentStep = 0;
    
    if (debugMode) {
        cout << "\n=== НАЧАЛО ВЫПОЛНЕНИЯ ===" << endl;
        if (singleStepMode) {
            cout << "Режим пошагового выполнения. Нажимайте Enter для продолжения." << endl;
        }
        printState();
        if (singleStepMode) waitForUser();
    }
    
    try {
        while (running && ip < poliz.size()) {
            if (!step()) break;
        }
        
        if (debugMode) {
            cout << "\n=== ВЫПОЛНЕНИЕ ЗАВЕРШЕНО ===" << endl;
            cout << "Всего шагов: " << currentStep << endl;
            if (!stack.empty()) {
                cout << "Остаток стека: ";
                printStack();
            }
        }
    } catch (const exception& e) {
        // Восстанавливаем старый колбэк перед выбросом исключения
        if (!oldInputCallback) {
            inputCallback = nullptr;
        }
        throw;
    }
    
    // Восстанавливаем старый колбэк
    if (!oldInputCallback) {
        inputCallback = nullptr;
    }
}

//============================================================================
// Сброс состояния
//============================================================================

void Interpreter::reset() {
    clearStack();
    ip = 0;
    running = false;
    currentStep = 0;
    
    if (debugMode) {
        cout << "Интерпретатор сброшен" << endl;
    }
}

//============================================================================
// Установка стандартного ввода/вывода
//============================================================================

void Interpreter::setStandardIO() {
    outputCallback = [](const string& s) {
        cout << s;
    };
    inputCallback = []() -> string {
        string line;
        if (!getline(cin, line)) {
            return "";
        }
        return line;
    };
}

//============================================================================
// Дополнительные методы для отладки (опционально)
//============================================================================

// Метод для просмотра значения переменной по имени (для отладки)
// Можно добавить в класс при необходимости