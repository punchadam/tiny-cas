#ifndef SOLVER_H
#define SOLVER_H

#include "parser.h"

struct Pass {

};

const Pass arithmetic_rules = {};

// main solver loop, iterates until nothing changes
const NodeID solve(const NodeID& root);

NodeID apply_pass(const NodeID& root, const Pass& rules, bool& changed);

#endif