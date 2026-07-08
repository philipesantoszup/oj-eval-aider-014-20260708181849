#pragma once
#ifndef PYTHON_INTERPRETER_EVALVISITOR_H
#define PYTHON_INTERPRETER_EVALVISITOR_H

#include "Python3ParserBaseVisitor.h"
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <sstream>
#include <any>
#include <algorithm>

// Basic BigInt implementation to handle arbitrary precision integers
struct BigInt {
    std::string val;
    bool negative;

    BigInt() : val("0"), negative(false) {}
    BigInt(long long v) {
        negative = v < 0;
        val = std::to_string(std::abs(v));
    }
    BigInt(std::string s) {
        if (s.empty()) { val = "0"; negative = false; return; }
        if (s[0] == '-') {
            negative = true;
            val = s.substr(1);
        } else {
            negative = false;
            val = s;
        }
        remove_leading_zeros();
        if (val == "0") negative = false;
    }

    void remove_leading_zeros() {
        size_t first = val.find_first_not_of('0');
        if (first == std::string::npos) val = "0";
        else val = val.substr(first);
    }

    std::string to_string() const {
        if (val == "0") return "0";
        return (negative ? "-" : "") + val;
    }

    bool operator<(const BigInt& other) const {
        if (negative != other.negative) return negative;
        if (val.length() != other.val.length()) {
            return negative ? val.length() > other.val.length() : val.length() < other.val.length();
        }
        return negative ? val > other.val : val < other.val;
    }

    bool operator==(const BigInt& other) const {
        return negative == other.negative && val == other.val;
    }

    static BigInt add_abs(const BigInt& a, const BigInt& b);
    static BigInt sub_abs(const BigInt& a, const BigInt& b);
    static BigInt mul_abs(const BigInt& a, const BigInt& b);
    static std::pair<BigInt, BigInt> div_mod_abs(const BigInt& a, const BigInt& b);

    BigInt operator+(const BigInt& other) const {
        if (negative == other.negative) {
            BigInt res = add_abs(*this, other);
            res.negative = negative;
            res.remove_leading_zeros();
            if (res.val == "0") res.negative = false;
            return res;
        }
        if (abs_less(other)) {
            BigInt res = sub_abs(other, *this);
            res.negative = other.negative;
            res.remove_leading_zeros();
            if (res.val == "0") res.negative = false;
            return res;
        } else {
            BigInt res = sub_abs(*this, other);
            res.negative = negative;
            res.remove_leading_zeros();
            if (res.val == "0") res.negative = false;
            return res;
        }
    }

    BigInt operator-(const BigInt& other) const {
        BigInt neg_other = other;
        neg_other.negative = !other.negative;
        if (neg_other.val == "0") neg_other.negative = false;
        return *this + neg_other;
    }

    BigInt operator*(const BigInt& other) const {
        BigInt res = mul_abs(*this, other);
        res.negative = (negative != other.negative);
        res.remove_leading_zeros();
        if (res.val == "0") res.negative = false;
        return res;
    }

    BigInt operator/(const BigInt& other) const {
        if (other.val == "0") return BigInt(0);
        auto res = div_mod_abs(*this, other);
        BigInt q = res.first;
        q.negative = (negative != other.negative);
        q.remove_leading_zeros();
        if (q.val == "0") q.negative = false;
        return q;
    }

    BigInt operator%(const BigInt& other) const {
        if (other.val == "0") return BigInt(0);
        auto res = div_mod_abs(*this, other);
        BigInt r = res.second;
        r.negative = negative;
        r.remove_leading_zeros();
        if (r.val == "0") r.negative = false;
        return r;
    }

private:
    bool abs_less(const BigInt& other) const {
        if (val.length() != other.val.length()) return val.length() < other.val.length();
        return val < other.val;
    }
};

// Represents a Python value.
struct Value {
    std::variant<BigInt, double, std::string, bool, std::nullptr_t> data;

    Value() : data(nullptr) {}
    Value(BigInt v) : data(v) {}
    Value(long long v) : data(BigInt(v)) {}
    Value(double v) : data(v) {}
    Value(std::string v) : data(v) {}
    Value(const char* v) : data(std::string(v)) {}
    Value(bool v) : data(v) {}
    Value(std::nullptr_t v) : data(v) {}

    std::string to_string() const {
        if (std::holds_alternative<BigInt>(data)) return std::get<BigInt>(data).to_string();
        if (std::holds_alternative<double>(data)) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6) << std::get<double>(data);
            return oss.str();
        }
        if (std::holds_alternative<std::string>(data)) return std::get<std::string>(data);
        if (std::holds_alternative<bool>(data)) return std::get<bool>(data) ? "True" : "False";
        return "None";
    }
};

class EvalVisitor : public Python3ParserBaseVisitor {
public:
    EvalVisitor();

    std::any visitFile_input(Python3Parser::File_inputContext *ctx) override;
    std::any visitStmt(Python3Parser::StmtContext *ctx) override;
    std::any visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) override;
    std::any visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) override;
    std::any visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) override;
    std::any visitArith_expr(Python3Parser::Arith_exprContext *ctx) override;
    std::any visitTerm(Python3Parser::TermContext *ctx) override;
    std::any visitFactor(Python3Parser::FactorContext *ctx) override;
    std::any visitAtom_expr(Python3Parser::Atom_exprContext *ctx) override;
    std::any visitAtom(Python3Parser::AtomContext *ctx) override;
    std::any visitTest(Python3Parser::TestContext *ctx) override;
    std::any visitOr_test(Python3Parser::Or_testContext *ctx) override;
    std::any visitAnd_test(Python3Parser::And_testContext *ctx) override;
    std::any visitNot_test(Python3Parser::Not_testContext *ctx) override;
    std::any visitComparison(Python3Parser::ComparisonContext *ctx) override;
    std::any visitTrailer(Python3Parser::TrailerContext *ctx) override;
    std::any visitTestlist(Python3Parser::TestlistContext *ctx) override;

private:
    std::map<std::string, Value> globals;
    Value evaluate_arithmetic(Value left, std::string op, Value right);
    std::string extract_name(Python3Parser::TestlistContext *ctx);
};

#endif//PYTHON_INTERPRETER_EVALVISITOR_H
