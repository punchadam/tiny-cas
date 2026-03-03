#include "lexer.h"
#include "parser.h"
#include <iostream>

bool getInputString(std::string& input) {
    input.clear();
    std::cout << "Input Expression (Type \"stop\" to stop): ";
    std::cin >> input;
    if (input == "stop" || input == "STOP") return false;
    return true;
}

int main(void) {
    std::cout << "Math Compiler v0.0.0\n\n";
    
    std::string input;
    while (getInputString(input)) {
        AST ast;
        Parser p;
        std::vector<Token> tokens;
        
        try {
            Tokenize(input, tokens);
        } catch (LexerError& e) {
            std::cerr << "Tokenize error at position " << e.pos << ": " << e.what() << "\n";
            return 1;
        } catch (std::exception& e) {
            std::cerr << "Unexpected error " << e.what() << "\n";
        }

        std::cout << "\nTokens:\n[ ";
        for (Token t : tokens) {
            std::cout << t.lexeme;
            if (!t.is(TokenType::End)) std::cout << ",";
            std::cout << " ";
        }
        std::cout << "]\n\n";

        try {
            p.parse(tokens, ast);
        } catch (ParserError& e) {
            std::cerr << "Parser error at position " << e.pos << ": " << e.what() << "\n";
        } catch (std::exception& e) {
            std::cerr << "Unexpected error " << e.what() << "\n";
        }

        std::cout << "Parsed AST:\n" << ast.toString() << "\n\n";
    }

    return 0;
}