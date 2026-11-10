// an Abstract Syntax Tree is a tree data structure storing all parts of an expression
// the parser builds the tree to encode order of operations in its structure, such that
// a postorder traversal of the tree yields the expression in reverse polish notation

// for example, A + B * C becomes the following AST:

//          +
//         / \
//        A   *
//           / \
//          B   C

// a post order traversal of this tree would look like: A -> B -> C -> * -> +
// which is RPN "ABC*+" which is equivalent to the original expression

#pragma once

#include "lexer.h"

#include <memory>

class ASTnode {
public:
    virtual ~ASTnode() = default;

    virtual std::string& toString() const = 0;
};

class RealNode : public ASTnode {
private:
    double value;
public:
    explicit RealNode(double val) : value(val) {}

    std::string& toString() const override {
        return std::to_string(value);
    }
};

class RationalNode : public ASTnode {
private:
    i64 num;
    i64 den;
public:
    explicit RationalNode(i64 n, i64 d) : num(n), den(d) {}

    std::string& toString() const override {
        return std::to_string(num) + "/" + std::to_string(den);
    }
};

enum class BinaryOpType {
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    POWER
};

class BinaryOpNode : public ASTnode {
private:
    BinaryOpType op;
    std::unique_ptr<ASTnode> left;
    std::unique_ptr<ASTnode> right;

public:
    BinaryOpNode(BinaryOpType operation, std::unique_ptr<ASTnode> l, std::unique_ptr<ASTnode> r)
        : op(operation), left(std::move(l)), right(std::move(r)) {}

    std::string& toString() const override {
        std::string str_op;
        switch (op) {
            case BinaryOpType::ADD:
                str_op = " + ";
                break;
            case BinaryOpType::SUBTRACT:
                str_op = " - ";
                break;
            case BinaryOpType::MULTIPLY:
                str_op = " * ";
                break;
            case BinaryOpType::DIVIDE:
                str_op = " / ";
                break;
            case BinaryOpType::POWER:
                str_op = " ^ ";
                break;
        }
        return "(" + left->toString() + str_op + right->toString() + ")";
    }
    
};

enum class UnaryOpType {
    NEGATE,
    FACTORIAL
};

class UnaryOpNode : public ASTnode {
private:
    UnaryOpType op;
    std::unique_ptr<ASTnode> inner;

public:
    UnaryOpNode(UnaryOpType operation, std::unique_ptr<ASTnode> in)
        : op(operation), inner(std::move(in)) {}

    std::string& toString() const override {
        if (op == UnaryOpType::NEGATE) {
            return "-" + inner->toString();
        } else if (op == UnaryOpType::FACTORIAL) {
            return inner->toString() + "!";
        } else throw std::runtime_error("Tried to print unknown unary op");
    }
};

enum class FunctionType {
    SIN, COS, TAN, CSC, SEC, COT,
    ARCSIN, ARCCOS, ARCTAN, ARCCSC, ARCSEC, ARCCOT,
    LOG, LN, EXP,
    SQRT,
    ABS,
    DERIVATIVE, ANTIDERIVATIVE,
    LIMIT,
    INTERVAL
};

class FunctionNode : public ASTnode {
private:
    FunctionType function;
    // vector of args for functions with multiple args like log
    std::vector<std::unique_ptr<ASTnode>> args;

public:
    FunctionNode(FunctionType f, std::vector<std::unique_ptr<ASTnode>> a)
        : function(f), args(std::move(a)) {}

    std::string& toString() {
        std::string str_f;
        switch (function) {
            case FunctionType::SIN:
                str_f = "sin";
                break;
            case FunctionType::COS:
                str_f = "cos";
                break;
            case FunctionType::TAN:
                str_f = "tan";
                break;
            case FunctionType::CSC:
                str_f = "csc";
                break;
            case FunctionType::SEC:
                str_f = "sec";
                break;
            case FunctionType::COT:
                str_f = "cot";
                break;
            case FunctionType::ARCSIN:
                str_f = "arcsin";
                break;
            case FunctionType::ARCCOS:
                str_f = "arccos";
                break;
            case FunctionType::ARCTAN:
                str_f = "arctan";
                break;
            case FunctionType::ARCCSC:
                str_f = "arccsc";
                break;
            case FunctionType::ARCSEC:
                str_f = "arcsec";
                break;
            case FunctionType::ARCCOT:
                str_f = "arccot";
                break;
            case FunctionType::LOG:
                str_f = "log";
                break;
            case FunctionType::LN:
                str_f = "ln";
                break;
            case FunctionType::EXP:
                str_f = "e^";
                break;
            case FunctionType::SQRT:
                str_f = "sqrt";
                break;
            case FunctionType::ABS:
                str_f = "abs";
                break;
            case FunctionType::DERIVATIVE:
                str_f = "derivative";
                break;
            case FunctionType::ANTIDERIVATIVE:
                str_f = "antiderivative";
                break;
            case FunctionType::LIMIT:
                str_f = "lim";
                break;
            case FunctionType::INTERVAL:
                str_f = "interval";
                break;
        }
        // print args inside parentheses
        str_f += "(";
        for (size_t i = 0; i < args.size();) {
            str_f += args[i]->toString();
            if (++i < args.size()) {
                str_f += ", ";
            }
        }
        str_f += ")";
        
        return str_f;
    }
};