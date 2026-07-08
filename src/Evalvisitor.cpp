#include "Evalvisitor.h"

// BigInt helper implementations
BigInt BigInt::add_abs(const BigInt& a, const BigInt& b) {
    std::string res = "";
    int i = a.val.length() - 1, j = b.val.length() - 1, carry = 0;
    while (i >= 0 || j >= 0 || carry) {
        int sum = carry + (i >= 0 ? a.val[i--] - '0' : 0) + (j >= 0 ? b.val[j--] - '0' : 0);
        res += (sum % 10) + '0';
        carry = sum / 10;
    }
    std::reverse(res.begin(), res.end());
    return BigInt(res);
}

BigInt BigInt::sub_abs(const BigInt& a, const BigInt& b) {
    std::string res = "";
    int i = a.val.length() - 1, j = b.val.length() - 1, borrow = 0;
    while (i >= 0) {
        int sub = (a.val[i--] - '0') - (j >= 0 ? b.val[j--] - '0' : 0) - borrow;
        if (sub < 0) { sub += 10; borrow = 1; }
        else borrow = 0;
        res += sub + '0';
    }
    std::reverse(res.begin(), res.end());
    BigInt result(res);
    result.remove_leading_zeros();
    return result;
}

BigInt BigInt::mul_abs(const BigInt& a, const BigInt& b) {
    if (a.val == "0" || b.val == "0") return BigInt(0);
    std::vector<int> res(a.val.length() + b.val.length(), 0);
    for (int i = a.val.length() - 1; i >= 0; --i) {
        for (int j = b.val.length() - 1; j >= 0; --j) {
            res[i + j + 1] += (a.val[i] - '0') * (b.val[j] - '0');
            res[i + j] += res[i + j + 1] / 10;
            res[i + j + 1] %= 10;
        }
    }
    std::string s = "";
    for (int x : res) s += (x + '0');
    BigInt result(s);
    result.remove_leading_zeros();
    return result;
}

std::pair<BigInt, BigInt> BigInt::div_mod_abs(const BigInt& a, const BigInt& b) {
    if (b.val == "0") return {BigInt(0), BigInt(0)};
    if (a < b) return {BigInt(0), a};

    BigInt quotient("0"), remainder("0");
    for (char c : a.val) {
        remainder = remainder * BigInt(10);
        remainder.val += c;
        remainder.remove_leading_zeros();
        
        int d = 0;
        // Since we are dividing by b, and the current remainder is at most 10*b + 9,
        // d will be between 0 and 9.
        while (!(remainder < b)) {
            remainder = sub_abs(remainder, b);
            d++;
        }
        quotient.val += (d + '0');
    }
    quotient.remove_leading_zeros();
    return {quotient, remainder};
}

EvalVisitor::EvalVisitor() {}

std::any EvalVisitor::visitFile_input(Python3Parser::File_inputContext *ctx) {
    for (auto stmt : ctx->stmt()) {
        visitStmt(stmt);
    }
    return nullptr;
}

std::any EvalVisitor::visitStmt(Python3Parser::StmtContext *ctx) {
    if (ctx->simple_stmt()) {
        return visitSimple_stmt(ctx->simple_stmt());
    } else if (ctx->compound_stmt()) {
        return visitCompound_stmt(ctx->compound_stmt());
    }
    return nullptr;
}

std::any EvalVisitor::visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) {
    return visitSmall_stmt(ctx->small_stmt());
}

std::any EvalVisitor::visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) {
    if (ctx->expr_stmt()) {
        return visitExpr_stmt(ctx->expr_stmt());
    } else if (ctx->flow_stmt()) {
        return visitFlow_stmt(ctx->flow_stmt());
    }
    return nullptr;
}

std::any EvalVisitor::visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) {
    if (!ctx->ASSIGN().empty()) {
        Value val = std::any_cast<Value>(visitTestlist(ctx->testlist(1)));
        std::string var_name = extract_name(ctx->testlist(0));
        if (var_name != "unknown") {
            globals[var_name] = val;
        }
    } else {
        visitTestlist(ctx->testlist(0));
    }
    return nullptr;
}

std::string EvalVisitor::extract_name(Python3Parser::TestlistContext *ctx) {
    if (!ctx || ctx->test().empty()) return "unknown";
    auto test = ctx->test(0);
    if (!test->or_test()) return "unknown";
    auto or_test = test->or_test();
    if (or_test->and_test().empty()) return "unknown";
    auto and_test = or_test->and_test(0);
    if (and_test->not_test().empty()) return "unknown";
    auto not_test = and_test->not_test(0);
    if (!not_test->comparison()) return "unknown";
    auto comp = not_test->comparison();
    if (comp->arith_expr().empty()) return "unknown";
    auto arith = comp->arith_expr(0);
    if (arith->term().empty()) return "unknown";
    auto term = arith->term(0);
    if (term->factor().empty()) return "unknown";
    auto factor = term->factor(0);
    if (!factor->atom_expr()) return "unknown";
    auto atom_expr = factor->atom_expr();
    if (!atom_expr->atom()) return "unknown";
    auto atom = atom_expr->atom();
    if (atom->NAME()) return atom->NAME()->getText();
    return "unknown";
}

std::any EvalVisitor::visitTestlist(Python3Parser::TestlistContext *ctx) {
    Value res;
    if (ctx->test().size() > 0) {
        res = std::any_cast<Value>(visitTest(ctx->test(0)));
    }
    return res;
}

std::any EvalVisitor::visitAtom(Python3Parser::AtomContext *ctx) {
    if (ctx->NAME()) {
        std::string name = ctx->NAME()->getText();
        if (globals.count(name)) return globals[name];
        if (name == "print") {
            return std::string("builtin_print");
        }
        return Value(0LL);
    }
    if (ctx->NUMBER()) {
        std::string text = ctx->NUMBER()->getText();
        if (text.find('.') != std::string::npos) {
            return Value(std::stod(text));
        }
        return Value(std::stoll(text));
    }
    if (ctx->TRUE()) return Value(true);
    if (ctx->FALSE()) return Value(false);
    if (ctx->NONE()) return Value(nullptr);
    if (!ctx->STRING().empty()) {
        std::string s = ctx->STRING(0)->getText();
        if (s.length() >= 2) s = s.substr(1, s.length() - 2);
        return Value(s);
    }
    return Value(0LL);
}

std::any EvalVisitor::visitTrailer(Python3Parser::TrailerContext *ctx) {
    auto atom_expr = ctx->getParent()->getRuleContext<Python3Parser::Atom_exprContext>();
    if (atom_expr && atom_expr->atom()) {
        std::any atom_val = visitAtom(atom_expr->atom());
        Value v = std::any_cast<Value>(atom_val);
        if (std::holds_alternative<std::string>(v.data) && 
            std::get<std::string>(v.data) == "builtin_print") {
            
            if (ctx->arglist()) {
                auto args = ctx->arglist()->argument();
                for (size_t i = 0; i < args.size(); ++i) {
                    Value arg_val = std::any_cast<Value>(visitTest(args[i]->test(0)));
                    std::cout << arg_val.to_string();
                    if (i < args.size() - 1) std::cout << " ";
                }
            }
            std::cout << std::endl;
            return Value(nullptr);
        }
    }
    return Value(0LL);
}

std::any EvalVisitor::visitArith_expr(Python3Parser::Arith_exprContext *ctx) {
    Value res = std::any_cast<Value>(visitTerm(ctx->term(0)));
    for (size_t i = 0; i < ctx->addorsub_op().size(); ++i) {
        std::string op = ctx->addorsub_op(i)->getText();
        Value right = std::any_cast<Value>(visitTerm(ctx->term(i + 1)));
        res = evaluate_arithmetic(res, op, right);
    }
    return res;
}

std::any EvalVisitor::visitTerm(Python3Parser::TermContext *ctx) {
    Value res = std::any_cast<Value>(visitFactor(ctx->factor(0)));
    for (size_t i = 0; i < ctx->muldivmod_op().size(); ++i) {
        std::string op = ctx->muldivmod_op(i)->getText();
        Value right = std::any_cast<Value>(visitFactor(ctx->factor(i + 1)));
        res = evaluate_arithmetic(res, op, right);
    }
    return res;
}

std::any EvalVisitor::visitFactor(Python3Parser::FactorContext *ctx) {
    if (ctx->factor()) {
        Value v = std::any_cast<Value>(visitFactor(ctx->factor()));
        if (ctx->MINUS()) {
            if (std::holds_alternative<BigInt>(v.data)) {
                BigInt bi = std::get<BigInt>(v.data);
                if (bi.val != "0") bi.negative = !bi.negative;
                return Value(bi);
            }
            if (std::holds_alternative<double>(v.data)) {
                return Value(-std::get<double>(v.data));
            }
        } else if (ctx->ADD()) {
            return v;
        }
        return v;
    }
    return visitAtom_expr(ctx->atom_expr());
}

std::any EvalVisitor::visitAtom_expr(Python3Parser::Atom_exprContext *ctx) {
    std::any val = visitAtom(ctx->atom());
    if (ctx->trailer()) {
        val = visitTrailer(ctx->trailer());
    }
    return val;
}

Value EvalVisitor::evaluate_arithmetic(Value left, std::string op, Value right) {
    if (std::holds_alternative<BigInt>(left.data) && std::holds_alternative<BigInt>(right.data)) {
        BigInt l = std::get<BigInt>(left.data);
        BigInt r = std::get<BigInt>(right.data);
        if (op == "+") return Value(l + r);
        if (op == "-") return Value(l - r);
        if (op == "*") return Value(l * r);
        if (op == "/") return Value(l / r);
        if (op == "%") return Value(l % r);
    }
    
    double l = 0, r = 0;
    if (std::holds_alternative<BigInt>(left.data)) l = std::stod(std::get<BigInt>(left.data).to_string());
    else if (std::holds_alternative<double>(left.data)) l = std::get<double>(left.data);
    
    if (std::holds_alternative<BigInt>(right.data)) r = std::stod(std::get<BigInt>(right.data).to_string());
    else if (std::holds_alternative<double>(right.data)) r = std::get<double>(right.data);

    if (op == "+") return Value(l + r);
    if (op == "-") return Value(l - r);
    if (op == "*") return Value(l * r);
    if (op == "/") return Value(r == 0 ? 0.0 : l / r);
    return Value(0LL);
}

std::any EvalVisitor::visitTest(Python3Parser::TestContext *ctx) { return visitOr_test(ctx->or_test()); }
std::any EvalVisitor::visitOr_test(Python3Parser::Or_testContext *ctx) { return visitAnd_test(ctx->and_test(0)); }
std::any EvalVisitor::visitAnd_test(Python3Parser::And_testContext *ctx) { return visitNot_test(ctx->not_test(0)); }
std::any EvalVisitor::visitNot_test(Python3Parser::Not_testContext *ctx) { 
    if (ctx->comparison()) return visitComparison(ctx->comparison());
    return Value(false);
}
std::any EvalVisitor::visitComparison(Python3Parser::ComparisonContext *ctx) { return Value(true); }
