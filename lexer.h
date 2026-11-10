#pragma once

#include <vector>
#include <string>
#include <stdexcept>

bool isNumber(const char c) {
    unsigned char usc = static_cast<unsigned char>(c);
    return (usc >= '0' && usc <= '9');
}
bool isLetter(const char c) {
    unsigned char usc = static_cast<unsigned char>(c);
    return ((usc >= 'A' && usc <= 'Z') || (usc >= 'a' && usc <= 'z'));
}

enum class TokenType {
    IDENTIFIER,
    CONSTANT,
    NUMBER,
    LPAREN,
    RPAREN,
    OPERATOR
};

struct Token {
    TokenType type;
    std::string lexeme;

    Token(TokenType t, std::string l) {
        type = t;
        lexeme = l;
    }
};

enum class LexerStates {
    START,
    READING_NUMBER,
    READING_DECIMAL,
    READING_FRAC,
    READING_TEXT
};

inline void Lex(const std::string& expression, std::vector<Token>& tokens) {
    LexerStates currentState = LexerStates::START;
    std::string lexeme;
    size_t i = 0;
    char c;
    while (i < expression.size()) {
        c = expression[i];
        switch (currentState) {
            case LexerStates::START:
                lexeme.clear();
                if (isNumber(c)) {
                    currentState = LexerStates::READING_NUMBER;
                    break;
                } if (c == '.') {
                    currentState = LexerStates::READING_DECIMAL;
                    lexeme.push_back(c);
                    i++;
                    break;
                } if (isLetter(c)) {
                    currentState = LexerStates::READING_TEXT;
                    break;
                } if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^') {
                    tokens.emplace_back(TokenType::OPERATOR, std::string(1, c));
                    i++;
                    break;
                } if (c == '(') {
                    tokens.emplace_back(TokenType::LPAREN, "(");
                    i++;
                    break;
                } if (c == ')') {
                    tokens.emplace_back(TokenType::RPAREN, ")");
                    i++;
                    break;
                } if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    i++;
                    break;
                } else {
                    std::string msg = "Unknown Character: \'";
                    msg += c;
                    msg += "\' at position: ";
                    msg += std::to_string(i);
                    msg += "\n";
                    throw std::invalid_argument(msg);
                }
                break;
            case LexerStates::READING_NUMBER:
                if (i < expression.size() && isNumber(expression[i])) {
                    lexeme.push_back(expression[i]);
                    i++;
                } else if (i < expression.size() && expression[i] == '.') {
                    lexeme.push_back(expression[i]);
                    i++;
                    currentState = LexerStates::READING_FRAC;
                } else {
                    tokens.emplace_back(TokenType::NUMBER, lexeme);
                    currentState = LexerStates::START;
                }
                break;
            case LexerStates::READING_DECIMAL:
                if (i < expression.size() && isNumber(expression[i])) {
                    lexeme.push_back(expression[i]);
                    i++;
                    currentState = LexerStates::READING_FRAC;
                } else {
                    std::string msg = "Decimal must be followed by number, instead got \'";
                    msg += c;
                    msg += "\' at position: ";
                    msg += std::to_string(i);
                    msg += "\n";
                    throw std::invalid_argument(msg);
                }
                break;
            case LexerStates::READING_FRAC:
                if (i < expression.size() && expression[i] == '.') {
                    std::string msg = "More than one decimal in one number, at position: ";
                    msg += std::to_string(i);
                    msg += "\n";
                    throw std::invalid_argument(msg);
                } if (i < expression.size() && isNumber(expression[i])) {
                    lexeme.push_back(expression[i]);
                    i++;
                } else {
                    tokens.emplace_back(TokenType::NUMBER, lexeme);
                    currentState = LexerStates::START;
                }
                break;
            case LexerStates::READING_TEXT:
                if (i < expression.size() && isLetter(expression[i])) {
                    lexeme.push_back(expression[i]);
                    i++;
                } else {
                    if (lexeme == "e" || lexeme == "pi") {
                        tokens.emplace_back(TokenType::CONSTANT, lexeme);
                    } else {
                        tokens.emplace_back(TokenType::IDENTIFIER, lexeme);
                    }
                    currentState = LexerStates::START;
                }
                break;
        }
    }

    // remaining tokens left in buffer
    if (currentState == LexerStates::READING_NUMBER || currentState == LexerStates::READING_FRAC) {
        tokens.emplace_back(TokenType::NUMBER, lexeme);
    } else if (currentState == LexerStates::READING_TEXT) {
        if (lexeme == "e" || lexeme == "pi") {
            tokens.emplace_back(TokenType::CONSTANT, lexeme);
        } else {
            tokens.emplace_back(TokenType::IDENTIFIER, lexeme);
        }
    }
}