#ifndef DEBUG_H
#define DEBUG_H

#include "poliz.h"
#include "symtable.h"
#include "parser.h"
#include "token.h"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <vector>
#include <stack>

//============================================================================
// Interpreter debugging class
// Writes all diagnostic information to program.log file
//============================================================================

class Debugger {
private:
    std::ofstream logFile;
    std::string logFilename;
    bool enabled;
    int stepCount;
    std::chrono::steady_clock::time_point startTime;

    // Convert OpCode to string
    std::string opCodeToString(OpCode code) {
        switch (code) {
            case OpCode::PUSH_INT: return "PUSH_INT";
            case OpCode::PUSH_REAL: return "PUSH_REAL";
            case OpCode::PUSH_STR: return "PUSH_STR";
            case OpCode::LOAD: return "LOAD";
            case OpCode::STORE: return "STORE";
            case OpCode::ADD: return "ADD";
            case OpCode::SUB: return "SUB";
            case OpCode::MUL: return "MUL";
            case OpCode::DIV: return "DIV";
            case OpCode::NEG: return "NEG";
            case OpCode::LT: return "LT";
            case OpCode::GT: return "GT";
            case OpCode::LE: return "LE";
            case OpCode::GE: return "GE";
            case OpCode::EQ: return "EQ";
            case OpCode::NEQ: return "NEQ";
            case OpCode::AND: return "AND";
            case OpCode::OR: return "OR";
            case OpCode::NOT: return "NOT";
            case OpCode::JMP: return "JMP";
            case OpCode::JZ: return "JZ";
            case OpCode::JNZ: return "JNZ";
            case OpCode::READ: return "READ";
            case OpCode::WRITE: return "WRITE";
            case OpCode::POP: return "POP";
            default: return "UNKNOWN";
        }
    }

    // Convert VType to string
    std::string vTypeToString(VType t) {
        switch (t) {
            case VType::INT: return "int";
            case VType::REAL: return "real";
            case VType::STRING: return "string";
            default: return "unknown";
        }
    }

    // Get current time in HH:MM:SS format
    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_r(&time_t, &tm);
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
            << std::setw(2) << tm.tm_min << ":"
            << std::setw(2) << tm.tm_sec;
        return oss.str();
    }

public:
    Debugger() : enabled(false), stepCount(0) {}

    // Initialize debugger
    void init(const std::string& sourceFilename) {
        // Generate log filename: source_name.log
        logFilename = sourceFilename;
        size_t dotPos = logFilename.find_last_of('.');
        if (dotPos != std::string::npos) {
            logFilename = logFilename.substr(0, dotPos);
        }
        logFilename += ".log";
        
        logFile.open(logFilename);
        if (!logFile.is_open()) {
            std::cerr << "Warning: could not create log file " << logFilename << std::endl;
            enabled = false;
        } else {
            enabled = true;
            stepCount = 0;
            startTime = std::chrono::steady_clock::now();
            writeHeader(sourceFilename);
        }
    }

    // Close log file
    void close() {
        if (enabled && logFile.is_open()) {
            writeFooter();
            logFile.close();
        }
    }

    // Write header
    void writeHeader(const std::string& sourceFilename) {
        if (!enabled) return;
        logFile << "╔════════════════════════════════════════════════════════════════════════════╗" << std::endl;
        logFile << "║                         INTERPRETER DEBUG LOG                             ║" << std::endl;
        logFile << "╠════════════════════════════════════════════════════════════════════════════╣" << std::endl;
        logFile << "║ Start time:      " << std::setw(40) << getCurrentTime() << " ║" << std::endl;
        logFile << "║ Source file:     " << std::setw(40) << sourceFilename << " ║" << std::endl;
        logFile << "╚════════════════════════════════════════════════════════════════════════════╝" << std::endl;
        logFile << std::endl;
    }

    // Write footer (statistics)
    void writeFooter() {
        if (!enabled) return;
        auto endTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        logFile << std::endl;
        logFile << "╔════════════════════════════════════════════════════════════════════════════╗" << std::endl;
        logFile << "║                         EXECUTION STATISTICS                              ║" << std::endl;
        logFile << "╠════════════════════════════════════════════════════════════════════════════╣" << std::endl;
        logFile << "║ Total steps:     " << std::setw(38) << stepCount << " ║" << std::endl;
        logFile << "║ Execution time:  " << std::setw(37) << elapsed << " ms ║" << std::endl;
        logFile << "╚════════════════════════════════════════════════════════════════════════════╝" << std::endl;
    }

    // Log lexical analysis information
    void logTokens(const std::vector<Token>& tokens) {
        if (!enabled) return;
        logFile << "┌────────────────────────────────────────────────────────────────────────────┐" << std::endl;
        logFile << "│ PHASE 1: LEXICAL ANALYSIS                                                 │" << std::endl;
        logFile << "├────────────────────────────────────────────────────────────────────────────┤" << std::endl;
        logFile << "│ Total tokens: " << tokens.size() << std::endl;
        logFile << "├────────────────────────────────────────────────────────────────────────────┤" << std::endl;
        
        for (size_t i = 0; i < tokens.size(); ++i) {
            logFile << "│ " << std::setw(4) << i << ": ";
            switch (tokens[i].type) {
                case TT::LEX_PROGRAM: logFile << "PROGRAM"; break;
                case TT::LEX_INT: logFile << "INT"; break;
                case TT::LEX_STRING: logFile << "STRING"; break;
                case TT::LEX_REAL: logFile << "REAL"; break;
                case TT::LEX_IF: logFile << "IF"; break;
                case TT::LEX_ELSE: logFile << "ELSE"; break;
                case TT::LEX_WHILE: logFile << "WHILE"; break;
                case TT::LEX_FOR: logFile << "FOR"; break;
                case TT::LEX_STEP: logFile << "STEP"; break;
                case TT::LEX_UNTIL: logFile << "UNTIL"; break;
                case TT::LEX_DO: logFile << "DO"; break;
                case TT::LEX_GOTO: logFile << "GOTO"; break;
                case TT::LEX_READ: logFile << "READ"; break;
                case TT::LEX_WRITE: logFile << "WRITE"; break;
                case TT::LEX_AND: logFile << "AND"; break;
                case TT::LEX_OR: logFile << "OR"; break;
                case TT::LEX_NOT: logFile << "NOT"; break;
                case TT::INT_L: logFile << "INT_LIT(" << tokens[i].val << ")"; break;
                case TT::REAL_L: logFile << "REAL_LIT(" << tokens[i].val << ")"; break;
                case TT::STR_L: logFile << "STR_LIT(\"" << tokens[i].val << "\")"; break;
                case TT::ID: logFile << "ID(" << tokens[i].val << ")"; break;
                case TT::PLUS: logFile << "PLUS"; break;
                case TT::MINUS: logFile << "MINUS"; break;
                case TT::STAR: logFile << "STAR"; break;
                case TT::SLASH: logFile << "SLASH"; break;
                case TT::LT: logFile << "LT"; break;
                case TT::GT: logFile << "GT"; break;
                case TT::LE: logFile << "LE"; break;
                case TT::GE: logFile << "GE"; break;
                case TT::EQ: logFile << "EQ"; break;
                case TT::NEQ: logFile << "NEQ"; break;
                case TT::ASSIGN: logFile << "ASSIGN"; break;
                case TT::LPAREN: logFile << "LPAREN"; break;
                case TT::RPAREN: logFile << "RPAREN"; break;
                case TT::LBRACE: logFile << "LBRACE"; break;
                case TT::RBRACE: logFile << "RBRACE"; break;
                case TT::SEMICOLON: logFile << "SEMICOLON"; break;
                case TT::COMMA: logFile << "COMMA"; break;
                case TT::COLON: logFile << "COLON"; break;
                case TT::EOF_TOK: logFile << "EOF"; break;
                default: logFile << "UNKNOWN"; break;
            }
            logFile << " (line " << tokens[i].line << ")" << std::endl;
        }
        logFile << "└────────────────────────────────────────────────────────────────────────────┘" << std::endl;
        logFile << std::endl;
    }

    // Log symbol table information
    void logSymbolTable(SymTable& sym) {
        if (!enabled) return;
        // In a real implementation, we would need to add a getAllVariables method to SymTable
        logFile << "┌────────────────────────────────────────────────────────────────────────────┐" << std::endl;
        logFile << "│ SYMBOL TABLE                                                              │" << std::endl;
        logFile << "├────────────────────────────────────────────────────────────────────────────┤" << std::endl;
        logFile << "│ Variables:                                                                │" << std::endl;
        logFile << "│   (Full information requires getAllVariables method in SymTable)          │" << std::endl;
        logFile << "└────────────────────────────────────────────────────────────────────────────┘" << std::endl;
        logFile << std::endl;
    }

    // Log POLIZ output
    void logPoliz(const std::vector<PolizOp>& code) {
        if (!enabled) return;
        logFile << "┌────────────────────────────────────────────────────────────────────────────┐" << std::endl;
        logFile << "│ PHASE 2: SYNTAX ANALYSIS AND POLIZ GENERATION                             │" << std::endl;
        logFile << "├────────────────────────────────────────────────────────────────────────────┤" << std::endl;
        logFile << "│ Total instructions: " << code.size() << std::endl;
        logFile << "├────────────────────────────────────────────────────────────────────────────┤" << std::endl;
        
        for (size_t i = 0; i < code.size(); ++i) {
            logFile << "│ " << std::setw(4) << i << ": ";
            logFile << opCodeToString(code[i].code);
            
            switch (code[i].code) {
                case OpCode::PUSH_INT:
                    logFile << " " << code[i].ival;
                    break;
                case OpCode::PUSH_REAL:
                    logFile << " " << code[i].rval;
                    break;
                case OpCode::PUSH_STR:
                case OpCode::LOAD:
                case OpCode::STORE:
                case OpCode::READ:
                    logFile << " " << code[i].sval;
                    break;
                case OpCode::JMP:
                case OpCode::JZ:
                case OpCode::JNZ:
                    logFile << " -> " << code[i].ival;
                    break;
                default:
                    break;
            }
            logFile << std::endl;
        }
        logFile << "└────────────────────────────────────────────────────────────────────────────┘" << std::endl;
        logFile << std::endl;
    }

    // Log execution step
    void logStep(size_t pc, const PolizOp& op, const std::stack<Val>& st, const std::string& additional = "") {
        if (!enabled) return;
        
        stepCount++;
        
        logFile << "┌────────────────────────────────────────────────────────────────────────────┐" << std::endl;
        logFile << "│ STEP " << std::setw(6) << stepCount << " | PC=" << std::setw(4) << pc << "                                 │" << std::endl;
        logFile << "├────────────────────────────────────────────────────────────────────────────┤" << std::endl;
        
        // Current instruction
        logFile << "│ Instruction: " << opCodeToString(op.code);
        switch (op.code) {
            case OpCode::PUSH_INT: logFile << " " << op.ival; break;
            case OpCode::PUSH_REAL: logFile << " " << op.rval; break;
            case OpCode::PUSH_STR: logFile << " \"" << op.sval << "\""; break;
            case OpCode::LOAD: logFile << " " << op.sval; break;
            case OpCode::STORE: logFile << " " << op.sval; break;
            case OpCode::READ: logFile << " " << op.sval; break;
            case OpCode::JMP: logFile << " -> " << op.ival; break;
            case OpCode::JZ: logFile << " -> " << op.ival << " (if top == 0)"; break;
            case OpCode::JNZ: logFile << " -> " << op.ival << " (if top != 0)"; break;
            default: break;
        }
        logFile << std::endl;
        
        // Additional information
        if (!additional.empty()) {
            logFile << "│ Info: " << additional << std::endl;
        }
        
        // Stack
        logFile << "├────────────────────────────────────────────────────────────────────────────┤" << std::endl;
        logFile << "│ Stack (top on the right): [";
        
        // Copy stack for output (std::stack has no iterators)
        std::stack<Val> temp = st;
        std::vector<std::string> stackValues;
        while (!temp.empty()) {
            Val v = temp.top();
            temp.pop();
            if (std::holds_alternative<long long>(v)) {
                stackValues.push_back(std::to_string(asInt(v)));
            } else if (std::holds_alternative<double>(v)) {
                stackValues.push_back(std::to_string(asReal(v)));
            } else {
                stackValues.push_back("\"" + asStr(v) + "\"");
            }
        }
        for (auto it = stackValues.rbegin(); it != stackValues.rend(); ++it) {
            logFile << *it;
            if (std::next(it) != stackValues.rend()) logFile << ", ";
        }
        
        logFile << "]" << std::endl;
        logFile << "└────────────────────────────────────────────────────────────────────────────┘" << std::endl;
        logFile << std::endl;
    }

    // Log error
    void logError(const std::string& error, const std::string& phase) {
        if (!enabled) return;
        logFile << "╔════════════════════════════════════════════════════════════════════════════╗" << std::endl;
        logFile << "║ ERROR IN PHASE: " << std::setw(41) << phase << " ║" << std::endl;
        logFile << "╠════════════════════════════════════════════════════════════════════════════╣" << std::endl;
        logFile << "║ " << error << std::endl;
        logFile << "╚════════════════════════════════════════════════════════════════════════════╝" << std::endl;
    }

    // Log execution start
    void logExecutionStart() {
        if (!enabled) return;
        logFile << "┌────────────────────────────────────────────────────────────────────────────┐" << std::endl;
        logFile << "│ PHASE 3: PROGRAM EXECUTION                                                │" << std::endl;
        logFile << "└────────────────────────────────────────────────────────────────────────────┘" << std::endl;
        logFile << std::endl;
    }

    // Log successful execution end
    void logExecutionEnd() {
        if (!enabled) return;
        logFile << std::endl;
        logFile << "┌────────────────────────────────────────────────────────────────────────────┐" << std::endl;
        logFile << "│ EXECUTION COMPLETED SUCCESSFULLY                                          │" << std::endl;
        logFile << "└────────────────────────────────────────────────────────────────────────────┘" << std::endl;
    }

    // Check if debugger is enabled
    bool isEnabled() const { return enabled; }
};

#endif // DEBUG_H