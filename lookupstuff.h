#ifndef LOOKUPSTUFF_H
#define LOOKUPSTUFF_H

#include <string>
#include <vector>
#include <optional>
#include <stdint.h>
#include <variant>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

static constexpr size_t UnknownPos = (size_t)-1;

struct Number {
    std::variant<double, i64> value;
    bool isInt = false;
};

// the supported latex commands that follow a '\'
enum class SupportedCommands {
    left, right,
    frac,
    sqrt,
    cdot, times, div,

    sin, cos, tan, csc, sec, cot,
    asin, acos, atan,
    arcsin, arccos, arctan,

    log, ln, exp,

    pi, infty, e,

    sum, prod,
    integrate, lim,

    max, min,

    operatorname    // custom ops
};

// stern brocot search for a fraction approx of a double
inline bool doubleToRational(const double& input, i64& outNumerator, i64& outDenominator) {
    // easier for sign stuff
    double value = input;
    
    bool canRationalize = false;
    bool isNegative = false;

    const double maxError = 1e-10;
    const u32 maxDenominator = 100000;

    if (value < 0) {
        isNegative = true;
        value = -value;
    }

    i64 wholePart = (i64)value;
    double fractionalPart = (double)(value - wholePart);

    i64 numL = 0;   i64 numR = 1;
    i64 denL = 1;   i64 denR = 1;

    while (denL + denR <= maxDenominator) {

        // calculate the mediant as fraction and double
        i64 numMediant = numL + numR;
        i64 denMediant = denL + denR;
        double mediant = (double)numMediant / (double)denMediant;

        // determine if the mediant is close enough to the input
        if (std::abs(fractionalPart - mediant) <= maxError) {
            outDenominator = denMediant;
            outNumerator = numMediant + wholePart * outDenominator;
            if (isNegative) outNumerator = -outNumerator;
            canRationalize = true;
            break;
        }

        // set bounds based on mediant
        if (mediant < fractionalPart) {
            numL = numMediant;
            denL = denMediant;
        } else if (mediant > fractionalPart) {
            numR = numMediant;
            denR = denMediant;
        }
    }
    
    return canRationalize;
}

#endif