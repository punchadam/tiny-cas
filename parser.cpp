#include "parser.h"

#include <stdexcept>
#include <iostream>

void Parser::parse(const std::vector<Token>& tokens, AST& ast) {
    _tokens = &tokens;
    _ast = &ast;

    _ast->root = parseExpression(0);

    if (peek().is(TokenType::End)) return;
    else {
        std::string msg = "Function parseExpression() didn't reach end of token stream";
        throw ParserError(_pos, msg);
    }
}


const Token& Parser::peek() const {
    return (*_tokens)[_pos];
}

const Token& Parser::advance() {
    const Token& t = (*_tokens)[_pos];
    if (!t.is(TokenType::End)) _pos++;
    return t;
}

const Token& Parser::expect(const TokenType& type) {
    if ((*_tokens)[_pos].type == type) {
        return advance();
    }

    std::string msg = "Expected " + std::to_string((int)type) + ", got \"" + peek().lexeme + "\"";
    throw ParserError(_pos, msg);
}

NodeID Parser::parseExpression(const u8& minBP) {
    NodeID leftSide = parsePrefix();

    size_t iteration = 0;
    size_t MAX_ITERATIONS = 10000;
    while (true) {
        if (++iteration >= MAX_ITERATIONS) {
            std::string msg = "Infinite Loop on Token: \"" + peek().lexeme + "\", Type: " + std::to_string(static_cast<int>(peek().type));
            throw ParserError(_pos, msg);
        }

        const Token& t = peek();

        // postfix ops
        if (t.is(TokenType::Identifier) && t.lexeme == "!") {
            if (POSTFIX_LBP < minBP) break;
            size_t p = advance().pos;
            leftSide = _ast->addUnaryOp(UnaryOpKind::Factorial, leftSide, p);
            continue;
        }

        // infix ops
        if (auto it = INFIX_OPS.find(t.type); it != INFIX_OPS.end()) {
            auto [leftBP, rightBP, opKind] = it->second;
            if (leftBP < minBP) break;
            const Token& op = advance();
            NodeID rightSide = parseExpression(rightBP);
            leftSide = _ast->addBinaryOp(opKind, leftSide, rightSide, op.pos);
            continue;
        }

        // infix commands
        if (auto it = INFIX_COMMAND_OPS.find(t.lexeme); it != INFIX_COMMAND_OPS.end()) {
            if (5 < minBP) break;
            BinaryOpKind kind = (t.lexeme == "div") ? BinaryOpKind::Divide : BinaryOpKind::Multiply;
            size_t p = advance().pos;
            NodeID rightSide = parseExpression(6);
            leftSide = _ast->addBinaryOp(kind, leftSide, rightSide, p);
            continue;
        }

        // implicit multiplication
        if (canImplicitMultiply()) {
            u8 leftBP = 5;
            u8 rightBP = 6;
            if (leftBP < minBP) break;
            size_t p = peek().pos;
            NodeID rightSide = parseExpression(rightBP);
            leftSide = _ast->addBinaryOp(BinaryOpKind::Multiply, leftSide, rightSide, p);
            continue;
        }

        break;
    }

    return leftSide;
}

// numbers, identifiers, unary '-', groups, commands
NodeID Parser::parsePrefix() {
    const Token& t = peek();

    if (t.is(TokenType::Number)) return parseNumber();

    if (t.is(TokenType::Identifier)) {
        const Token& t = advance();
        return _ast->addIdentifier(t.lexeme, t.pos);
    }

    if (t.is(TokenType::Minus)) {
        size_t p = advance().pos;
        NodeID inner = parseExpression(PREFIX_UNARY_RBP);
        return _ast->addUnaryOp(UnaryOpKind::Negate, inner, p);
    }

    if (t.is(TokenType::LParenthesis)) {
        advance();
        NodeID inner = parseExpression(0);
        expect(TokenType::RParenthesis);
        return inner;
    }

    if (t.is(TokenType::RBrace)) {
        return parseBraceGroup();
    }

    if (t.is(TokenType::Command)) {
        return parseCommand();
    }

    std::string msg = "Unexpected token: \"" + t.lexeme + "\"";
    throw ParserError(_pos, msg);
}

NodeID Parser::parseNumber() {
    const Token& t = advance();
    if (t.isInt()) {
        return _ast->addRational(std::get<i64>(t.number->value), 1, t.pos);
    }
    double val = std::get<double>(t.number->value);
    i64 num, den;
    if (doubleToRational(val, num, den)) {
        return _ast->addRational(num, den, t.pos);
    }
    return _ast->addReal(std::get<double>(t.number->value), t.pos);
}

NodeID Parser::parseCommand() {
    const Token& t = peek();
    const std::string& cmd = t.lexeme;

    if (auto it = CONSTANT_MAP.find(cmd); it != CONSTANT_MAP.end()) {
        size_t p = advance().pos;
        return _ast->addConstant(it->second, p);
    }

    if (auto it = FUNCTION_KIND_MAP.find(cmd); it != FUNCTION_KIND_MAP.end()) {
        return parseSingleArgFunction();
    }

    if (auto it = MULTI_ARG_FUNCTION_KIND_MAP.find(cmd); it != MULTI_ARG_FUNCTION_KIND_MAP.end()) {
        return parseMultiArgFunction();
    }

    if (cmd == "operatorname") {
        return parseOperatorName();
    }

    if (cmd == "frac") {
        return parseFraction();
    }

    if (cmd == "sqrt") {
        size_t p = advance().pos;
        NodeID inner = parseBraceGroup();

        // sqrt(x) = x^(1/2)
        NodeID half = _ast->addRational(1, 2, p);
        return _ast->addBinaryOp(BinaryOpKind::Power, inner, half, p);
    }

    if (cmd == "left") {
        return parseLeftRight();
    }

    std::string msg = "Unknown command: " + cmd;
    throw ParserError(_pos, msg);
}

NodeID Parser::parseBraceGroup() {
    expect(TokenType::LBrace);
    NodeID inner = parseExpression(0);
    expect(TokenType::RBrace);
    return inner;
}

NodeID Parser::parseLeftRight() {
    advance();
    expect(TokenType::LParenthesis);
    NodeID inner = parseExpression(0);
    if (!(peek().is(TokenType::Command) && peek().lexeme != "right")) {
        std::string msg = "Expected \"right\", got \"" + peek().lexeme + "\"";
        throw ParserError(_pos, msg);
    }
    advance();
    expect(TokenType::RParenthesis);
    return inner;
}

NodeID Parser::parseFraction() {
    // consume "\frac"
    size_t p = advance().pos;

    // store the original pos so we can use advance freely
    size_t saved = _pos;

    // fast path if numerator and denominator can be rationalized
    if (peek().is(TokenType::LBrace)) {
        advance();
        bool negativeNumerator = peek().is(TokenType::Minus);
        if (negativeNumerator) advance();
        if (peek().is(TokenType::Number)) {
            i64 n_numerator, n_denominator = 1;
            bool gotNumerator = false;
            if (peek().isInt()) {
                n_numerator = std::get<i64>(advance().number->value);
                gotNumerator = true;
            } else {
                double val = std::get<double>(advance().number->value);
                gotNumerator = doubleToRational(val, n_numerator, n_denominator);
            }

            if (negativeNumerator) n_numerator = -n_numerator;

            if (gotNumerator && peek().is(TokenType::RBrace)) {
                advance();
                bool negativeDenominator = peek().is(TokenType::Minus);
                if (negativeDenominator) advance();
                if (peek().is(TokenType::Number)) {
                    i64 d_numerator, d_denominator = 1;
                    bool gotDenominator = false;
                    if (peek().isInt()) {
                        d_numerator = std::get<i64>(advance().number->value);
                        gotDenominator = true;
                    } else {
                        double val = std::get<double>(advance().number->value);
                        gotDenominator = doubleToRational(val, d_numerator, d_denominator);
                    }

                    if (negativeDenominator) d_numerator = -d_numerator;

                    if (gotDenominator && peek().is(TokenType::RBrace)) {
                        advance();
                        i64 final_numerator = d_denominator * n_numerator;
                        i64 final_denominator = n_denominator * d_numerator;
                        return _ast->addRational(final_numerator, final_denominator, p);
                    }
                } 
            }
        }
    }

    // slow path for whatever else
    _pos = saved;
    NodeID numerator = parseBraceGroup();
    NodeID denominator = parseBraceGroup();
    return _ast->addBinaryOp(BinaryOpKind::Divide, numerator, denominator, p);
}

NodeID Parser::parseSingleArgFunction() {
    const Token& t = advance();
    FunctionKind fKind = FUNCTION_KIND_MAP.at(t.lexeme);
    size_t p = t.pos;

    NodeID arg;
    if (peek().is(TokenType::LBrace)) {
        arg = parseBraceGroup();
    } else if (peek().is(TokenType::LParenthesis)) {
        advance();
        arg = parseExpression(0);
        expect(TokenType::RParenthesis);
    } else {
        arg = parseExpression(PREFIX_UNARY_RBP);
    }

    return _ast->addCall(fKind, {arg}, p); 
}

NodeID Parser::parseMultiArgFunction() {
    const Token& t = advance();
    FunctionKind fKind = MULTI_ARG_FUNCTION_KIND_MAP.at(t.lexeme);
    size_t p = t.pos;

    expect(TokenType::LParenthesis);
    std::vector<NodeID> args = parseArgList();
    expect(TokenType::RParenthesis);

    return _ast->addCall(fKind, args, p);
}

NodeID Parser::parseOperatorName() {
    size_t p = advance().pos;

    expect(TokenType::LBrace);
    const Token& name = expect(TokenType::Identifier);
    expect(TokenType::RBrace);

    auto it = OPERATOR_NAME_MAP.find(name.lexeme);
    if (it == OPERATOR_NAME_MAP.end()) {

        std::string msg = "Unknown operatorname: \"" + name.lexeme + "\"";
        throw ParserError(_pos, msg);
    }

    expect(TokenType::LParenthesis);
    std::vector<NodeID> args = parseArgList();
    expect(TokenType::RParenthesis);

    return _ast->addCall(it->second, args, p);
}

std::vector<NodeID> Parser::parseArgList() {
    std::vector<NodeID> args;
    args.push_back(parseExpression(0));
    while (peek().is(TokenType::Comma)) {
        advance();
        args.push_back(parseExpression(0));
    }

    return args;
}

bool Parser::canImplicitMultiply() const {
    const Token& t = peek();
    if (t.is(TokenType::Number)) return true;
    if (t.is(TokenType::Identifier)) return t.lexeme != "!";
    if (t.is(TokenType::LParenthesis)) return true;
    if (t.is(TokenType::LBrace)) return true;
    if (t.is(TokenType::Command)) return PREFIX_COMMANDS.count(t.lexeme) > 0;
    return false;
}