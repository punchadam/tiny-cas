#include "solver.h"

SolutionSet solve(const AST& input, const std::string& targetVar) {

}

bool validateInput(const AST& ast, const std::string& targetVar) {

}

AST moveToOneSide(const AST& input) {

}

SolutionSet checkTrivial(const AST& f) {
    
}

SolutionSet solveLinear(const AST& f, const std::string& targetVar, const std::map<i64, NodeID>& terms) {

}

SolutionSet solveQuadratic(const AST& f, const std::string& targetVar, const std::map<i64, NodeID>& terms) {

}

SolutionSet solveCubic(const AST& f, const std::string& targetVar, const std::map<i64, NodeID>& terms) {

}

SolutionSet numericFallback(const AST& f, const std::string& targetVar) {

}

SolutionSet retransformSolutions(SolutionSet result) {

}