#ifndef NODETOOLS_H
#define NODETOOLS_H

#include "ast.h"
#include "Error.h"
#include <set>

#pragma region IS_TYPE_METHODS
inline bool isConstant(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<ConstantNode>(k)) return true;
    return false;
}

inline bool isReal(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<RealNode>(k)) return true;
    return false;
}

inline bool isRational(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<RationalNode>(k)) return true;
    return false;
}

inline bool isNumeric(const AST& ast, const NodeID& id) {
    return isReal(ast, id) || isRational(ast, id);
}

inline bool isIdentifier(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<IdentifierNode>(k)) return true;
    return false;
}

inline bool isBinaryOp(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<BinaryOpNode>(k)) return true;
    return false;
}

inline bool isUnaryOp(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<UnaryOpNode>(k)) return true;
    return false;
}

inline bool isCall(const AST& ast, const NodeID& id) {
    if (id.isNone()) return false;
    const ASTNode::Kind& k = ast.at(id).kind;
    if (std::holds_alternative<CallNode>(k)) return true;
    return false;
}
#pragma endregion IS_METHODS

#pragma region GET_METHODS
inline const std::optional<double> getReal(const AST& ast, const NodeID& id) {
    if (!isReal(ast, id)) return std::nullopt;
    return std::get<RealNode>(ast.at(id).kind).value;
}

inline std::optional<RationalNode> getRational(const AST& ast, const NodeID& id) {
    if (!isRational(ast, id)) return std::nullopt;
    return std::get<RationalNode>(ast.at(id).kind);
}

inline std::optional<ConstantNode> getConstant(const AST& ast, const NodeID& id) {
    if (!isConstant(ast, id)) return std::nullopt;
    return std::get<ConstantNode>(ast.at(id).kind);
}

inline std::optional<std::string> getIdentifierName(const AST& ast, const NodeID& id) {
    if (!isIdentifier(ast, id)) return std::nullopt;
    return std::get<IdentifierNode>(ast.at(id).kind).name;
}

inline std::optional<BinaryOpNode> getBinaryOp(const AST& ast, const NodeID& id) {
    if (!isBinaryOp(ast, id)) return std::nullopt;
    return std::get<BinaryOpNode>(ast.at(id).kind);
}

inline std::optional<UnaryOpNode> getUnaryOp(const AST& ast, const NodeID& id) {
    if (!isUnaryOp(ast, id)) return std::nullopt;
    return std::get<UnaryOpNode>(ast.at(id).kind);
}

inline std::optional<CallNode> getCall(const AST& ast, const NodeID& id) {
    if (!isCall(ast, id)) return std::nullopt;
    return std::get<CallNode>(ast.at(id).kind);
}
#pragma endregion GET_METHODS

#pragma region NUMBER_METHODS
inline const bool isZero(const AST& ast, const NodeID& id) {
    if (auto r = getReal(ast, id)) return *r == 0.0;
    if (auto r = getRational(ast, id)) return r->numerator == 0;
    return false;
}
inline bool isZero(const RationalNode& r) {
    return r.numerator == 0;
}

inline const bool isOne(const AST& ast, const NodeID& id) {
    if (auto r = getReal(ast, id)) return *r == 1.0;
    if (auto r = getRational(ast, id)) return r->numerator == r->denominator;
    return false;
}
inline bool isOne(const RationalNode& r) {
    return r.numerator == r.denominator;
}

inline const bool isNegativeOne(const AST& ast, const NodeID& id) {
    if (auto r = getReal(ast, id)) return *r == -1.0;
    if (auto r = getRational(ast, id)) return -r->numerator == r->denominator;
    return false;
}
inline bool isNegativeOne(const RationalNode& r) {
    return -r.numerator == r.denominator;
}

inline const bool isPositive(const AST& ast, const NodeID& id) {
    if (auto r = getReal(ast, id)) return *r > 0.0;
    if (auto r = getRational(ast, id)) return (r->numerator > 0) == (r->denominator > 0);
    return false;
}

inline const bool isNegative(const AST& ast, const NodeID& id) {
    if (auto r = getReal(ast, id)) return *r < 0.0;
    if (auto r = getRational(ast, id)) return (r->numerator > 0) != (r->denominator > 0);
    return false;
}

inline const std::optional<double> toDouble(const AST& ast, const NodeID& id) {
    if (auto r = getReal(ast, id)) return *r;
    if (auto r = getRational(ast, id)) return (double)r->numerator / (double)r->denominator;
    return std::nullopt;
}
inline const std::optional<double> toDouble(const ConstantNode& c) {
    if (c.cKind == ConstantKind::E) return 2.718281828459045;
    if (c.cKind == ConstantKind::PI) return 3.141592653589793;
    return std::nullopt;
}
inline double toDouble(const RationalNode& r) {
    return (double)r.numerator / (double)r.denominator;
}
#pragma endregion NUMBER_METHODS

#pragma region IDENTIFIER_METHODS
inline bool containsIdentifier(const AST& ast, const NodeID& id, const std::string& name) {
    if (id.isNone()) return false;

    if (auto n = getIdentifierName(ast, id)) return *n == name;
    if (auto b = getBinaryOp(ast, id)) return containsIdentifier(ast, b->right, name) || containsIdentifier(ast, b->left, name);
    if (auto u = getUnaryOp(ast, id)) return containsIdentifier(ast, u->inner, name);
    if (auto c = getCall(ast, id)) {
        for (const NodeID& arg : c->args) {
            if (containsIdentifier(ast, arg, name)) return true;
        }
    }

    return false;
}

inline std::set<std::string> collectIdentifiers(const AST& ast, const NodeID& id) {
    if (id.isNone()) return {};

    if (auto n = getIdentifierName(ast, id)) return { *n };

    if (auto b = getBinaryOp(ast, id)) {
        auto left  = collectIdentifiers(ast, b->left);
        auto right = collectIdentifiers(ast, b->right);
        left.insert(right.begin(), right.end());
        return left;
    }
    if (auto u = getUnaryOp(ast, id)) return collectIdentifiers(ast, u->inner);
    if (auto c = getCall(ast, id)) {
        std::set<std::string> result;
        for (const NodeID& arg : c->args) {
            auto argSet = collectIdentifiers(ast, arg);
            result.insert(argSet.begin(), argSet.end());
        }
        return result;
    }
    return {};
}
#pragma endregion IDENTIFIER_METHODS

#pragma region BUILDERS
inline NodeID makeSqrt(AST& ast, const NodeID& inner) {
    return ast.addBinaryOp(BinaryOpKind::Power, inner, ast.addRational(1, 2));
}

inline NodeID makeNeg(AST& ast, const NodeID& inner) {
    return ast.addUnaryOp(UnaryOpKind::Negate, inner);
}

inline NodeID makeReciprocal(AST& ast, const NodeID& inner) {
    return ast.addBinaryOp(BinaryOpKind::Divide, ast.addRational(1, 1), inner);
}

inline NodeID makePiMultiple(AST& ast, i64 num, i64 den) {
    if (num == 0) return ast.addRational(0, 1);
    NodeID pi = ast.addConstant(ConstantKind::PI);
    if (num == 1 && den == 1) return pi;
    return ast.addBinaryOp(BinaryOpKind::Multiply, ast.addRational(num, den), pi);
}

inline NodeID makeProduct(AST& ast, const NodeID& a, const NodeID& b) {
    return ast.addBinaryOp(BinaryOpKind::Multiply, a, b);
}

inline NodeID makeSum(AST& ast, const NodeID& a, const NodeID& b) {
    return ast.addBinaryOp(BinaryOpKind::Add, a, b);
}

inline NodeID makeQuotient(AST& ast, const NodeID& a, const NodeID& b) {
    return ast.addBinaryOp(BinaryOpKind::Divide, a, b);
}

inline NodeID makePower(AST& ast, const NodeID& base, const NodeID& exp) {
    return ast.addBinaryOp(BinaryOpKind::Power, base, exp);
}

inline NodeID cloneSubtree(const AST& in, const NodeID& id, AST& out) {
    if (id.isNone()) return NodeID::None();

    return std::visit([&](const auto& node) -> NodeID {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ConstantNode>) {
            return out.addConstant(node.cKind, node.pos);
        } if constexpr (std::is_same_v<T, RealNode>) {
            return out.addReal(node.value, node.pos);
        } if constexpr (std::is_same_v<T, RationalNode>) {
            return out.addRational(node.numerator, node.denominator, node.pos);
        } if constexpr (std::is_same_v<T, IdentifierNode>) {
            return out.addIdentifier(node.name, node.pos);
        } if constexpr (std::is_same_v<T, BinaryOpNode>) {
            return out.addBinaryOp(node.bKind, cloneSubtree(in, node.left, out), cloneSubtree(in, node.right, out), node.pos);
        } if constexpr (std::is_same_v<T, UnaryOpNode>) {
            return out.addUnaryOp(node.uKind, cloneSubtree(in, node.inner, out), node.pos);
        } if constexpr (std::is_same_v<T, CallNode>) {
            std::vector<NodeID> args;
            args.reserve(node.args.size());
            for (const NodeID& arg : node.args) {
                args.push_back(cloneSubtree(in, arg, out));
            }
            return out.addCall(node.fKind, args, node.pos);
        }
    }, in.at(id).kind);
}
#pragma endregion BUILDERS

#pragma region COEFFICIENT_EXTRACTION

struct CoefficientPair {
    RationalNode coefficient;
    NodeID remainder; // None if the expression is purely rational
};

inline std::optional<CoefficientPair> extractCoefficient(const AST& ast, const NodeID& id) {
    if (isRational(ast, id)) {
        return CoefficientPair{*getRational(ast, id), NodeID::None()};
    }
    if (isIdentifier(ast, id) || isConstant(ast, id)) {
        return CoefficientPair{{1, 1}, id};
    }

    if (auto b = getBinaryOp(ast, id)) {
        // rational * expression
        if (b->bKind == BinaryOpKind::Multiply) {
            if (isRational(ast, b->left)) {
                auto inner = extractCoefficient(ast, b->right);
                if (inner) {
                    auto l = *getRational(ast, b->left);
                    return CoefficientPair{
                        {l.numerator * inner->coefficient.numerator,
                         l.denominator * inner->coefficient.denominator},
                        inner->remainder
                    };
                }
                return CoefficientPair{*getRational(ast, b->left), b->right};
            }
            // expression * rational
            if (isRational(ast, b->right)) {
                auto inner = extractCoefficient(ast, b->left);
                if (inner) {
                    auto r = *getRational(ast, b->right);
                    return CoefficientPair{
                        {r.numerator * inner->coefficient.numerator,
                         r.denominator * inner->coefficient.denominator},
                        inner->remainder
                    };
                }
                return CoefficientPair{*getRational(ast, b->right), b->left};
            }
        }

        // expression / rational
        if (b->bKind == BinaryOpKind::Divide && isRational(ast, b->right)) {
            auto inner = extractCoefficient(ast, b->left);
            auto r = *getRational(ast, b->right);
            if (inner) {
                return CoefficientPair{
                    {inner->coefficient.numerator * r.denominator,
                     inner->coefficient.denominator * r.numerator},
                    inner->remainder
                };
            }
            return CoefficientPair{{r.denominator, r.numerator}, b->left};
        }
    }

    // pull out -1
    if (auto u = getUnaryOp(ast, id)) {
        if (u->uKind == UnaryOpKind::Negate) {
            auto inner = extractCoefficient(ast, u->inner);
            if (inner) {
                return CoefficientPair{
                    {-inner->coefficient.numerator, inner->coefficient.denominator},
                    inner->remainder
                };
            }
            return CoefficientPair{{-1, 1}, u->inner};
        }
    }

    return std::nullopt;
}

// check if an expression is a rational multiple of a specific constant
inline std::optional<RationalNode> extractConstantCoefficient(const AST& ast, const NodeID& id, ConstantKind kind) {
    auto pair = extractCoefficient(ast, id);
    if (!pair) return std::nullopt;

    // pure zero
    if (pair->remainder.isNone()) {
        if (pair->coefficient.numerator == 0) return RationalNode{0, 1};
        return std::nullopt;
    }

    if (auto c = getConstant(ast, pair->remainder)) {
        if (c->cKind == kind) return pair->coefficient;
    }

    return std::nullopt;
}

inline std::optional<RationalNode> extractPiCoefficient(const AST& ast, const NodeID& id) {
    return extractConstantCoefficient(ast, id, ConstantKind::PI);
}

inline std::optional<RationalNode> extractECoefficient(const AST& ast, const NodeID& id) {
    return extractConstantCoefficient(ast, id, ConstantKind::E);
}

#pragma endregion COEFFICIENT_EXTRACTION

#endif