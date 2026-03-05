/*
Multi-Pass Transformer

It's getting FUN!!!! The transformer turns a rough AST
full of shit like "x * 1", "ln(a ^ n)", and other stuff
that the solver shouldn't be bothered by into a clean,
deterministic, and happy little tree.

This takes multiple passes, each doing its own thing
to prune and canonicalize the tree. Canonicalize just
means predictable in shape.

    1. foldConstants
        Finds nodes whose children are all constants and
        evaluates ahead of time. It only does this if the
        children are rationalNodes though, as operating on
        floating point stuff is lossy.
    
    2. eliminateSubtraction
        Converts any BinaryOpNode of type Subtract into
        addition of the negative using UnaryOpNode. This
        means I don't have to worry about subtraction at
        all in my solver or even in later transformer passes.

    3. simplifyIdentities
        Uses operator identities and destructors like
        "x + 0" and "x * 1" to simplify the AST

    4. applyTrigIdentities
        Routes all trig functions via their sin identities
        through a known sin lookup function to figure out
        if they can be simplified. For example:
            cos(k * pi) = sin((1/2 - k) * pi)
          & tan(k * pi) = sin(k * pi) / sin((1/2 * k) * pi)
            because tan = sin / cos
        There's a list of known sin values at rational
        multiples of pi that are checked against the form
        of the input to simplify the tree where possible.
    
    5. canonicalizeLogExp
        Applies standard log and exp rules to expressions
        like "exp(ln(x)) = x" to simplify. Also, uses the
        change-of-base rules to canonicalize arbitrary bases
        into purely natural-log form.

Each of these passes is done in sequence, and then each
sequence is repeated using a fixed-point loop until the AST
stops changing between iterations. This is because something
like 2 * sin(pi) might expand into 2 * 0, which should be
folded by another iteration of the sequence.
*/

#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include "nodetools.h"

// returns a transformed AST
NodeID transform(const AST& input, AST& output);

// removes negate as a unary op and instead stores it directly or by (-1) * x
NodeID eliminateNegate(const AST& input, const NodeID& id, AST& output);

// losslessly folds constant subtrees
NodeID foldConstants(const AST& input, const NodeID& id, AST& output);

// turns all subtraction into addition of negation 
NodeID eliminateSubtraction(const AST& input, const NodeID& id, AST& output);

// simplifies stuff like x * 0, x + 0, x * 1, etc
NodeID simplifyIdentities(const AST& input, const NodeID & id, AST& output);

// folds trig functions at known multiples of pi into exact values
NodeID applyTrigIdentities(const AST& input, const NodeID id, AST& output);

// applies standard log/exp rules plus expansion/change of base stuff to canonicalize to ln
NodeID canonicalizeLogExp(const AST& input, const NodeID id, AST& output);

// finds and sums coefficients within a group of like terms
NodeID combineLikeTerms(const AST& input, const NodeID& id, AST& output);

// convert "-(-x)" into "x"
NodeID normalizeSign(const AST& input, const NodeID& id, AST& output);

// returns exact rational if possible
std::optional<NodeID> tryFoldPower(const RationalNode& base, const RationalNode& exp, AST& output);
// returns exact nth root of val if possible
std::optional<i64> tryIntegerRoot(i64 val, i64 n);
// tries to fold trig functions using known sin identities
std::optional<NodeID> tryFoldTrig(FunctionKind fKind, const AST& ast, const NodeID& arg, AST& out);

// normalize a rational to [0, 2] by mod 2
static RationalNode modTwoPi(RationalNode r);
// shift a pi coefficient (returns 1/2 - k) so cos(k * pi) = sin((1/2 - k) * pi)
static RationalNode cosShift(const RationalNode& k);
// core sin solver
static std::optional<NodeID> foldSinAtPiMultiple(const RationalNode& piCoeff, AST& out);

#endif