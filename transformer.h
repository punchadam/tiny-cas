#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include "nodetools.h"

// returns a transformed AST
NodeID transform(const AST& input, AST& output);

// losslessly folds constant subtrees
NodeID foldConstants(const AST& input, const NodeID& id, AST& output);

// turns all subtraction into addition of negation 
NodeID eliminateSubtraction(const AST& input, const NodeID& id, AST& output);

#endif