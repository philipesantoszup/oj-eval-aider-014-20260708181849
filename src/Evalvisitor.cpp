#include "Evalvisitor.h"

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
    // Handle assignment: testlist = testlist
    if (!ctx->ASSIGN().empty()) {
        Value val = std::any_cast<Value>(visitTestlist(ctx->testlist(0)));
        // Simplified: only handle single variable assignment for now
        // In a real implementation, we'd iterate through the testlist
        // For now, we assume the first element of the testlist is a name
        // This is a placeholder for the complex unpacking logic
    } else {
        // Just evaluate the expression
        visitTestlist(ctx->testlist(0));
    }
    return nullptr;
}

std::any EvalVisitor::visitAtom(Python3Parser::AtomContext *ctx) {
    if (ctx->NAME()) {
        std::string name = ctx->NAME()->getText();
        if (globals.count(name)) return globals[name];
        // Handle built-ins like 'print'
        if (name == "print") {
            // We return a special marker or handle it in Trailer
            return std::string("builtin_print");
        }
        return Value(0LL); // Default
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
    // Handle function calls: atom(args)
    // This is where 'print' is actually executed
    // We need the atom value from the parent
    // For simplicity in this first pass, we'll handle the print call
    // by checking the context or the result of the atom
    
    // This is a simplified implementation
    if (ctx->arglist()) {
        // If the atom was "print", we print the arguments
        // In a real visitor, we'd pass the atom's value here
        // For now, we'll just print a placeholder or the first arg
        // to show we are making progress.
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
    // Simplified arithmetic for BigInt (converting to long long for now as a placeholder)
    auto to_ll = [](const Value& v) -> long long {
        if (std::holds_alternative<BigInt>(v.data)) {
            BigInt bi = std::get<BigInt>(v.data);
            long long res = std::stoll(bi.val);
            return bi.negative ? -res : res;
        }
        if (std::holds_alternative<double>(v.data)) {
            return static_cast<long long>(std::get<double>(v.data));
        }
        return 0LL;
    };

    long long l = to_ll(left);
    long long r = to_ll(right);
    if (op == "+") return Value(l + r);
    if (op == "-") return Value(l - r);
    if (op == "*") return Value(l * r);
    if (op == "/") return Value(r == 0 ? 0LL : l / r);
    return Value(0LL);
}

// Stubs for other required methods to avoid crashes
std::any EvalVisitor::visitTest(Python3Parser::TestContext *ctx) { return visitOr_test(ctx->or_test()); }
std::any EvalVisitor::visitOr_test(Python3Parser::Or_testContext *ctx) { return visitAnd_test(ctx->and_test(0)); }
std::any EvalVisitor::visitAnd_test(Python3Parser::And_testContext *ctx) { return visitNot_test(ctx->not_test(0)); }
std::any EvalVisitor::visitNot_test(Python3Parser::Not_testContext *ctx) { 
    if (ctx->comparison()) return visitComparison(ctx->comparison());
    return Value(false);
}
std::any EvalVisitor::visitComparison(Python3Parser::ComparisonContext *ctx) { return Value(true); }
