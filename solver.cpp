#include "solver.h"

const NodeID solve(const NodeID& root) {
    NodeID current = root;
    bool changed;
    do {
        changed = false;
        current = apply_pass(current, arithmetic_rules, changed);
    } while (changed);
}