#include "lexer.h"
#include "Error.h"

enum class State {
    Start,
    Number,
    NumberFracMark,
    NumberFrac,
    NumberExpMark,
    NumberExpSign,
    NumberExp,
    Identifier,
    Command
};

void Tokenize(const std::string& input, std::vector<Token>& tokens) {
    State s = State::Start;

    std::string buffer; // stored lexeme
    size_t startPos = 0;   // start of token in original string

    auto commit = [&](TokenType type, std::string lexeme, size_t pos, std::optional<Number> number = std::nullopt) {
        if (number.has_value()) { 
            std::cout << "variant index: " << number->value.index() << std::endl;
            tokens.push_back(Token{ type, std::move(lexeme), pos, number });
        }
        else { tokens.push_back(Token{ type, std::move(lexeme), pos }); }
    };

    // begin a new token
    auto begin = [&](State next, size_t pos) {
        s = next;
        buffer.clear();
        startPos = pos;
    };

    for (size_t i = 0; i <= input.size(); i++) {
        // add a sentinel '\0' at the end of the input string
        unsigned char c = (i < input.size()) ? static_cast<unsigned char>(input[i]) : '\0';

        switch (s) {
            case State::Start: {
                if (c == '\0') {
                    // End sentinel token at the end + 1
                    commit(TokenType::End, "End", input.size());
                    return;
                }

                // ignore spaces at the start of a token
                if (isspace(c)) { break; }
                
                if (isdigit(c)) { begin(State::Number, i); buffer += static_cast<char>(c); break; }
                if (c == '.') { begin(State::NumberFracMark, i); buffer += '.'; break; }
                if (c == '\\') { begin(State::Command, i); break; }
                
                if (isalnum(c)) { begin(State::Identifier, i); buffer += static_cast<char>(c); break; }
                
                if (c == '{') { commit(TokenType::LBrace, "{", i); break; }
                if (c == '}') { commit(TokenType::RBrace, "}", i); break; }
                if (c == '(') { commit(TokenType::LParenthesis, "(", i); break; }
                if (c == ')') { commit(TokenType::RParenthesis, ")", i); break; }
                if (c == '[') { commit(TokenType::LBracket, "[", i); break; }
                if (c == ']') { commit(TokenType::RBracket, "]", i); break; }
                
                if (c == ',') { commit(TokenType::Comma, ",", i); break; }
                if (c == '+') { commit(TokenType::Plus, "+", i); break; }
                if (c == '-') { commit(TokenType::Minus, "-", i); break; }
                if (c == '*') { commit(TokenType::Star, "*", i); break; }
                if (c == '/') { commit(TokenType::Slash, "/", i); break; }
                if (c == '^') { commit(TokenType::Caret, "^", i); break; }
                if (c == '_') { commit(TokenType::Underscore, "_", i); break; }
                if (c == '=') { commit(TokenType::Equals, "=", i); break; }

                std::string msg = "Unexpected character '" + c + '\'';
                throw LexerError(i, msg);
            }

            case State::Number: {
                if (isdigit(c)) { buffer += c; break; }
                if (c == '.') { buffer += '.'; s = State::NumberFracMark; break; }

                if (c == 'e' || c == 'E') { buffer += static_cast<char>(c); s = State::NumberExpMark; break; }
                
                
                Number num;
                num.value = std::stoi(buffer);
                num.isInt = true;
                commit(TokenType::Number, buffer, startPos, num);
                s = State::Start;
                --i;
                break;
            }
            case State::NumberFracMark: {
                if (isdigit(c)) { buffer += static_cast<char>(c); s = State::NumberFrac; break; }
                
                std::string msg = "Expected digit after '.', instead got '" + c + '\'';
                throw LexerError(i, msg);
            }
            case State::NumberFrac: {
                if (isdigit(c)) { buffer += static_cast<char>(c); break; }
                
                //Number num{ std::stoll(buffer), false };
                Number num;
                num.value = std::stod(buffer);
                num.isInt = false;
                commit(TokenType::Number, buffer, startPos, num);
                s = State::Start;
                --i;
                break;
            }
            case State::NumberExpMark: {
                if (isdigit(c)) { buffer += static_cast<char>(c); s = State::NumberExp; break; }
                if (c == '-') { buffer += '-'; s = State::NumberExpSign; break; }
                
                std::string msg = "Expected digit or sign after 'E', instead got '" + c + '\'';
                throw LexerError(i, msg);
            }
            case State::NumberExpSign: {
                if (isdigit(c)) { buffer += static_cast<char>(c); s = State::NumberExp; break; }
                
                std::string msg = "Expected digit after \"E(sign)\", instead got '" + c + '\'';
                throw LexerError(i, msg);
            }
            case State::NumberExp: {
                if (isdigit(c)) { buffer += static_cast<char>(c); break; }
                
                Number num{ std::stod(buffer), false };
                commit(TokenType::Number, buffer, startPos, num);
                s = State::Start;
                --i;
                break;
            }
            case State::Identifier: {
                if (isalnum(c)) { buffer += c; break; } 

                commit(TokenType::Identifier, buffer, startPos);
                s = State::Start;
                --i;
                break;
            }
            case State::Command: {
                if (isalnum(c)) { buffer += c; break; }

                commit(TokenType::Command, buffer, startPos);
                s = State::Start;
                --i;
                break;
            }
        }
    }
    // after the loop, back to the start means it all got handled
    if (s == State::Start) {
        return;
    }

    std::string msg = "Internal lexer error. Reached end of input without sentinel";
    throw LexerError(UnknownPos, msg);

    return;
}