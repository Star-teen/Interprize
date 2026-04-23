#ifndef LEXER_H
#define LEXER_H
#include "token.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <cctype>

class Lexer {
    std::string prog_text;   // весь текст программы
    size_t      pos;   // текущая позиция
    int         ln;    // номер строки (для сообщений об ошибках)

    // Текущий символ (или '\0' если конец файла)
    char cur() {return pos < prog_text.size() ? prog_text[pos] : '\0';}

    // Следующий символ
    char nextChar() {return (pos + 1) < prog_text.size() ? prog_text[pos + 1] : '\0';}

    // прочитать и перейти дальше
    char step_by() {
        char c = prog_text[pos++];
        if (c == '\n') ln++;
        return c;
    }

    // Пропустить все пробельные символы (пробел, \n, \t, \r)
    void skipSpace() {
        while (pos < prog_text.size() && std::isspace((unsigned char)cur()))
            step_by();
    }

    // Пропустить комментарий /* ... */
    void skipComment() {
        step_by(); step_by();   // съедаем '/' и '*'
        while (pos + 1 < prog_text.size()) {
            if (cur() == '*' && nextChar() == '/') {
                step_by(); step_by();   // съедаем '*' и '/'
                return;
            }
            step_by();
        }
        throw std::runtime_error("Незакрытый комментарий /* без */");
    }

    // Прочитать число: целое или вещественное
    Token readNumber(int L) {
        std::string s;
        while (pos < prog_text.size() && std::isdigit((unsigned char)cur()))
            s += step_by();

        // Если после цифр стоит точка и ещё цифра → это REAL
        if (cur() == '.' && std::isdigit((unsigned char)nextChar())) {
            s += step_by();   // добавляем '.'
            while (pos < prog_text.size() && std::isdigit((unsigned char)cur()))
                s += step_by();
            return Token(TT::REAL_L, s, L);
        }

        return Token(TT::INT_L, s, L);
    }

    // Прочитать строку в кавычках "..."
    Token readString(int L) {
        step_by();   // пропускаем открывающую "
        std::string s;
        while (pos < prog_text.size() && cur() != '"') s += step_by();
        if (cur() != '"') throw std::runtime_error("Строка не закрыта (достигнут конец файла)");
        step_by();   // пропускаем закрывающую "
        return Token(TT::STR_L, s, L);
    }

    // Прочитать идентификатор или служебное слово
    Token readIdentOrKeyword(int L) {
        std::string s;
        while (pos < prog_text.size() && (std::isalnum((unsigned char)cur()) || cur() == '_'))
            s += step_by();

        // Сравниваем с таблицей ключевых слов
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

        // имя переменной или метки
        return Token(TT::ID, s, L);
    }

public:
    explicit Lexer(std::string text) : prog_text(std::move(text)), pos(0), ln(1) {}

    //разбить текст на токены
    std::vector<Token> tokenize() {
        std::vector<Token> result;

        while (true) {
            // Пропускаем пробелы и комментарии
            while (true) {
                skipSpace();
                if (cur() == '/' && nextChar() == '*') {
                    skipComment();
                    continue;
                }
                break;
            }

            // Конец файла
            if (cur() == '\0') {
                result.push_back(Token(TT::EOF_TOK, "", ln));
                break;
            }

            int L = ln;    // запоминаем строку текущего токена
            char c = cur();

            // Число
            if (std::isdigit((unsigned char)c)) {
                result.push_back(readNumber(L));
                continue;
            }
            // Идентификатор / ключевое слово
            if (std::isalpha((unsigned char)c) || c == '_') {
                result.push_back(readIdentOrKeyword(L));
                continue;
            }
            // Строка
            if (c == '"') {
                result.push_back(readString(L));
                continue;
            }

            // Двухсимвольные операторы (проверяем пару символов)
            if (c=='<' && nextChar()=='=') { step_by(); step_by(); result.push_back(Token(TT::LE, "<=", L)); continue; }
            if (c=='>' && nextChar()=='=') { step_by(); step_by(); result.push_back(Token(TT::GE, ">=", L)); continue; }
            if (c=='=' && nextChar()=='=') { step_by(); step_by(); result.push_back(Token(TT::EQ, "==", L)); continue; }
            if (c=='!' && nextChar()=='=') { step_by(); step_by(); result.push_back(Token(TT::NEQ, "!=", L)); continue; }

            // Односимвольные операторы
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
                default: throw std::runtime_error("Строка " + std::to_string(L) + ": неизвестный символ: " + c);
            }
        }

        return result;
    }
};
#endif // LEXER_H
