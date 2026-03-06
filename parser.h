/*
Pratt Parsing

    pls read https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html

In short, we need to take a linear token stream
and turn it into an AST (Abstract Syntax TREE).
This requires a technique called recursive descent.

    Recursivce Descent Parsing: The parser starts with the grammar's
    start symbol (the root of the parse tree) and tries to build the
    parse tree downwards to match the input tokens.

Pratt parsing combines all rules into one function,
called parseExpression(minBP), that uses
binding power to decide when to stop consuming tokens.

    Binding power: every operator has L and R binding power, which
    encodes both its precedence in order of operations AND its
    associativity (L or R). Consider "1 + 2 * 3". The '+' has a 
    lower binding power than '*', so when parsing the right side
    of '+', encountering '*' steals that 2 because it binds tighter.

Using asymmetric bindings means L and R associativity
is automatically encoded. When two ops are next to each
other, the side with the lower number loses the tug of
war, so the parse tree groups that direction.

    "Not only does it send your mind into Möbeus-shaped hamster wheel,
    it also handles associativity and precedence!"  - matklad

--
There are 3 types of token in Pratt parsing:

    1. Prefix (some call it "nud" for null denotation)
        Tokens that start an expression (numbers, identifiers, '(',
        unary '-', functions like "sin(", etc).
    2. Infix (aka "led" for left denotation)
        Tokens that appear between expressions ('+', '*', '^', etc).
    3. Postfix
        Tokens that appear after an expression ('!', '%').

The core loop:

    parseExpression(minBP) :
        left_side = parsePrefix()     // consume one prefix thing
        loop:
            op = peek next token
            if op is postfix anmd its left_bp >= minBP:
                consume it
                wrap left_side
                continue
            if op is infix and its left_bp >= minBP:
                consume it
                right_side = parseExpression(right_bp)  // recurse
                left_side = makeBinaryNode(op, left_side, right_side)
                continue
            break
        return left_side

Recursion with right_bp is what creates the tree structure.
It determines how much of the remaining tokens the right
side can consume.
*/

#ifndef PARSER_H
#define PARSER_H

#include "AST.h"
#include "lexer.h"

#include <unordered_map>
#include <unordered_set>

class Parser {
    public:
        void parse(const std::vector<Token>& tokens, AST& ast);

    private:
        // object state
        const std::vector<Token>* _tokens = nullptr;
        AST* _ast = nullptr;
        size_t _pos = 0;

        // returns a reference to the token at pos
        const Token& peek() const;
        // returns a reference to the current token, then moves forward
        const Token& advance();
        // asserts the current token matches, then advances past it
        const Token& expect(const TokenType& type);

        NodeID parseExpression(const u8& minBP);

        NodeID parsePrefix();
        NodeID parseNumber();               // rational or real
        NodeID parseCommand();              // \sqrt{}, \pi
        NodeID parseBraceGroup();           // {...}
        NodeID parseLeftRight();            // \left(expr \right)
        NodeID parseFraction();             // \frac{n}{d}
        //NodeID parseFunctionArg();          // \sin{x}, \sin x, or \sin(x + 1)
        NodeID parseSingleArgFunction();    // sin(), ln()
        NodeID parseMultiArgFunction();     // max(), min(), atan2(), etc.
        NodeID parseOperatorName();         // \operatorname{name}(args...)
        std::vector<NodeID> parseArgList(); // \max{arg1, arg2, arg3, etc}
        
        bool canImplicitMultiply() const;
};

struct InfixInfo {
    u8 leftBP;
    u8 rightBP;
    BinaryOpKind opKind;
};

// infix ops
inline const std::unordered_map<TokenType, InfixInfo> INFIX_OPS = {
    { TokenType::Equals,    { 1, 2, BinaryOpKind::Equals } },
    { TokenType::Plus,      { 3, 4, BinaryOpKind::Add } },
    { TokenType::Minus,     { 3, 4, BinaryOpKind::Subtract } },
    { TokenType::Star,      { 5, 6, BinaryOpKind::Multiply } },
    { TokenType::Slash,     { 5, 6, BinaryOpKind::Divide } },
    { TokenType::Caret,     { 12, 11, BinaryOpKind::Power } }
};

// commands that act as infix ops
inline const std::unordered_map<std::string, InfixInfo> INFIX_COMMAND_OPS = {
    { "cdot",   { 5, 6, BinaryOpKind::Multiply } },
    { "times",  { 5, 6, BinaryOpKind::Multiply } },
    { "div",    { 5, 6, BinaryOpKind::Divide } }
};

// prefix unary operators only need a right binding power
inline constexpr u8 PREFIX_UNARY_RBP = 9;

// postfix operators only need a left binding power, which binds tightest
inline constexpr u8 POSTFIX_LBP = 13;

// maps commands to FunctionKind for single-arg functions
inline const std::unordered_map<std::string, FunctionKind> FUNCTION_KIND_MAP = {
    { "sin",    FunctionKind::Sine },
    { "cos",    FunctionKind::Cosine },
    { "tan",    FunctionKind::Tangent },
    { "ln",     FunctionKind::NaturalLogarithm },
    { "log",    FunctionKind::Logarithm },
    { "exp",    FunctionKind::Exponential }
};

// maps multi-arg functions \operatorname{name} tp FunctionKind
inline const std::unordered_map<std::string, FunctionKind> OPERATOR_NAME_MAP = {
    { "max",    FunctionKind::Max },
    { "min",    FunctionKind::Min },
    { "atan2",  FunctionKind::Atan2 },
    { "hypot",  FunctionKind::Hypotenuse },
    { "abs",    FunctionKind::AbsoluteValue }
};

// maps commands to FunctionKind for multi-arg functions (e.g. \max, \min)
inline const std::unordered_map<std::string, FunctionKind> MULTI_ARG_FUNCTION_KIND_MAP = {
    { "max",    FunctionKind::Max },
    { "min",    FunctionKind::Min },
    { "atan2",  FunctionKind::Atan2 },
    { "hypot",  FunctionKind::Hypotenuse },
    { "abs",    FunctionKind::AbsoluteValue }
};

// maps commands to ConstantKind
inline const std::unordered_map<std::string, ConstantKind> CONSTANT_MAP = {
    { "pi",     ConstantKind::PI },
    { "e",      ConstantKind::E }
};

// commands that can start a new expression for implicit multiplication
inline const std::unordered_set<std::string> PREFIX_COMMANDS = {
    "sin", "cos", "tan",
    "ln", "log", "exp",
    "pi", "e",
    "sqrt", "frac",
    "left", "operatorname",
    "arcsin", "arccos", "arctan",
    "max", "min", "atan2", "hypot", "abs",
};

#endif