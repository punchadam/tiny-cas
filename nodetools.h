#ifndef NODETOOLS_H
#define NODETOOLS_H

#include "ast.h"
#include "Error.h"
#include <set>
#include <algorithm>

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

inline bool isLeafNode(const AST& ast, const NodeID& id) {
    return isNumeric(ast, id) || isConstant(ast, id) || isIdentifier(ast, id);
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
    if (auto r = getRational(ast, inner)) {
        return ast.addRational(-r->numerator, r->denominator);
    }
    return ast.addBinaryOp(BinaryOpKind::Multiply, ast.addRational(-1, 1), inner);
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

// compares 2 subtrees for structural identity, for like-term grouping
inline bool structurallyEqual(const AST& a, const NodeID& idA, const AST& b, const NodeID& idB) {
    if (idA.isNone() && idB.isNone()) return true;
    if (idA.isNone() || idB.isNone()) return false;

    const ASTNode::Kind& kindA = a.at(idA).kind;
    const ASTNode::Kind& kindB = b.at(idB).kind;

    if (kindA.index() != kindB.index()) return false;

    return std::visit([&](const auto& nodeA) -> bool {
        using T = std::decay_t<decltype(nodeA)>;

        const T& nodeB = std::get<T>(kindB);

        if constexpr (std::is_same_v<T, ConstantNode>) {
            return nodeA.cKind == nodeB.cKind;
        }
        if constexpr (std::is_same_v<T, RealNode>) {
            return nodeA.value == nodeB.value;
        }
        if constexpr (std::is_same_v<T, RationalNode>) {
            return nodeA.numerator == nodeB.numerator && nodeA.denominator == nodeB.denominator;
        }
        if constexpr (std::is_same_v<T, IdentifierNode>) {
            return nodeA.name == nodeB.name;
        }

        if constexpr (std::is_same_v<T, BinaryOpNode>) {
            return nodeA.bKind == nodeB.bKind && structurallyEqual(a, nodeA.left, b, nodeB.left) && structurallyEqual(a, nodeA.right, b, nodeB.right);
        }
        if constexpr (std::is_same_v<T, UnaryOpNode>) {
            return nodeA.uKind == nodeB.uKind && structurallyEqual(a, nodeA.inner, b, nodeB.inner);
        }
        if constexpr (std::is_same_v<T, CallNode>) {
            if (nodeA.fKind != nodeB.fKind) return false;
            if (nodeA.args.size() != nodeB.args.size()) return false;
            for (size_t i = 0; i < nodeA.args.size(); i++) {
                if (!structurallyEqual(a, nodeA.args[i], b, nodeB.args[i])) return false;
            }
            return true;
        }

        return false;
    }, kindA);
}

inline bool structurallyEqual(const AST& ast, const NodeID& idA, const NodeID& idB) {
    return structurallyEqual(ast, idA, ast, idB);
}

// walks a chain of add nodes and turns all coefficients into a flat vector of terms
inline void flattenSum(const AST& ast, const NodeID& id, std::vector<NodeID>& terms) {
    if (id.isNone()) return;

    if (auto b = getBinaryOp(ast, id)) {
        if (b->bKind == BinaryOpKind::Add) {
            flattenSum(ast, b->left, terms);
            flattenSum(ast, b->right, terms);
            return;
        }
    }

    terms.push_back(id);
}

inline std::vector<NodeID> flattenSum(const AST& ast, const NodeID& id) {
    std::vector<NodeID> terms;
    flattenSum(ast, id, terms);
    return terms;
}

inline std::vector<NodeID> flattenProduct(const AST& ast, const NodeID& id, std::vector<NodeID> factors) {
    if (id.isNone()) return;

    if (auto b = getBinaryOp(ast, id)) {
        if (b->bKind == BinaryOpKind::Multiply) {
            flattenProduct(ast, b->left, factors);
            flattenProduct(ast, b->right, factors);
            return;
        }
    }
    factors.push_back(id);
}

#pragma region ORDERING
// rank for top-level node type, used for canonical ordering
inline i8 nodeTypeRank(const AST& ast, const NodeID& id) {
    if (id.isNone()) return -1;
    return std::visit([](const auto& node) -> u8 {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, RationalNode>) return 0;
        if constexpr (std::is_same_v<T, RealNode>) return 1;
        if constexpr (std::is_same_v<T, ConstantNode>) return 2;
        if constexpr (std::is_same_v<T, IdentifierNode>) return 3;
        if constexpr (std::is_same_v<T, UnaryOpNode>) return 4;
        if constexpr (std::is_same_v<T, CallNode>) return 5;
        if constexpr (std::is_same_v<T, BinaryOpNode>) return 6;
        return 7;
    }, ast.at(id).kind);
}

inline i8 compareNodes(const AST& ast, const NodeID& a, const NodeID& b) {
    if (a.isNone() && b.isNone()) return 0;
    if (a.isNone()) return -1;
    if (b.isNone()) return 1;

    i8 rankA = nodeTypeRank(ast, a);
    i8 rankB = nodeTypeRank(ast, b);
    if (rankA != rankB) return rankA - rankB;

    return std::visit([&](const auto& nodeA) -> i8 {
        using T = std::decay_t<decltype(nodeA)>;
        const T& nodeB = std::get<T>(ast.at(b).kind);

        if constexpr (std::is_same_v<T, RationalNode>) {
            double aVal = (double)nodeA.numerator / (double)nodeA.denominator;
            double bVal = (double)nodeB.numerator / (double)nodeB.denominator;

            if (aVal < bVal) return -1;
            if (aVal > bVal) return 1;
            return 0;
        }
        if constexpr (std::is_same_v<T, RealNode>) {
            if (nodeA.value < nodeB.value) return -1;
            if (nodeA.value > nodeB.value) return 1;
            return 0;
        }
        if constexpr (std::is_same_v<T, ConstantNode>) {
            return static_cast<i8>(nodeA.cKind) - static_cast<i8>(nodeB.cKind);
        }
        if constexpr (std::is_same_v<T, CallNode>) {
            i8 k = static_cast<i8>(nodeA.fKind) - static_cast<i8>(nodeB.fKind);
            if (k != 0) return k;
            size_t minArgs = std::min(nodeA.args.size(), nodeB.args.size());
            for (size_t i = 0; i < minArgs; i++) {
                i8 c = compareNodes(ast, nodeA.args[i], nodeB.args[i]);
                if (c != 0) return c;
            }
            if (nodeA.args.size() < nodeB.args.size()) return -1;
            if (nodeA.args.size() > nodeB.args.size()) return 1;
            return 0; 
        }
        if constexpr (std::is_same_v<T, BinaryOpNode>) {
            i8 k = static_cast<i8>(nodeA.bKind) - static_cast<i8>(nodeB.bKind);
            if (k != 0) return k;
            i8 compareLeft = compareNodes(ast, nodeA.left, nodeB.left);
            if (compareLeft != 0) return compareLeft;
            return compareNodes(ast, nodeA.right, nodeB.right);
        }
        return 0;
    }, ast.at(a).kind);
}

inline bool nodeLessThan(const AST& ast, const NodeID& a, const NodeID& b) {
    return compareNodes(ast, a, b) < 0;
}

#pragma endregion ORDERING

#endif