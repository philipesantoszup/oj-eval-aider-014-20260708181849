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
        if (val == "0") negative = false;
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

    // Override visitor methods to implement Python evaluation logic
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

private:
    std::map<std::string, Value> globals;
    Value evaluate_arithmetic(Value left, std::string op, Value right);
};

#endif//PYTHON_INTERPRETER_EVALVISITOR_H
