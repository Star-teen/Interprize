#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <string>
#include <variant>
#include <map>
#include <stdexcept>
#include <climits>

// Variable type
enum class VType { INT, REAL, STRING };

// Val — variable value at runtime
using Val = std::variant<int, double, std::string>;

// Helper function to check integer overflow before multiplication
inline bool willOverflow(int a, int b) {
    if (a == 0 || b == 0) return false;
    if (a > 0 && b > 0 && a > INT_MAX / b) return true;
    if (a < 0 && b < 0 && a < INT_MAX / b) return true;
    if (a > 0 && b < 0 && b < INT_MIN / a) return true;
    if (a < 0 && b > 0 && a < INT_MIN / b) return true;
    return false;
}

// Helper function to check integer overflow before addition
inline bool willOverflowAdd(int a, int b) {
    if ((b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b)) return true;
    return false;
}

// Functions to determine the active type of Val
inline VType valType(const Val& v) {
    if (std::holds_alternative<int>(v)) return VType::INT;
    if (std::holds_alternative<double>(v))    return VType::REAL;
    return VType::STRING;
}

inline int      asInt(const Val& v)  { return std::get<int>(v); }
inline double   asReal(const Val& v) { return std::get<double>(v); }
inline std::string asStr(const Val& v)  { return std::get<std::string>(v); }

// Convert Val to double (needed for int+real arithmetic → real)
inline double toReal(const Val& v) {
    if (std::holds_alternative<int>(v)) return static_cast<double>(asInt(v));
    return asReal(v);
}

// Convert Val to string for output via write
inline std::string valToStr(const Val& v) {
    if (std::holds_alternative<int>(v))
        return std::to_string(asInt(v));
    if (std::holds_alternative<double>(v))
        return std::to_string(asReal(v));
    return asStr(v);
}

// Symbol table entry
struct SymTab {
    VType type;
    Val   val;
    bool  initialized;
};

// Symbol table
class SymTable {
    std::map<std::string, SymTab> vars;

public:
    // Declare a variable without an initial value
    void declare(const std::string& name, VType t) {
        if (vars.count(name))
            throw std::runtime_error("Duplicate variable declaration: " + name);
        Val def; // default value
        if (t == VType::INT) def = 0;
        else if (t == VType::REAL) def = 0.0;
        else def = std::string{};
        vars[name] = { t, def, false };
    }

    // Declare and initialize with a constant
    void declareInit(const std::string& name, VType t, Val v) {
        declare(name, t);
        set(name, v);
        vars[name].initialized = true;
    }

    // Declare an iterator variable for for loops (can be overwritten)
    void declareIter(const std::string& name, VType t) {
        Val def;
        if (t == VType::INT)  def = 0;
        else if (t == VType::REAL) def = 0.0;
        else def = std::string{};
        vars[name] = { t, def, false };
    }

    // Get a variable entry (throws if not declared)
    SymTab& get(const std::string& name) {
        auto it = vars.find(name);
        if (it == vars.end())
            throw std::runtime_error("Variable " + name + " not declared");
        return it->second;
    }

    // Assign a value with type compatibility check
    void set(const std::string& name, Val v) {
        auto& cur = get(name);
        if (cur.type == VType::INT) { // int = real → truncation
            if (std::holds_alternative<double>(v)) {
                double dval = asReal(v);
                if (dval > INT_MAX || dval < INT_MIN) {throw std::runtime_error("Integer overflow: value out of int range");}
                v = static_cast<int>(dval);
            }
            else if (!std::holds_alternative<int>(v))
                throw std::runtime_error("Type mismatch in assignment to " + name);
        } else if (cur.type == VType::REAL) { // real = int → widening
            if (std::holds_alternative<int>(v))
                v = static_cast<double>(asInt(v));
            else if (!std::holds_alternative<double>(v))
                throw std::runtime_error("Type mismatch in assignment to " + name);
        } else { // string = only string
            if (!std::holds_alternative<std::string>(v))
                throw std::runtime_error("Type mismatch in assignment to " + name);
        }
        cur.val = v;
        cur.initialized = true;
    }

    // Read a variable's value (throws if not initialized)
    Val value(const std::string& name) {
        auto& cur = get(name);
        if (!cur.initialized)
            throw std::runtime_error("Variable " + name + " used before initialization");
        return cur.val;
    }
};

// Arithmetic operations with overflow checking
inline int safeAdd(int a, int b) {
    if (willOverflowAdd(a, b)) throw std::runtime_error("Integer overflow in addition");
    return a + b;
}

inline int safeSub(int a, int b) {
    if ((b > 0 && a < INT_MIN + b) || (b < 0 && a > INT_MAX + b))
        throw std::runtime_error("Integer overflow in subtraction");
    return a - b;
}

inline int safeMul(int a, int b) {
    if (willOverflow(a, b)) throw std::runtime_error("Integer overflow in multiplication");
    return a * b;
}

inline int safeDiv(int a, int b) {
    if (b == 0) throw std::runtime_error("Division by zero");
    if (a == INT_MIN && b == -1) throw std::runtime_error("Integer overflow in division");
    return a / b;
}

#endif // SYMTABLE_H