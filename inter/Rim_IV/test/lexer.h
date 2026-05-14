#ifndef LEXER_H
#define LEXER_H
#include "token.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <cctype>

class Lexer {
    std::string prog_text;   // entire program text
    size_t      pos;   // current position
    int         ln;    // line number (for error messages)

    // Current character (or '\0' if end of file)
    char cur() {return pos < prog_text.size() ? prog_text[pos] : '\0';}

    // Next character
    char nextChar() {return (pos + 1) < prog_text.size() ? prog_text[pos + 1] : '\0';}

    // read and advance
    char step_by() {
        char c = prog_text[pos++];
        if (c == '\n') ln++;
        return c;
    }

    // Skip all whitespace characters (space, \n, \t, \r)
    void skipSpace() {
        while (pos < prog_text.size() && std::isspace((unsigned char)cur()))
            step_by();
    }

    // Skip /* ... */ comment
    void skipComment() {
        step_by(); step_by();   // eat '/' and '*'
        while (pos + 1 < prog_text.size()) {
            if (cur() == '*' && nextChar() == '/') {
                step_by(); step_by();   // eat '*' and '/'
                return;
            }
            step_by();
        }
        throw std::runtime_error("Unclosed /* comment */");
    }

    // Read number: integer or real
    Token readNumber(int L) {
        std::string s;
        while (pos < prog_text.size() && std::isdigit((unsigned char)cur()))
            s += step_by();

        // If there is a dot followed by a digit → this is REAL
        if (cur() == '.' && std::isdigit((unsigned char)nextChar())) {
            s += step_by();   // add '.'
            while (pos < prog_text.size() && std::isdigit((unsigned char)cur()))
                s += step_by();
            return Token(TT::REAL_L, s, L);
        }

        return Token(TT::INT_L, s, L);
    }

    // Read quoted string "..."
    Token readString(int L) {
        step_by();   // skip opening quote
        std::string s;
        while (pos < prog_text.size() && cur() != '"') s += step_by();
        if (cur() != '"') throw std::runtime_error("Unclosed string (end of file reached)");
        step_by();   // skip closing quote
        return Token(TT::STR_L, s, L);
    }

    // Read identifier or keyword
    Token readIdentOrKeyword(int L) {
        std::string s;
        while (pos < prog_text.size() && (std::isalnum((unsigned char)cur()) || cur() == '_'))
            s += step_by();

        // Compare with keyword table
        if (s == "program") return Token(TT::LEX_PROGRAM, s, L);
        if (s == "int") return Token(TT::LEX_INT, s, L);
        if (s == "string") return Token(TT::LEX_STRING, s, L);
        if (s == "real") return Token(TT::LEX_REAL, s, L);
        if (s == "if") return Token(TT::LEX_IF, s, L);
        if (s == "else") return Token(TT::LEX_ELSE, s, L);
        if (s == "while") return Token(TT::LEX_WHILE, s, L);
        if (s == "for") return Token(TT::LEX_FOR, s, L);
        if (s == "step") return Token(TT::LEX_STEP, s, L);
        if (s == "until") return Token(TT::LEX_UNTIL, s, L);
        if (s == "do") return Token(TT::LEX_DO, s, L);
        if (s == "goto") return Token(TT::LEX_GOTO, s, L);
        if (s == "read") return Token(TT::LEX_READ, s, L);
        if (s == "write") return Token(TT::LEX_WRITE, s, L);
        if (s == "and") return Token(TT::LEX_AND, s, L);
        if (s == "or") return Token(TT::LEX_OR, s, L);
        if (s == "not") return Token(TT::LEX_NOT, s, L);

        // variable or label name
        return Token(TT::ID, s, L);
    }

public:
    explicit Lexer(std::string text) : prog_text(std::move(text)), pos(0), ln(1) {}

    // tokenize the text
    std::vector<Token> tokenize() {
        std::vector<Token> result;

        while (true) {
            // Skip whitespace and comments
            while (true) {
                skipSpace();
                if (cur() == '/' && nextChar() == '*') {
                    skipComment();
                    continue;
                }
                break;
            }

            // End of file
            if (cur() == '\0') {
                result.push_back(Token(TT::EOF_TOK, "", ln));
                break;
            }

            int L = ln;    // remember line number for current token
            char c = cur();

            // Number
            if (std::isdigit((unsigned char)c)) {
                result.push_back(readNumber(L));
                continue;
            }
            // Identifier / keyword
            if (std::isalpha((unsigned char)c) || c == '_') {
                result.push_back(readIdentOrKeyword(L));
                continue;
            }
            // String
            if (c == '"') {
                result.push_back(readString(L));
                continue;
            }

            // Two-character operators (check pair of characters)
            if (c=='<' && nextChar()=='=') { step_by(); step_by(); result.push_back(Token(TT::LE, "<=", L)); continue; }
            if (c=='>' && nextChar()=='=') { step_by(); step_by(); result.push_back(Token(TT::GE, ">=", L)); continue; }
            if (c=='=' && nextChar()=='=') { step_by(); step_by(); result.push_back(Token(TT::EQ, "==", L)); continue; }
            if (c=='!' && nextChar()=='=') { step_by(); step_by(); result.push_back(Token(TT::NEQ, "!=", L)); continue; }

            // Single-character operators
            step_by();
            switch (c) {
                case '+': result.push_back(Token(TT::PLUS, "+", L)); break;
                case '-': result.push_back(Token(TT::MINUS, "-", L)); break;
                case '*': result.push_back(Token(TT::STAR, "*", L)); break;
                case '/': result.push_back(Token(TT::SLASH, "/", L)); break;
                case '<': result.push_back(Token(TT::LT, "<", L)); break;
                case '>': result.push_back(Token(TT::GT, ">", L)); break;
                case '=': result.push_back(Token(TT::ASSIGN, "=", L)); break;
                case '(': result.push_back(Token(TT::LPAREN, "(", L)); break;
                case ')': result.push_back(Token(TT::RPAREN, ")", L)); break;
                case '{': result.push_back(Token(TT::LBRACE, "{", L)); break;
                case '}': result.push_back(Token(TT::RBRACE, "}", L)); break;
                case ';': result.push_back(Token(TT::SEMICOLON, ";", L)); break;
                case ',': result.push_back(Token(TT::COMMA, ",", L)); break;
                case ':': result.push_back(Token(TT::COLON, ":", L)); break;
                default: throw std::runtime_error("Line " + std::to_string(L) + ": unknown character: " + c);
            }
        }

        return result;
    }
};
#endif // LEXER_H