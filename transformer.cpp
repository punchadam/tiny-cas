#include "transformer.h"
#include <cmath>

NodeID transform(const AST& input, AST& output) {
    AST a, b, c, d;

    a.root = foldConstants(input, input.root, a);
    b.root = eliminateSubtraction(a, a.root, b);
    c.root = simplifyIdentities(b, b.root, c);
    d.root = applyTrigIdentities(c, c.root, d);
    output.root = canonicalizeLogExp(d, d.root, output);

    return output.root;
}

NodeID foldConstants(const AST& input, const NodeID& id, AST& output) {
    if (id.isNone()) return NodeID::None();

    if (isConstant(input, id) || isReal(input, id) || isRational(input, id) || isIdentifier(input, id)) {
        return cloneSubtree(input, id, output);
    }

    if (auto b = getBinaryOp(input, id)) {
        NodeID left = foldConstants(input, b->left, output);
        NodeID right = foldConstants(input, b->right, output);

        if (isRational(output, left) && isRational(output, right)) {
            auto l = *getRational(output, left);
            auto r = *getRational(output, right);

            switch (b->bKind) {
                case BinaryOpKind::Add: return output.addRational(l.numerator * r.denominator + r.numerator * l.denominator, l.denominator * r.denominator);
                case BinaryOpKind::Subtract:  return output.addRational(l.numerator * r.denominator - r.numerator * l.denominator, l.denominator * r.denominator);
                case BinaryOpKind::Multiply: return output.addRational(l.numerator * r.numerator, l.denominator * r.denominator);
                case BinaryOpKind::Divide: return output.addRational(l.numerator * r.denominator, l.denominator * r.numerator);
                case BinaryOpKind::Power: if (auto result = tryFoldPower(l, r, output)) return *result;
                    // fall through
                default:
                    return output.addBinaryOp(b->bKind, left, right);
            }
        }
        return output.addBinaryOp(b->bKind, left, right);
    }

    if (auto u = getUnaryOp(input, id)) {
        NodeID inner = foldConstants(input, u->inner, output);

        if (u->uKind == UnaryOpKind::Negate && isRational(output, inner)) {
            auto r = *getRational(output, inner);
            return output.addRational(-r.numerator, r.denominator);
        }

        if (u->uKind == UnaryOpKind::Factorial && isRational(output, inner)) {
            auto r = *getRational(output, inner);
            if (r.denominator == 1 && r.numerator >= 0) {
                i64 result = 1;
                for (i64 k = 2; k <= r.numerator; k++) result *= k;
                return output.addRational(result, 1);
            }
        }

        if (u->uKind == UnaryOpKind::Percent && isRational(output, inner)) {
            auto r = *getRational(output, inner);
            return output.addRational(r.numerator, r.denominator * 100);
        }

        return output.addUnaryOp(u->uKind, inner);
    }

    if (auto c = getCall(input, id)) {
        std::vector<NodeID> args;
        args.reserve(c->args.size());
        for (const NodeID& arg : c->args) {
            args.emplace_back(foldConstants(input, arg, output));
        }
        return output.addCall(c->fKind, args);
    }

    return cloneSubtree(input, id, output);
}

std::optional<NodeID> tryFoldPower( const RationalNode& base, const RationalNode& exp, AST& output) {
    RationalNode b = base;
    i64 p = exp.numerator;
    i64 q = exp.denominator;

    if (isZero(exp)) return output.addRational(1, 1);

    if (p < 0) {
        std::swap(b.numerator, b.denominator);
        p = -p;
    }
    
    // try exact qth root
    if (q != 1) {
        if (b.numerator < 0 && q % 2 == 0) return std::nullopt;

        bool negative = b.numerator < 0;
        auto numRoot = tryIntegerRoot(std::abs(b.numerator), q);
        auto denRoot = tryIntegerRoot(b.denominator, q);

        if (!numRoot || !denRoot) return std::nullopt;

        b = { negative ? -*numRoot: *numRoot, *denRoot };
    }

    // apply pth power
    if (p == 1) return output.addRational(b.numerator, b.denominator);
    i64 num = 1, den = 1;
    for (i64 k = 0; k < p; k++) {
        num *= b.numerator;
        den *= b.denominator;
    }
    return output.addRational(num, den);
}

std::optional<i64> tryIntegerRoot(i64 val, i64 n) {
    if (val < 0) return std::nullopt; // sign should've been handled by now
    if (val == 0) return 0;
    if (n == 0) return 1; // also shoulda been handled but idc fuck it whatever
    if (val == 1 || n == 1) return val;

    // newton's method
    double guess = std::pow(static_cast<double>(val), 1.0 / n);
    i64 candidate = static_cast<i64>(std::round(guess));

    // check neighbors bc floating point can be off by 1
    for (i64 c = std::max<i64>(0, candidate - 2); c <= candidate + 2; c++) { // "THIS MUST BE SOME KIND OF... C++" HOOOLLY SHIT HE SAID THE NAME OF THE LANGUAGE HOOOOOLLLLY SHIT
        i64 power = 1;
        bool overflow = false;
        for (i64 i = 0; i < n; i++) {
            if (power > std::numeric_limits<i64>::max() / std::max<i64>(c, 1)) {
                overflow = true;
                break;
            }
            power *= c;
        }
        if (!overflow && power == val) return c;
    }
    return std::nullopt;
}

NodeID eliminateSubtraction(const AST& input, const NodeID& id, AST& output) {
    if (id.isNone()) return NodeID::None();

    if (isConstant(input, id) || isReal(input, id) || isRational(input, id) || isIdentifier(input, id)) {
        return cloneSubtree(input, id, output);
    }

    if (auto b = getBinaryOp(input, id)) {
        NodeID left = eliminateSubtraction(input, b->left, output);
        NodeID right = eliminateSubtraction(input, b->right, output);

        if (b->bKind == BinaryOpKind::Subtract) {
            // a - b  ->  a + (-1 * b)
            return makeSum(output, left, makeNeg(output, right));
        }
        return output.addBinaryOp(b->bKind, left, right);
    }

    if (auto u = getUnaryOp(input, id)) {
        NodeID inner = eliminateSubtraction(input, u->inner, output);

        if (u->uKind == UnaryOpKind::Negate) {
            // unary negation -> (-1 * inner)
            return makeNeg(output, inner);
        }
        return output.addUnaryOp(u->uKind, inner);
    }

    if (auto c = getCall(input, id)) {
        std::vector<NodeID> args;
        args.reserve(c->args.size());
        for (const NodeID& arg : c->args) {
            args.emplace_back(eliminateSubtraction(input, arg, output));
        }
        return output.addCall(c->fKind, args);
    }

    return cloneSubtree(input, id, output);
}