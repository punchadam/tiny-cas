/*
Solver

The solver takes a clean, fully-transformed AST rooted
at a BinaryOpKind::Equals node and tries to find the value
of a target variable that satisfies the equation.

Currently, it's a simple linear solver, which requires the
following pipeline:

    1. validateInput
        makes sure the root is actually an equals node,
        collects all identifiers in the tree, and checks
        that the target variable appears at least once.

    2. moveToOneSide
        turns a = b into a - b = 0, and re-runs transformer
        so that only one subtree must be solved.

    3. routePolynomial
        checks the degree of the polynomial and determines
        what to use based on that.

            - degree 0: checkTrivial()
            - degree 1: solveLinear()
            - degree 2: solveQuadratic()
            - degree 3: solveCubic()
            - degree 4: solveQuartic()
            - degree 5: idk it's impossible lol
    
    

*/

#ifndef SOLVER_H
#define SOLVER_H

#include "transformer.h"
#include <map>

struct NoSolution {
    // no value of any variable satisfies it or it's a contradiction (0 = 1)
};

struct Identity {
    // the equation is universally true (0 = 0)
    // every value of the target variable satisfies it
};

struct ExactSolutions {
    // closed-form solutions, each represented as an AST
    // may contain other identifiers for parametric solutions
    std::vector<AST> solutions; 
};

struct NumericSolutions {
    // float approximation used as a fallback
    std::vector<double> solutions;
};

struct SolutionSet {
    using Value = std::variant<
        NoSolution,
        Identity,
        ExactSolutions,
        NumericSolutions
    >;

    Value value;

    // the variable that was solved for
    std::string solvedVar;
};

// primary entry point
SolutionSet solve(const AST& input, const std::string& targetVar);

bool validateInput(const AST& ast, const std::string& targetVar);

AST moveToOneSide(const AST& input);

SolutionSet checkTrivial(const AST& f);

SolutionSet solveLinear(
    const AST& f,
    const std::string& targetVar,
    const std::map<i64, NodeID>& terms);

SolutionSet solveQuadratic(
    const AST& f,
    const std::string& targetVar,
    const std::map<i64, NodeID>& terms);

SolutionSet solveCubic(
    const AST& f,
    const std::string& targetVar,
    const std::map<i64, NodeID>& terms);

SolutionSet numericFallback(
    const AST& f,
    const std::string& targetVar);

SolutionSet retransformSolutions(SolutionSet result);

#endif