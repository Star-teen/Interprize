// interpreter.h
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "poliz.h"
#include "symbol_table.h"
#include "value.h"
#include <vector>
#include <functional>
#include <iostream>

//============================================================================
// Класс интерпретатора ПОЛИЗа (виртуальная машина)
// Реализует выполнение внутреннего представления программы
//============================================================================

class Interpreter {
private:
    //-----------------------------------------------------------------------
    // Состояние виртуальной машины
    //-----------------------------------------------------------------------
    
    const Poliz& poliz;             // ссылка на выполняемую программу (ПОЛИЗ)
    SymbolTable& symTable;          // ссылка на таблицу символов (данные программы)
    
    std::vector<Value> stack;       // стек операндов
    size_t ip;                      // instruction pointer (счётчик команд)
    bool running;                   // флаг выполнения
    
    //-----------------------------------------------------------------------
    // Отладка
    //-----------------------------------------------------------------------
    
    bool debugMode;                 // режим пошаговой отладки
    int maxSteps;                   // максимальное количество шагов (защита от зацикливания)
    int currentStep;                // текущий шаг выполнения
    bool singleStepMode;            // режим пошагового выполнения (ожидание Enter)
    
    //-----------------------------------------------------------------------
    // Приватные методы
    //-----------------------------------------------------------------------
    
    // Работа со стеком
    void push(const Value& v);
    Value pop();
    Value peek() const;
    size_t stackSize() const { return stack.size(); }
    void clearStack();
    
    // Выполнение одной инструкции
    void executeInstruction(const PolizInstruction& instr);
    
    // Отладочный вывод
    void printState() const;
    void waitForUser() const;
    
public:
    //-----------------------------------------------------------------------
    // Конструкторы и деструктор
    //-----------------------------------------------------------------------
    
    explicit Interpreter(const Poliz& p, SymbolTable& st);
    ~Interpreter() = default;
    
    //-----------------------------------------------------------------------
    // Управление выполнением
    //-----------------------------------------------------------------------
    
    // Запуск интерпретации (основной метод)
    void run(std::istream& input = std::cin);
    
    // Выполнение одного шага (для отладки)
    bool step();
    
    // Сброс состояния (очистка стека, сброс IP)
    void reset();
    
    // Останов выполнения
    void stop() { running = false; }
    
    //-----------------------------------------------------------------------
    // Настройка интерпретатора
    //-----------------------------------------------------------------------
    
    void setDebugMode(bool enable) { debugMode = enable; }
    void setSingleStepMode(bool enable) { singleStepMode = enable; }
    void setMaxSteps(int steps) { maxSteps = steps; }
    
    //-----------------------------------------------------------------------
    // Получение состояния
    //-----------------------------------------------------------------------
    
    bool isRunning() const { return running; }
    size_t getIP() const { return ip; }
    size_t getStackSize() const { return stack.size(); }
    void printStack() const;
    
    //-----------------------------------------------------------------------
    // Колбэки для ввода/вывода (позволяют переопределить стандартный ввод/вывод)
    //-----------------------------------------------------------------------
    
    // Колбэк для вывода: вызывается при каждой операции WRITE
    std::function<void(const std::string&)> outputCallback;
    
    // Колбэк для ввода: вызывается при каждой операции READ
    std::function<std::string()> inputCallback;
    
    // Установка стандартного ввода/вывода (консоль)
    void setStandardIO();
    
    // Установка пользовательских колбэков
    void setOutputCallback(std::function<void(const std::string&)> callback) {
        outputCallback = callback;
    }
    
    void setInputCallback(std::function<std::string()> callback) {
        inputCallback = callback;
    }
};

#endif // INTERPRETER_H