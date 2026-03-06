#ifndef AST_H
#define AST_H

/*
The Abstract Syntax Tree (AST)

This is super important in understanding what's
happening here. The parser will turn a list of tokens
into a tree, in which each branch is a bound expression
and the root node is usually some sort of operator.
This encodes order, and allows us to do some cool
simplification and solving tricks that would be otherwise
very hard to implement on a simple stream of tokens,
or even worse, on a raw ass string.

Not gonna explain much more here, but here's one
example with the right-sides labeled:

    "2 + 3 * (5 - 2)" becomes:

      + <- ROOT (binary node)
     / \
    2   * <- (binary node)
       / \
      3   - <- (binary node)
         / \
        5   2 <- (rational node)

for more on ASTs, see https://medium.com/@jessica_lopez/basic-understanding-of-abstract-syntax-tree-ast-d40ff911c3bf

This AST implementation is stored in a flat vector,\
called an arena, and its storage structure is abstracted
by the helper methods like at() and all of those add_()s.

Each node in the tree can be one of the below
node structs:
    - Constant (like pi, e, i)
    - Real (stored as a double)
    - Rational (stored as i64s numerator & denominator)
    - Identifier (like x, theta, etc)
    - Binary Operation (left child and right child)
    - Unary Operation (inner child)
    - Function call (like sin(, max(, etc wiht a vector of arguments)

Every operation or function call can have ANY other
node as one of its children, forming a tree structure.
*/

#include "lookupstuff.h"
#include <variant>
#include <numeric>

struct NodeID {
    // i is the index of this node in the arena
    size_t i = 0;
    static constexpr size_t noPosition = (size_t)-1;
    static NodeID None() { return NodeID{ noPosition }; }
    bool isNone() const { return i == noPosition; }
};

enum class ConstantKind {
    PI,
    E,
    I
};
struct ConstantNode {
    ConstantKind cKind;
    size_t pos = 0;
    const std::string toString() const {
        switch (cKind) {
            case ConstantKind::E: return "e";
            case ConstantKind::I: return "i";
            case ConstantKind::PI: return "pi";
            default: return "Unknown Constant";
        }
    }
};

struct RealNode {
    double value = 0.0;
    size_t pos = 0;
    const std::string toString() const {
        return std::to_string(value);
    }
};

struct RationalNode {
    i64 numerator = 0;
    i64 denominator = 0;
    size_t pos = 0;
    const std::string toString() const {
        return std::to_string(numerator) + "/" + std::to_string(denominator);
    }
};

struct IdentifierNode {
    std::string name;
    size_t pos = 0;
    const std::string toString() const {
        return name;
    }
};

enum class BinaryOpKind {
    Add,
    Subtract,
    Multiply,
    Divide,
    Power,

    Equals
    // NotEquals,
    // LessThan,
    // GreaterThan,
    // LessThanOrEqual,
    // GreaterThanOrEqual
};
struct BinaryOpNode {
    BinaryOpKind bKind;
    NodeID left = NodeID::None();
    NodeID right = NodeID::None();
    size_t pos = 0;
    const std::string toString() const {
        switch (bKind) {
            case BinaryOpKind::Equals: return "=";
            case BinaryOpKind::Add: return "+";
            case BinaryOpKind::Subtract: return "-";
            case BinaryOpKind::Multiply: return "*";
            case BinaryOpKind::Divide: return "/";
            case BinaryOpKind::Power: return "^";
            default: return "Unknown Binary Operation";
        }
    }
};

enum class UnaryOpKind {
    Negate,
    Factorial,
    Percent
};
struct UnaryOpNode {
    UnaryOpKind uKind;
    NodeID inner = NodeID::None();
    size_t pos = 0;
    const std::string toString() const {
        switch (uKind) {
            case UnaryOpKind::Negate: return "-";
            case UnaryOpKind::Factorial: return "!";
            case UnaryOpKind::Percent: return "%";
            default: return "Unknown Unary Operation";
        }
    }
};

enum class FunctionKind {
    Sine,
    Cosine,
    Tangent,
    Atan2,
    AbsoluteValue,
    Exponential,
    NaturalLogarithm,
    Logarithm,
    Hypotenuse,
    Max,
    Min
};
struct CallNode {
    FunctionKind fKind;
    std::vector<NodeID> args;
    size_t pos = 0;
    const std::string toString() const {
        switch (fKind) {
            case FunctionKind::Sine: return "sin";
            case FunctionKind::Cosine: return "cos";
            case FunctionKind::Tangent: return "tan";
            case FunctionKind::Atan2: return "atan2";
            case FunctionKind::AbsoluteValue: return "abs";
            case FunctionKind::Exponential: return "exp";
            case FunctionKind::NaturalLogarithm: return "ln";
            case FunctionKind::Logarithm: return "log";
            case FunctionKind::Hypotenuse: return "hypot";
            case FunctionKind::Max: return "max";
            case FunctionKind::Min: return "min";
            default: return "Unknown Call";
        }
    }
};


struct ASTNode {
    using Kind = std::variant<
    ConstantNode,
    RealNode,
    RationalNode,
    IdentifierNode,
    BinaryOpNode,
    UnaryOpNode,
    CallNode
    >;

    Kind kind;

    template <class T>
    ASTNode(T t) : kind(std::move(t)) {}
};

class AST {
    public:
        NodeID root = NodeID::None();

        // storage arena for all nodes
        std::vector<ASTNode> arena;

        // modifiable reference
        ASTNode& at(NodeID id) { return arena.at(id.i); }
        // unmodifyiable
        const ASTNode& at(NodeID id) const { return arena.at(id.i); }

        void reserve(size_t n) { arena.reserve(n); }

        // using "const" and "&" to avoid copying unneccessarily

        NodeID addConstant(const ConstantKind& cKind, const size_t& pos = UnknownPos) {
            return addNode(ConstantNode{ cKind, pos });
        }

        NodeID addReal(const double& value, const size_t& pos = UnknownPos) {
            return addNode(RealNode{ value, pos });
        }

        NodeID addRational(const i64& numerator, const i64& denominator, const size_t& pos = UnknownPos) {
            i64 commonDivisor = std::gcd(std::abs(numerator), std::abs(denominator));
            return addNode(RationalNode{ numerator / commonDivisor, denominator / commonDivisor, pos });
        }

        NodeID addIdentifier(const std::string& name, const size_t& pos = UnknownPos) {
            return addNode(IdentifierNode{ name, pos });
        }

        NodeID addBinaryOp(const BinaryOpKind& bKind, const NodeID& left, const NodeID& right, const size_t& pos = UnknownPos) {
            return addNode(BinaryOpNode{ bKind, left, right, pos });
        }

        NodeID addUnaryOp(const UnaryOpKind& uKind, const NodeID& inner, const size_t& pos = UnknownPos) {
            return addNode(UnaryOpNode{ uKind, inner, pos });
        }

        NodeID addCall(const FunctionKind& fKind, const std::vector<NodeID>& args, const size_t& pos = UnknownPos) {
            return addNode(CallNode{ fKind, args, pos });
        }

        const std::string toString() const {
            return toString(root, 0);
        }
    
    private:
        template <class T>
        NodeID addNode(T t) {
            // arena.size() becomes i in NodeID
            NodeID id{ arena.size() };
            arena.emplace_back(std::move(t));
            return id;
        }

        std::string toString(NodeID id, u8 depth) const {
            std::string indent(2*depth, ' ');
            std::string result;
            
            std::visit([&](auto& node) {
                using T = std::decay_t<decltype(node)>;
                if constexpr (std::is_same_v<T, RationalNode>) {
                    if (node.denominator == 1) result += indent + std::to_string(node.numerator) + "\n";
                    else result += indent + node.toString() + "\n";
                } else result += indent + node.toString() + "\n";

                if constexpr (std::is_same_v<T, BinaryOpNode>) {
                    result += toString(node.left, depth + 1);
                    result += toString(node.right, depth + 1);
                }
                if constexpr (std::is_same_v<T, UnaryOpNode>) {
                    result += toString(node.inner, depth + 1);
                }
                if constexpr (std::is_same_v<T, CallNode>) {
                    for (const NodeID& arg : node.args) {
                        result += toString(arg, depth + 1);
                    }
                }
            }, at(id).kind);
            return result;
        }
};

#endif