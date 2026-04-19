#pragma once
// ============================================================
// symtable.h — таблица символов (переменных)
//
// Хранит все объявленные переменные: имя → { тип, значение, инициализирована }
//
// Тип значения — std::variant<long long, double, std::string>:
//   Это тип C++17, который в одном объекте хранит РОВНО ОДНО
//   из нескольких возможных значений. Безопасно и без union.
//
//   Например:
//     Val v = 42LL;          // хранит long long
//     Val v = 3.14;          // хранит double
//     Val v = std::string{}; // хранит string
//
// Правила совместимости типов при присваивании (Таблица №2 из ТЗ):
//   int   = int    → OK
//   real  = real   → OK
//   real  = int    → неявное расширение (3 → 3.0)
//   int   = real   → усечение (3.9 → 3)
//   string= string → OK
//   остальные      → ошибка несовместимости
// ============================================================
#include <string>
#include <variant>
#include <unordered_map>
#include <stdexcept>

// Тип переменной
enum class VType { INT, REAL, STRING };

// Val — значение переменной во время выполнения
using Val = std::variant<long long, double, std::string>;

// ---- Удобные функции для работы с Val ----

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
    if (std::holds_alternative<double>(v)) {
        // Убираем лишние нули: 3.140000 → 3.14
        std::string s = std::to_string(asReal(v));
        auto dot = s.find('.');
        if (dot != std::string::npos) {
            size_t last = s.find_last_not_of('0');
            if (last == dot) last++;   // хотя бы одна цифра после точки
            s = s.substr(0, last + 1);
        }
        return s;
    }
    return asStr(v);
}

// ---- Запись в таблице символов ----
struct SymEntry {
    VType type;
    Val   val;
    bool  initialized;   // была ли переменная инициализирована
};

// ---- Таблица символов ----
class SymTable {
    std::unordered_map<std::string, SymEntry> vars;

public:
    // Объявить переменную без начального значения
    void declare(const std::string& name, VType t) {
        if (vars.count(name))
            throw std::runtime_error(
                "Повторное объявление переменной '" + name + "'");
        Val def;
        if      (t == VType::INT)    def = 0LL;
        else if (t == VType::REAL)   def = 0.0;
        else                          def = std::string{};
        vars[name] = { t, def, false };
    }

    // Объявить и сразу инициализировать константой
    void declareInit(const std::string& name, VType t, Val v) {
        declare(name, t);
        set(name, v);
        vars[name].initialized = true;
    }

    // Объявить скрытую служебную переменную (для for: __step_N, __limit_N)
    // Не проверяем повторное объявление — эти имена генерируются внутри
    void declareHidden(const std::string& name, VType t) {
        Val def;
        if      (t == VType::INT)  def = 0LL;
        else if (t == VType::REAL) def = 0.0;
        else                        def = std::string{};
        vars[name] = { t, def, false };
    }

    // Получить запись о переменной (бросает если не объявлена)
    SymEntry& get(const std::string& name) {
        auto it = vars.find(name);
        if (it == vars.end())
            throw std::runtime_error(
                "Переменная '" + name + "' не объявлена");
        return it->second;
    }

    // Присвоить значение с проверкой совместимости типов
    void set(const std::string& name, Val v) {
        auto& e = get(name);
        if (e.type == VType::INT) {
            // int = real → усечение
            if (std::holds_alternative<double>(v))
                v = static_cast<long long>(asReal(v));
            else if (!std::holds_alternative<long long>(v))
                throw std::runtime_error(
                    "Несовместимость типов при присваивании в '" + name + "'");
        } else if (e.type == VType::REAL) {
            // real = int → расширение
            if (std::holds_alternative<long long>(v))
                v = static_cast<double>(asInt(v));
            else if (!std::holds_alternative<double>(v))
                throw std::runtime_error(
                    "Несовместимость типов при присваивании в '" + name + "'");
        } else {
            // string = только string
            if (!std::holds_alternative<std::string>(v))
                throw std::runtime_error(
                    "Несовместимость типов при присваивании в '" + name + "'");
        }
        e.val         = v;
        e.initialized = true;
    }

    // Прочитать значение переменной (бросает если не инициализирована)
    Val value(const std::string& name) {
        auto& e = get(name);
        if (!e.initialized)
            throw std::runtime_error(
                "Переменная '" + name + "' использована до инициализации");
        return e.val;
    }
};
