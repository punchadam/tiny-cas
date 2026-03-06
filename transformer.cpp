#include "transformer.h"
#include <cmath>
#include <cassert>
#include <iostream>

NodeID transform(const AST& input, AST& output) {
    AST current;
    current.root = cloneSubtree(input, input.root, current);

    for (size_t iterations = 0; iterations < 64; iterations++) {
        u8 pass = 0;
        AST a, b, c, d, e, f, g, h, i, next;
        try {
            ++pass;
            a.root = eliminateNegate(current, current.root, a);
            ++pass;
            b.root = foldConstants(a, a.root, b);
            ++pass;
            c.root = eliminateSubtraction(b, b.root, c);
            ++pass;
            d.root = eliminateDivision(c, c.root, d);
            ++pass;
            e.root = simplifyIdentities(d, d.root, e);
            ++pass;
            f.root = combineLikeTerms(e, e.root, f);
            ++pass;
            g.root = collectExponents(f, f.root, g);
            ++pass;
            h.root = applyTrigIdentities(g, g.root, h);
            ++pass;
            i.root = canonicalizeLogExp(h, h.root, i);
            ++pass;
            next.root = canonicalOrder(i, i.root, next);
        } catch(const std::exception& e) {
            std::string msg = "In pass: ";
            switch (pass) {
                case 0: msg = "In initialization\n"; break;
                case 1: msg += "eliminateNegate\n"; break;
                case 2: msg += "foldConstants\n"; break;
                case 3: msg += "eliminateSubtraction\n"; break;
                case 4: msg += "eliminateDivision\n"; break;
                case 5: msg += "simplifyIdentities\n"; break;
                case 6: msg += "combineLikeTerms\n"; break;
                case 7: msg += "collectExponent\n"; break;
                case 8: msg += "applyTrigIdentities\n"; break;
                case 9: msg += "canonicalizeLogExp\n"; break;
                case 10: msg += "canonicalOrder\n"; break;
                default: msg = "Idk man you fucked up tho\n";
            }
            throw TransformerError(UnknownPos, msg);
        }
        
        if (structurallyEqual(current, current.root, next, next.root)) {
            current = std::move(next);
            break;
        }

        current = std::move(next);
        if (iterations == 63) throw TransformerError(UnknownPos, "Transform did not converge");
    }

    output.root = cloneSubtree(current, current.root, output);
    return output.root;
}

NodeID eliminateNegate(const AST& input, const NodeID& id, AST& output) {
    if (id.isNone()) return NodeID::None();

    if (auto u = getUnaryOp(input, id)) {
        if (u->uKind == UnaryOpKind::Negate) {
            NodeID inner = eliminateNegate(input, u->inner, output);
            return makeNeg(output, inner);
        }
        NodeID inner = eliminateNegate(input, u->inner, output);
        return output.addUnaryOp(u->uKind, inner);
    }

    if (auto b = getBinaryOp(input, id)) {
        NodeID left = eliminateNegate(input, b->left, output);
        NodeID right = eliminateNegate(input, b->right, output);
        return output.addBinaryOp(b->bKind, left, right);
    }

    if (auto c = getCall(input, id)) {
        std::vector<NodeID> args;
        args.reserve(c->args.size());
        for (const NodeID& arg : c->args) {
            args.emplace_back(eliminateNegate(input, arg, output));
        }
        return output.addCall(c->fKind, args);
    }

    return cloneSubtree(input, id, output);
}

NodeID foldConstants(const AST& input, const NodeID& id, AST& output) {
    if (id.isNone()) return NodeID::None();

    if (isLeafNode(input, id)) return cloneSubtree(input, id, output);

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

    if (isLeafNode(input, id)) return cloneSubtree(input, id, output);

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

NodeID eliminateDivision(const AST& input, const NodeID& id, AST& output) {
    if (id.isNone()) return NodeID::None();

    if (isLeafNode(input, id)) return cloneSubtree(input, id, output);

    if (auto b = getBinaryOp(input, id)) {
        NodeID left = eliminateDivision(input, b->left, output);
        NodeID right = eliminateDivision(input, b->right, output);

        if (b->bKind == BinaryOpKind::Divide) {
            // don't decompose rational/rational
            if (isRational(output, left) && isRational(output, right)) {
                return output.addBinaryOp(BinaryOpKind::Divide, left, right);
            }

            // a / b -> a * b^(-1)
            return makeProduct(output, left, makeReciprocal(output, right));
        }
        return output.addBinaryOp(b->bKind, left, right);
    }

    if (auto u = getUnaryOp(input, id)) {
        NodeID inner = eliminateDivision(input, u->inner, output);
        return output.addUnaryOp(u->uKind, inner);
    }

    if (auto c = getCall(input, id)) {
        std::vector<NodeID> args;
        args.reserve(c->args.size());
        for (const NodeID& arg : c->args) {
            args.emplace_back(eliminateDivision(input, arg, output));
        }
        return output.addCall(c->fKind, args);
    }

    return cloneSubtree(input, id, output);
}

NodeID simplifyIdentities(const AST& input, const NodeID & id, AST& output) {
    if (id.isNone()) return NodeID::None();

    if (isConstant(input, id) || isNumeric(input, id)) return cloneSubtree(input, id, output);

    if (auto b = getBinaryOp(input, id)) {
        NodeID left = simplifyIdentities(input, b->left, output);
        NodeID right = simplifyIdentities(input, b->right, output);

        switch (b->bKind) {
            case BinaryOpKind::Add: {
                if (isZero(output, right)) return left;
                if (isZero(output, left)) return right;
                break;
            }
            case BinaryOpKind::Multiply: {
                if (isZero(output, left) || isZero(output, right)) return output.addRational(0, 1);
                if (isOne(output, left)) return right;
                if (isOne(output, right)) return left;
                if (isNegativeOne(output, left)) return makeNeg(output, right);
                if (isNegativeOne(output, right)) return makeNeg(output, left);
                break;
            }
            /*
            case BinaryOpKind::Divide: {
                if (isZero(output, left)) return output.addRational(0, 1);
                if (isOne(output, right)) return left;
                if (isRational(output, left) && isRational(output, right)) {
                    auto l = *getRational(output, left);
                    auto r = *getRational(output, right);
                    if (!isZero(r)) {
                        return output.addRational(l.numerator * r.denominator, l.denominator * r.numerator);
                    }
                }
                break;
            }
            */
            case BinaryOpKind::Power: {
                // flatten nested powers
                if (auto innerPow = getBinaryOp(output, left)) {
                    if (innerPow->bKind == BinaryOpKind::Power) {
                        NodeID newExp = makeProduct(output, innerPow->right, right);
                        return output.addBinaryOp(BinaryOpKind::Power, innerPow->left, newExp);
                    }
                }
                if (isZero(output, right)) return output.addRational(1, 1);
                if (isOne(output, right)) return left;
                if (isZero(output, left) && isPositive(output, right)) return output.addRational(0, 1);
                if (isOne(output, left)) return output.addRational(1, 1);
                break;
            }
            default: break;
        }
        return output.addBinaryOp(b->bKind, left, right);
    }

    if (auto u = getUnaryOp(input, id)) {
        NodeID inner = simplifyIdentities(input, u->inner, output);
        return output.addUnaryOp(u->uKind, inner);
    }

    if (auto c = getCall(input, id)) {
        std::vector<NodeID> args;
        args.reserve(c->args.size());
        for (const NodeID& arg : c->args) {
            args.emplace_back(simplifyIdentities(input, arg, output));
        }
        return output.addCall(c->fKind, args);
    }
    
    return cloneSubtree(input, id, output);
}

NodeID combineLikeTerms(const AST& input, const NodeID& id, AST& output) {
    if (id.isNone()) return NodeID::None();

    if (isLeafNode(input, id)) return cloneSubtree(input, id, output);

    // recurse into binary ops
    if (auto b = getBinaryOp(input, id)) {
        NodeID left  = combineLikeTerms(input, b->left, output);
        NodeID right = combineLikeTerms(input, b->right, output);

        // only flatten Add ops
        if (b->bKind != BinaryOpKind::Add) {
            return output.addBinaryOp(b->bKind, left, right);
        }

        // build temp AST to avoid corrupting output
        AST temp;
        NodeID tempLeft = cloneSubtree(output, left, temp);
        NodeID tempRight = cloneSubtree(output, right, temp);
        NodeID tempAdd = temp.addBinaryOp(BinaryOpKind::Add, tempLeft, tempRight);
        auto terms = flattenSum(temp, tempAdd);

        // each group is a summed coefficient, remainder NodeID
        struct Group {
            i64 num;     // accumulated numerator
            i64 den;     // accumulated denominator
            NodeID remainder;
        };
        std::vector<Group> groups;

        for (const NodeID& term : terms) {
            assert(term.i < temp.arena.size() && "term not in temp!");
            auto coeff = extractCoefficient(temp, term);
            if (!coeff) {
                // can't extract coefficient, treat as 1 * term
                // check if any existing group matches this whole term
                bool merged = false;
                for (auto& group : groups) {
                    if (group.remainder.isNone()) continue;
                    if (structurallyEqual(temp, group.remainder, term)) {
                        // add 1 to this group
                        group.num += group.den; // (group.num/group.den) + 1/1
                        // group.den stays the same
                        merged = true;
                        break;
                    }
                }
                if (!merged) {
                    groups.push_back({1, 1, term});
                }
                continue;
            }

            RationalNode c = coeff->coefficient;
            NodeID rem = coeff->remainder;

            // find existing group with same remainder
            bool merged = false;
            for (auto& group : groups) {
                bool bothPure = group.remainder.isNone() && rem.isNone();
                bool bothHaveRemainder = !group.remainder.isNone() && !rem.isNone();

                if (bothPure || (bothHaveRemainder && structurallyEqual(temp, group.remainder, rem)))
                {
                    // add the coefficients: group.num/group.den + c.num/c.den
                    group.num = group.num * c.denominator + c.numerator * group.den;
                    group.den = group.den * c.denominator;
                    // reduce
                    i64 gcd = std::gcd(std::abs(group.num), std::abs(group.den));
                    if (gcd > 0) { group.num /= gcd; group.den /= gcd; }
                    merged = true;
                    break;
                }
            }
            if (!merged) {
                groups.push_back({c.numerator, c.denominator, rem});
            }
        }

        // convert each group back to a node, then fold into an Add chain
        std::vector<NodeID> rebuilt;
        for (const auto& group : groups) {
            if (group.num == 0) continue;

            if (group.remainder.isNone()) {
                rebuilt.push_back(output.addRational(group.num, group.den));
            } else if (group.num == 1 && group.den == 1) {
                rebuilt.push_back(cloneSubtree(temp, group.remainder, output));
            } else if (group.num == -1 && group.den == 1) {
                rebuilt.push_back(makeNeg(output, cloneSubtree(temp, group.remainder, output)));
            } else {
                rebuilt.push_back(makeProduct(output, output.addRational(group.num, group.den), cloneSubtree(temp, group.remainder, output)));
            }
        }

        if (rebuilt.empty()) {
            return output.addRational(0, 1);
        }

        // fold into a right-leaning add chain
        NodeID result = rebuilt.back();
        for (i16 i = (i16)rebuilt.size() - 2; i >= 0; i--) {    // iterate backwards to preserve add order
            result = makeSum(output, rebuilt[i], result);
        }
        return result;
    }

    // recurse into unary ops
    if (auto u = getUnaryOp(input, id)) {
        NodeID inner = combineLikeTerms(input, u->inner, output);
        return output.addUnaryOp(u->uKind, inner);
    }

    // recurse into calls
    if (auto c = getCall(input, id)) {
        std::vector<NodeID> args;
        args.reserve(c->args.size());
        for (const NodeID& arg : c->args) {
            args.emplace_back(combineLikeTerms(input, arg, output));
        }
        return output.addCall(c->fKind, args);
    }

    return cloneSubtree(input, id, output);
}

NodeID collectExponents(const AST& input, const NodeID& id, AST& output) {
    if (id.isNone()) return NodeID::None();

    if (isLeafNode(input, id)) return cloneSubtree(input, id, output);

    // recurse into binary ops
    if (auto b = getBinaryOp(input, id)) {
        NodeID left  = collectExponents(input, b->left, output);
        NodeID right = collectExponents(input, b->right, output);

        if (b->bKind != BinaryOpKind::Multiply) {
            return output.addBinaryOp(b->bKind, left, right);
        }

        // build temp AST to avoid corrupting output
        AST temp;
        NodeID tempLeft = cloneSubtree(output, left, temp);
        NodeID tempRight = cloneSubtree(output, right, temp);
        NodeID tempMultiply = temp.addBinaryOp(BinaryOpKind::Multiply, tempLeft, tempRight);
        auto factors = flattenProduct(temp, tempMultiply);

        // each group is a base with rational exponent
        struct Group {
            i64 num;     // exponent numerator
            i64 den;     // exponent denominator
            NodeID base;
        };
        std::vector<Group> groups;

        // separate out the numeric coefficient (keep rationals as-is)
        std::optional<RationalNode> numericCoefficient;

        for (const NodeID& factor : factors) {
            
            // accumulate rational factors separately
            if (isRational(temp, factor)) {
                auto r = *getRational(temp, factor);
                if (!numericCoefficient) {
                    numericCoefficient = r;
                } else {
                    numericCoefficient = RationalNode {
                        numericCoefficient->numerator * r.numerator,
                        numericCoefficient->denominator * r.denominator
                    };
                }
                continue;
            }

            auto exponent = extractExponent(temp, factor);
            if (!exponent) {
                groups.push_back({1, 1, factor});
                continue;
            }

            bool merged = false;
            // merge groups with the same base as the exponent
            for (auto& group : groups) {
                if (structurallyEqual(temp, group.base, exponent->base)) {
                    group.num = group.num * exponent->exponent.denominator + exponent->exponent.numerator * group.den;
                    group.den = group.den * exponent->exponent.denominator;
                    i64 gcd = std::gcd(std::abs(group.num), std::abs(group.den));
                    if (gcd > 0) { group.num /= gcd; group.den /= gcd; }
                    merged = true;
                    break;
                }
            }
            if (!merged) {
                groups.push_back({exponent->exponent.numerator, exponent->exponent.denominator, exponent->base });
            }
        }

        // rebuild
        std::vector<NodeID> rebuilt;

        if (numericCoefficient && !isOne(*numericCoefficient)) {
            rebuilt.push_back(output.addRational(numericCoefficient->numerator, numericCoefficient->denominator));
        }
        
        for (const auto& group : groups) {
            if (group.num == 0) continue; // vanish x^0 = 1

            NodeID base = cloneSubtree(temp, group.base, output);

            if (group.num == 1 && group.den == 1) {
                rebuilt.push_back(base);
            } else {
                rebuilt.push_back(makePower(output, base, output.addRational(group.num, group.den)));
            }
        }

        if (rebuilt.empty()) {
            return output.addRational(1, 1);
        }

        // fold into right-leaning multiply chain
        NodeID result = rebuilt.back();
        for (i8 i = (i8)rebuilt.size() - 2; i >= 0; i--) {
            result = makeProduct(output, rebuilt[i], result);
        }
        return result;
    }

    if (auto u = getUnaryOp(input, id)) {
        NodeID inner = collectExponents(input, u->inner, output);
        return output.addUnaryOp(u->uKind, inner);
    }
    
    if (auto c = getCall(input, id)) {
        std::vector<NodeID> args;
        args.reserve(c->args.size());
        for (const NodeID& arg : c->args) {
            args.emplace_back(collectExponents(input, arg, output));
        }
        return output.addCall(c->fKind, args);
    }
    
    return cloneSubtree(input, id, output);
}

NodeID applyTrigIdentities(const AST& input, const NodeID id, AST& output) {
    if (id.isNone()) return NodeID::None();

    if (isConstant(input, id) || isNumeric(input, id)) return cloneSubtree(input, id, output);

    if (auto b = getBinaryOp(input, id)) {
        NodeID left = applyTrigIdentities(input, b->left, output);
        NodeID right = applyTrigIdentities(input, b->right, output);
        return output.addBinaryOp(b->bKind, left, right);
    }
    if (auto u = getUnaryOp(input, id)) {
        NodeID inner = applyTrigIdentities(input, u->inner, output);
        return output.addUnaryOp(u->uKind, inner);
    }

    if (auto c = getCall(input, id)) {
        std::vector<NodeID> args;
        args.reserve(c->args.size());
        for (const NodeID& arg : c->args) {
            args.emplace_back(applyTrigIdentities(input, arg, output));
        }

        if (args.size() == 1) {
            if (auto folded = tryFoldTrig(c->fKind, output, args[0], output)) return *folded;
        }

        return output.addCall(c->fKind, args);
    }
    
    return cloneSubtree(input, id, output);
}

static RationalNode modTwoPi(RationalNode r) {
    // the function name is LIES because it's already working with the coefficient of pi
    // normalize sign
    if (r.denominator < 0) {
        r.numerator = -r.numerator; r.denominator = -r.denominator;
    }
    i64 period = 2 * r.denominator;
    r.numerator = ((r.numerator % period) + period) % period;
    return r;
}

static RationalNode cosShift(const RationalNode& k) {
    return { k.denominator - 2 * k.numerator, 2 * k.denominator };
}

static std::optional<NodeID> evaluateSinAtPiMultiple(const RationalNode& piCoefficient, AST& out) {
    RationalNode r = modTwoPi(piCoefficient);
    i64 n = r.numerator;
    i64 d = r.denominator;

    // reduce to first quadrant using sin(pi - x) = sin(x), sin(pi + x) = -sin(x)
    bool negate = false;
    // if in [pi, 2pi), subtract one factor of pi
    if (n >= d) {
        n -= d;
        negate = true;
    }
    // now n/d is in [0, 1)
    // if in (pi/2, pi), use sin(pi - x) = sin(x) -> reflect
    if (2 * n > d) {
        n = d - n;
    }
    // now n/d is in [0, 1/2]

    // check known exact values
    i64 gcd = std::gcd(std::abs(n), std::abs(d));
    i64 rn = n / gcd, rd = d / gcd;

    std::optional<NodeID> result;

    if (rn == 0) {                      // sin(0) = 0
        result = out.addRational(0, 1);
    } else if (rn == 1 && rd == 12) {   // sin(pi/12) = (sqrt(6) - sqrt(2)) / 4
        result = makeQuotient(out, makeSum(out, makeSqrt(out, out.addRational(6, 1)), makeNeg(out, makeSqrt(out, out.addRational(2, 1)))), out.addRational(4, 1));
    } else if (rn == 1 && rd == 10) {   // sin(pi/10) = (sqrt(5) - 1) / 4 <- GOLDEN RATIO!!! WOOOOOO
        result = makeQuotient(out, makeSum(out, makeSqrt(out, out.addRational(5, 1)), makeNeg(out, out.addRational(1, 1))), out.addRational(4, 1));
    } else if (rn == 1 && rd == 8) {    // sin(pi/8) = (sqrt(2 - sqrt(2))) / 2
        result = makeQuotient(out, makeSqrt(out, makeSum(out, out.addRational(2, 1), makeNeg(out, makeSqrt(out, out.addRational(2, 1))))), out.addRational(2, 1));
    } else if (rn == 1 && rd == 6) {    // sin(pi/6) = 1/2
        result = out.addRational(1, 2);
    } else if (rn == 1 && rd == 5) {    // sin(pi/5) = sqrt((5 - sqrt(5)) / 8)
        result = makeSqrt(out, makeQuotient(out, makeSum(out, out.addRational(5, 1), makeNeg(out, makeSqrt(out, out.addRational(5, 1)))), out.addRational(8, 1)));
    } else if (rn == 1 && rd == 4) {    // sin(pi/4) = sqrt(2)/2
        result = makeQuotient(out, makeSqrt(out, out.addRational(2, 1)), out.addRational(2, 1));
    } else if (rn == 1 && rd == 3) {    // sin(pi/3) = sqrt(3)/2
        result = makeQuotient(out, makeSqrt(out, out.addRational(3, 1)), out.addRational(2, 1));
    } else if (rn == 1 && rd == 2) {    // sin(pi/2) = 1
        result = out.addRational(1, 1);
    }
    
    if (!result) return std::nullopt;
    return negate ? makeNeg(out, *result) : *result;
}

std::optional<NodeID> tryFoldTrig(FunctionKind fKind, const AST& ast, const NodeID& arg, AST& out) {
    auto piCoefficient = extractPiCoefficient(ast, arg);
    if (!piCoefficient) return std::nullopt;

    if (fKind == FunctionKind::Sine) {
        return evaluateSinAtPiMultiple(*piCoefficient, out);
    }

    if (fKind == FunctionKind::Cosine) {
        return evaluateSinAtPiMultiple(cosShift(*piCoefficient), out);
    }

    if (fKind == FunctionKind::Tangent) {
        AST temp; // temporary ast to check if both are exact and solveable
        auto sinVal = evaluateSinAtPiMultiple(*piCoefficient, temp);
        auto cosVal = evaluateSinAtPiMultiple(cosShift(*piCoefficient), temp);
        if (!sinVal || !cosVal) return std::nullopt;
        if (isZero(temp, *cosVal)) return std::nullopt;
        if (isZero(temp, *sinVal)) return out.addRational(0, 1);

        auto s = evaluateSinAtPiMultiple(*piCoefficient, out);
        auto c = evaluateSinAtPiMultiple(cosShift(*piCoefficient), out);
        return makeQuotient(out, *s, *c);
    }

    return std::nullopt;
}

NodeID canonicalizeLogExp(const AST& input, const NodeID id, AST& output) {
    if (id.isNone()) return NodeID::None();

    if (auto b = getBinaryOp(input, id)) {
        NodeID left = canonicalizeLogExp(input, b->left, output);
        NodeID right = canonicalizeLogExp(input, b->right, output);

        // e ^ something
        if (b->bKind == BinaryOpKind::Power) {
            if (auto k = getConstant(output, left)) {
                if (k->cKind == ConstantKind::E) {
                    // e^0 = 1
                    if (isZero(output, right)) return output.addRational(1, 1);
                    // e^1 = e
                    if (isOne(output, right)) return output.addConstant(ConstantKind::E);
                    // e^ln(x) = x
                    if (auto call = getCall(output, right)) {
                        if (call->fKind == FunctionKind::NaturalLogarithm && call->args.size() == 1) {
                            return call->args[0];
                        }
                    }
                    // e^(n * ln(x)) = x^n
                    if (auto mul = getBinaryOp(output, right)) {
                        if (mul->bKind == BinaryOpKind::Multiply) {
                            auto tryExtract = [&](NodeID maybeCoeff, NodeID maybeLn) -> std::optional<NodeID> {
                                if (auto call = getCall(output, maybeLn)) {
                                    if (call->fKind == FunctionKind::NaturalLogarithm && call->args.size() == 1) {
                                        return makePower(output, call->args[0], maybeCoeff);
                                    }
                                }
                                return std::nullopt;
                            };
                            if (auto r = tryExtract(mul->left, mul->right)) return *r;
                            if (auto r = tryExtract(mul->right, mul->left)) return *r;
                        }
                    }
                }
            }
        }

        return output.addBinaryOp(b->bKind, left, right);
    }

    if (auto u = getUnaryOp(input, id)) {
        NodeID inner = canonicalizeLogExp(input, u->inner, output);
        return output.addUnaryOp(u->uKind, inner);
    }

    if (auto c = getCall(input, id)) {
        std::vector<NodeID> args;
        args.reserve(c->args.size());
        for (const NodeID& arg : c->args) {
            args.emplace_back(canonicalizeLogExp(input, arg, output));
        }

        if (c->fKind == FunctionKind::NaturalLogarithm && args.size() == 1) {
            NodeID arg = args[0];

            if (isOne(output, arg)) return output.addRational(0, 1);
            if (auto k = getConstant(output, arg)) {
                if (k->cKind == ConstantKind::E) return output.addRational(1, 1);
            }

            if (auto bp = getBinaryOp(output, arg)) {
                if (bp->bKind == BinaryOpKind::Power) {
                    if (auto k = getConstant(output, bp->left)) {
                        if (k->cKind == ConstantKind::E) return bp->right;
                    }
                }

                if (bp->bKind == BinaryOpKind::Multiply) {
                    NodeID lnA = output.addCall(FunctionKind::NaturalLogarithm, {bp->left});
                    NodeID lnB = output.addCall(FunctionKind::NaturalLogarithm, {bp->right});
                    return makeSum(output, lnA, lnB);
                }
                /*
                if (bp->bKind == BinaryOpKind::Divide) {
                    NodeID lnA = output.addCall(FunctionKind::NaturalLogarithm, {bp->left});
                    NodeID lnB = output.addCall(FunctionKind::NaturalLogarithm, {bp->right});
                    return makeSum(output, lnA, makeNeg(output, lnB));
                }
                */

                if (bp->bKind == BinaryOpKind::Power) {
                    NodeID lnBase = output.addCall(FunctionKind::NaturalLogarithm, {bp->left});
                    return makeProduct(output, bp->right, lnBase);
                }
            }
        }

        if (c->fKind == FunctionKind::Exponential && args.size() == 1) {
            NodeID arg = args[0];

            if (isZero(output, arg)) return output.addRational(1, 1);
            if (isOne(output, arg)) return output.addConstant(ConstantKind::E);

            if (auto inner = getCall(output, arg)) {
                if (inner->fKind == FunctionKind::NaturalLogarithm && inner->args.size() == 1) {
                    return inner->args[0];
                }
            }
        }

        if (c->fKind == FunctionKind::Logarithm) {
            NodeID lnX = output.addCall(FunctionKind::NaturalLogarithm, {args[0]});
            NodeID base = (args.size() >= 2) ? args[1] : output.addRational(10, 1);
            NodeID lnBase = output.addCall(FunctionKind::NaturalLogarithm, {base});
            return makeQuotient(output, lnX, lnBase);
        }

        return output.addCall(c->fKind, args);
    }

    return cloneSubtree(input, id, output);
}

NodeID canonicalOrder(const AST& input, const NodeID& id, AST& output) {
    if (id.isNone()) return NodeID::None();

    if (isLeafNode(input, id)) return cloneSubtree(input, id, output);

    if (auto b = getBinaryOp(input, id)) {
        NodeID left = canonicalOrder(input, b->left, output);
        NodeID right = canonicalOrder(input, b->right, output);

        if (b->bKind == BinaryOpKind::Add || b->bKind == BinaryOpKind::Multiply) {
            if (b->bKind == BinaryOpKind::Add) {
                AST temp;
                NodeID tempLeft = cloneSubtree(output, left, temp);
                NodeID tempRight = cloneSubtree(output, right, temp);
                NodeID tempAdd = temp.addBinaryOp(BinaryOpKind::Add, tempLeft, tempRight);

                std::vector terms = flattenSum(temp, tempAdd);
                if (terms.empty()) return output.addBinaryOp(b->bKind, left, right);

                std::sort(terms.begin(), terms.end(),
                    [&](const NodeID& a, const NodeID& b) {
                        return nodeLessThan(temp, a, b);
                });

                // rebuild as right-leaning chain
                NodeID result = cloneSubtree(temp, terms.back(), output);
                for (i16 i = (i16)terms.size() - 2; i >= 0; i--) {    // iterate backwards to preserve order
                    result = makeSum(output, cloneSubtree(temp, terms[i], output), result);
                }
                return result;
            }

            if (b->bKind == BinaryOpKind::Multiply) {
                AST temp;

                NodeID tempLeft = cloneSubtree(output, left, temp);
                NodeID tempRight = cloneSubtree(output, right, temp);
                NodeID tempMultiply = temp.addBinaryOp(BinaryOpKind::Multiply, tempLeft, tempRight);

                std::vector<NodeID> factors = flattenProduct(temp, tempMultiply);
                if (factors.empty()) return output.addBinaryOp(b->bKind, left, right);

                std::sort(factors.begin(), factors.end(),
                    [&](const NodeID& a, const NodeID& b) {
                        return nodeLessThan(temp, a, b);
                });

                NodeID result = cloneSubtree(temp, factors.back(), output);
                for (i16 i = (i16)factors.size() - 2; i >= 0; i--) {    // iterate backwards to preserve order
                    result = makeProduct(output, cloneSubtree(temp, factors[i], output), result);
                }
                return result;
            }
        }

        return output.addBinaryOp(b->bKind, left, right);
    }
    
    if (auto u = getUnaryOp(input, id)) {
        NodeID inner = canonicalOrder(input, u->inner, output);
        return output.addUnaryOp(u->uKind, inner);
    }

    if (auto c = getCall(input, id)) {
        std::vector<NodeID> args;
        args.reserve(c->args.size());
        for (const NodeID& arg : c->args) {
            args.emplace_back(canonicalOrder(input, arg, output));
        }
        return output.addCall(c->fKind, args);
    }

    return cloneSubtree(input, id, output);
}