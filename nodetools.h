#ifndef NODETOOLS_H
#define NODETOOLS_H

#include "ast.h"
#include "Error.h"
#include <set>

bool isConstant(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<ConstantNode>(k)) return true;
    return false;
}

bool isReal(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<RealNode>(k)) return true;
    return false;
}

bool isRational(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<RationalNode>(k)) return true;
    return false;
}

bool isNumeric(const AST& ast, const NodeID& id) {
    return isReal(ast, id) || isRational(ast, id);
}

bool isIdentifier(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<IdentifierNode>(k)) return true;
    return false;
}

bool isBinaryOp(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<BinaryOpNode>(k)) return true;
    return false;
}

bool isUnaryOp(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<UnaryOpNode>(k)) return true;
    return false;
}

bool isCall(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<CallNode>(k)) return true;
    return false;
}

const std::optional<double> getReal(const AST& ast, const NodeID& id) {
    if (!isReal(ast, id)) return std::nullopt;
    return std::get<RealNode>(ast.at(id).kind).value;
}

std::optional<RationalNode> getRational(const AST& ast, const NodeID& id) {
    if (!isRational(ast, id)) return std::nullopt;
    return std::get<RationalNode>(ast.at(id).kind);
}

std::optional<std::string> getIdentifierName(const AST& ast, const NodeID& id) {
    if (!isIdentifier(ast, id)) return std::nullopt;
    return std::get<IdentifierNode>(ast.at(id).kind).name;
}

std::optional<BinaryOpNode> getBinaryOp(const AST& ast, const NodeID& id) {
    if (!isBinaryOp(ast, id)) return std::nullopt;
    return std::get<BinaryOpNode>(ast.at(id).kind);
}

std::optional<UnaryOpNode> getUnaryOp(const AST& ast, const NodeID& id) {
    if (!isUnaryOp(ast, id)) return std::nullopt;
    return std::get<UnaryOpNode>(ast.at(id).kind);
}

std::optional<CallNode> getCall(const AST& ast, const NodeID& id) {
    if (!isCall(ast, id)) return std::nullopt;
    return std::get<CallNode>(ast.at(id).kind);
}

const bool isZero(const AST& ast, const NodeID& id) {
    if (auto r = getReal(ast, id)) return *r == 0.0;
    if (auto r = getRational(ast, id)) return r->numerator == 0;
    return false;
}

const bool isOne(const AST& ast, const NodeID& id) {
    if (auto r = getReal(ast, id)) return *r == 1.0;
    if (auto r = getRational(ast, id)) return r->numerator == r->denominator;
    return false;
}

const bool isNegativeOne(const AST& ast, const NodeID& id) {
    if (auto r = getReal(ast, id)) return *r == -1.0;
    if (auto r = getRational(ast, id)) return -r->numerator == r->denominator;
    return false;
}

const std::optional<double> toDouble(const AST& ast, const NodeID& id) {
    if (auto r = getReal(ast, id)) return *r;
    if (auto r = getRational(ast, id)) return (double)r->numerator / (double)r->denominator;
    return std::nullopt;
}

bool containsIdentifier(const AST& ast, const NodeID& id) {
    
}

std::set<std::string> collectIdentifiers(const AST& ast, const NodeID& id) {

}

#endif