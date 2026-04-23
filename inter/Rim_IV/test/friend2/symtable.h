#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <string>
#include <variant>
#include <map>
#include <stdexcept>

// Тип переменной
enum class VType { INT, REAL, STRING };

// Val — значение переменной во время выполнения
using Val = std::variant<long long, double, std::string>;

// при каждом значении Val вызов свой
inline VType valType(const Val& v) {
    if (std::holds_alternative<long long>(v)) return VType::INT;
    if (std::holds_alternative<double>(v))    return VType::REAL;
    return VType::STRING;
}

inline long long   asInt(const Val& v)  { return std::get<long long>(v); }
inline double      asReal(const Val& v) { return std::get<double>(v); }
inline std::string asStr(const Val& v)  { return std::get<std::string>(v); }

// Привести Val к double (нужно для арифметики int+real → real)
inline double toReal(const Val& v) {
    if (std::holds_alternative<long long>(v)) return static_cast<double>(asInt(v));
    return asReal(v);
}

// Преобразовать Val в строку для вывода через write
inline std::string valToStr(const Val& v) {
    if (std::holds_alternative<long long>(v))
        return std::to_string(asInt(v));
    if (std::holds_alternative<double>(v))
        return std::to_string(asReal(v));
    return asStr(v);
}

// Запись в таблице символов
struct SymTab {
    VType type;
    Val   val;
    bool  initialized;
};

// Таблица символов
class SymTable {
    std::map<std::string, SymTab> vars;

public:
    // Объявить переменную без начального значения
    void declare(const std::string& name, VType t) {
        if (vars.count(name))
            throw std::runtime_error("Повторное объявление переменной: " + name);
        Val def; // значение по умолчанию
        if (t == VType::INT) def = 0LL;
        else if (t == VType::REAL) def = 0.0;
        else def = std::string{};
        vars[name] = { t, def, false };
    }

    // Объявить и сразу инициализировать константой
    void declareInit(const std::string& name, VType t, Val v) {
        declare(name, t);
        set(name, v);
        vars[name].initialized = true;
    }

    // Объявить переменную итератор для for
    // Имя можно перезаписывать
    void declareIter(const std::string& name, VType t) {
        Val def;
        if (t == VType::INT)  def = 0LL;
        else if (t == VType::REAL) def = 0.0;
        else def = std::string{};
        vars[name] = { t, def, false };
    }

    // Получить запись о переменной
    SymTab& get(const std::string& name) {
        auto it = vars.find(name);
        if (it == vars.end())
            throw std::runtime_error("Переменная " + name + " не объявлена");
        return it->second;
    }

    // Присвоить значение с учетом совместимости
    void set(const std::string& name, Val v) {
        auto& cur = get(name);
        if (cur.type == VType::INT) { // int = real → усечение
            if (std::holds_alternative<double>(v)) v = static_cast<long long>(asReal(v));
            else if (!std::holds_alternative<long long>(v))
                throw std::runtime_error("Несовместимость типов при присваивании в " + name);
        } else if (cur.type == VType::REAL) { // real = int → расширение
            if (std::holds_alternative<long long>(v)) v = static_cast<double>(asInt(v));
            else if (!std::holds_alternative<double>(v))
                throw std::runtime_error("Несовместимость типов при присваивании в " + name);
        } else { // string = только string
            if (!std::holds_alternative<std::string>(v))
                throw std::runtime_error("Несовместимость типов при присваивании в " + name);
        }
        cur.val = v;
        cur.initialized = true;
    }

    // Прочитать значение переменной
    Val value(const std::string& name) {
        auto& cur = get(name);
        if (!cur.initialized)
            throw std::runtime_error("Переменная " + name + " использована до инициализации");
        return cur.val;
    }
};
#endif // SYMTABLE_H