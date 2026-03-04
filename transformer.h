#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include "nodetools.h"

// returns a transformed AST
NodeID transform(const AST& input, AST& output);

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

// returns exact rational if possible
std::optional<NodeID> tryFoldPower(const RationalNode& base, const RationalNode& exp, AST& output, size_t pos);
// returns exact nth root of val if possible
std::optional<i64> tryIntegerRoot(i64 val, i64 n);
// tries to fold trig functions using known sin identities
std::optional<NodeID> tryFoldTrig(FunctionKind fKind, const AST& ast, const NodeID& arg, AST& out);

#endif