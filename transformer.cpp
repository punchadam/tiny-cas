#include "transformer.h"

NodeID transform(const AST& input, AST& output) {
    AST temp;
    NodeID afterFold = foldConstants(input, input.root, temp);
    temp.root = afterFold;
    NodeID afterSubtractionElim = eliminateSubtraction(temp, temp.root, output);
    output.root = afterSubtractionElim;
    return afterSubtractionElim;
}

NodeID foldConstants(const AST& input, const NodeID& id, AST& output) {
    if (id.isNone()) return;

}

NodeID eliminateSubtraction(const AST& input, const NodeID& id, AST& output) {

}