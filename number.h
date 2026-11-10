// In a CAS, it is important to be able to handle different kinds
// of numbers, sometimes even all at once. As such, it cleans up
// the code quite a bit to create a variant "Number" class, which
// overloads common operators and retains precision wherever possible.

// For example, dividing a rational by an integer should not result in
// a cast to a lesser-precision data type like double, and instead should\
// stay as either a rational or an integer.

// In this implementation, rationals also automatically simplify upon creation.

#pragma once

#include <stdint.h>
#include <string>
#include <variant>
#include <stdexcept>

using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8 = int8_t;
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

inline i64 gcd(i64 a, i64 b) {
    while (b != 0) {
        i64 temp = b;
        b = a % b;
        a = temp;
    }
    return a < 0 ? -a : a;
}

struct Rational {
    i64 num, den;

    Rational(i64 n = 0, i64 d = 1) : num(n), den(d) {
        if (den == 0) throw std::invalid_argument("Rational Denominator is Zero");
        if (den < 0) {
            num = -num;
            den = -den;
        }
        reduce();
    }

    void reduce() {
        if (num == 0) {
            den = 1;
            return;
        }

        i64 common = gcd(num, den);
        num /= common;
        den /= common;
    }

    double toDouble() const {
        return static_cast<double>(num) / static_cast<double>(den);
    }
};

enum class NumberType {
    REAL, RATIONAL, INTEGER
};

class Number {
public:
    std::variant<i64, Rational, double> value;

    Number(i64 v) : value(v) {}
    Number(double v) : value(v) {}
    Number(Rational r) : value(r) {}
    Number(i64 num, i64 den) : value(Rational(num, den)) {}

    NumberType type() const {
        if (std::holds_alternative<i64>(value)) return NumberType::INTEGER;
        if (std::holds_alternative<Rational>(value)) return NumberType::RATIONAL;
        return NumberType::REAL;
    }

    double toDouble() const {
        switch (type()) {
            case NumberType::INTEGER:
                return static_cast<double>(std::get<i64>(value));
            case NumberType::RATIONAL:
                return std::get<Rational>(value).toDouble();
            case NumberType::REAL:
                return std::get<double>(value);
        }
        return 0.0;
    }

    friend Number operator+(const Number& left, const Number& right) {
        if (left.type() == NumberType::REAL || right.type() == NumberType::REAL) {
            return left.toDouble() + right.toDouble();
        }
        if (left.type() == NumberType::RATIONAL || right.type() == NumberType::RATIONAL) {
            Rational r1 = toRational(left);
            Rational r2 = toRational(right);
            return Number(r1.num * r2.den + r2.num * r1.den, r1.den * r2.den);
        }
        i64 a = std::get<i64>(left.value);
        i64 b = std::get<i64>(right.value);

        return Number(a + b);
    }
    friend Number operator-(const Number& left, const Number& right) {
        if (left.type() == NumberType::REAL || right.type() == NumberType::REAL) {
            return left.toDouble() - right.toDouble();
        }
        if (left.type() == NumberType::RATIONAL || right.type() == NumberType::RATIONAL) {
            Rational r1 = toRational(left);
            Rational r2 = toRational(right);
            return Number(r1.num * r2.den - r2.num * r1.den, r1.den * r2.den);
        }
        i64 a = std::get<i64>(left.value);
        i64 b = std::get<i64>(right.value);

        return Number(a - b);
    }
    friend Number operator*(const Number& left, const Number& right) {
        if (left.type() == NumberType::REAL || right.type() == NumberType::REAL) {
            return left.toDouble() * right.toDouble();
        }
        if (left.type() == NumberType::RATIONAL || right.type() == NumberType::RATIONAL) {
            Rational r1 = toRational(left);
            Rational r2 = toRational(right);
            return Number(r1.num * r2.num, r1.den * r2.den);
        }
        i64 a = std::get<i64>(left.value);
        i64 b = std::get<i64>(right.value);

        return Number(a * b);
    }
    friend Number operator/(const Number& lhs, const Number& rhs) {
        if (lhs.type() == NumberType::REAL || rhs.type() == NumberType::REAL || rhs.toDouble() == 0.0) {
            double denom = rhs.toDouble();
            if (denom == 0.0) throw std::runtime_error("Division by Zero");
            return Number(lhs.toDouble() / denom);
        }
        // division is just a rational
        Rational r1 = toRational(lhs);
        Rational r2 = toRational(rhs);
        if (r2.num == 0) throw std::runtime_error("Division by Zero");
        return Rational(r1.num * r2.den, r1.den * r2.num);
    }

    std::string toString() const {
        switch (type()) {
            case NumberType::INTEGER:
                return std::to_string(std::get<i64>(value));
            case NumberType::RATIONAL: {
                auto r = std::get<Rational>(value);
                if (r.den == 1) {
                    return std::to_string(r.num);
                } else {
                    return std::to_string(r.num) + "/" + std::to_string(r.den);
                }
            }
            case NumberType::REAL:
                return std::to_string(std::get<double>(value));
        }
        return "";
    }

private:
    static Rational toRational(const Number& n) {
        switch (n.type()) {
            case NumberType::INTEGER:
                return Rational(std::get<i64>(n.value), 1);
            case NumberType::RATIONAL:
                return std::get<Rational>(n.value);
            case NumberType::REAL:
                double d = std::get<double>(n.value);
                return Rational(static_cast<i64>(d * 1e6, 1e6));
        }        
        return Rational(0, 1);
    }
};