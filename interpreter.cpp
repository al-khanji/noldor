#include "noldor.h"
#include "noldor_impl.h"

#include <stdlib.h>
#include <stddef.h>

#include <iostream>

#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <mutex>
#include <thread>

namespace noldor {

#define X(n) #n,
const char * const regnames[] = { REGISTERS(X) };
#undef X

static bool is_self_evaluating(value exp)
{
    return is_number(exp) || object_metaobject(exp)->flags & typeflags_self_eval;
}

static bool is_quoted(value exp)
{
    return is_tagged_list(exp, SYMBOL_LITERAL(quote));
}

static bool is_quasiquoted(value exp)
{
    return is_tagged_list(exp, SYMBOL_LITERAL(quasiquote));
}

static bool is_unquoted(value exp)
{
    return is_tagged_list(exp, SYMBOL_LITERAL(unquote));
}

static bool is_unquoted_splicing(value exp)
{
    return is_tagged_list(exp, SYMBOL_LITERAL(unquote-splicing));
}

static value text_of_quotation(value exp)
{
    return cadr(exp);
}

static bool is_variable(value exp)
{
    return is_symbol(exp);
}

static bool is_assignment(value exp)
{
    return is_tagged_list(exp, SYMBOL_LITERAL(set!));
}

static value assignment_variable(value exp)
{
    return cadr(exp);
}

static value assignment_value(value exp)
{
    return caddr(exp);
}

static bool is_lambda(value exp)
{
    return is_tagged_list(exp, SYMBOL_LITERAL(lambda));
}

static value lambda_parameters(value exp)
{
    return cadr(exp);
}

static value lambda_body(value exp)
{
    return cddr(exp);
}

static value make_lambda(value parameters, value body)
{
    return cons(SYMBOL_LITERAL(lambda), cons(parameters, body));
}

static bool is_definition(value exp)
{
    return is_tagged_list(exp, SYMBOL_LITERAL(define));
}

static value definition_variable(value exp)
{
    const bool is_sym = is_symbol(cadr(exp));
    if (is_sym) return cadr(exp);
    return caadr(exp);
}

static value definition_value(value exp)
{
    const bool is_sym = is_symbol(cadr(exp));
    if (is_sym) return caddr(exp);
    return make_lambda(cdadr(exp), cddr(exp));
}

static bool is_if(value exp)
{
    return is_tagged_list(exp, SYMBOL_LITERAL(if));
}

static value if_predicate(value exp)
{
    return cadr(exp);
}

static value if_consequent(value exp)
{
    return caddr(exp);
}

static value if_alternative(value exp)
{
    return is_null(cdddr(exp)) == false ? cadddr(exp)
                                        : mk_bool(false);
}

static bool is_begin(value exp)
{
    return is_tagged_list(exp, SYMBOL_LITERAL(begin));
}

static value begin_actions(value exp)
{
    return cdr(exp);
}

static bool is_last_exp(value seq)
{
    return is_null(cdr(seq));
}

static value first_exp(value seq)
{
    return car(seq);
}

static value rest_exps(value seq)
{
    return cdr(seq);
}

static bool is_application(value exp)
{
    return is_pair(exp);
}

static value operator_(value exp)
{
    return car(exp);
}

static value operands(value exp)
{
    return cdr(exp);
}

static bool is_last_operand(value ops)
{
    return is_null(cdr(ops));
}

static bool has_no_operands(value ops)
{
    return is_null(ops);
}

static value first_operand(value ops)
{
    return car(ops);
}

static value rest_operands(value ops)
{
    return cdr(ops);
}

static value make_if(value predicate, value consequent, value alternative)
{
    return list(SYMBOL_LITERAL(if), predicate, consequent, alternative);
}

static value make_begin(value seq)
{
    return cons(SYMBOL_LITERAL(begin), seq);
}

static value sequence_to_exp(value seq)
{
    if (is_null(seq))
        return seq;
    else if (is_last_exp(seq))
        return first_exp(seq);
    else
        return make_begin(seq);
}

static bool is_cond(value exp)
{
    return is_tagged_list(exp, SYMBOL_LITERAL(cond));
}

static value cond_clauses(value exp)
{
    return cdr(exp);
}

static value cond_predicate(value clause)
{
    return car(clause);
}

static value cond_actions(value clause)
{
    return cdr(clause);
}

static bool is_cond_else_clause(value clause)
{
    return eq(cond_predicate(clause), SYMBOL_LITERAL(else));
}

static value expand_clauses(value clauses)
{
    if (is_null(clauses))
        return clauses;

    auto first = car(clauses);
    auto rest = cdr(clauses);

    if (is_cond_else_clause(first)) {
        if (!is_null(rest))
            std::cerr << "warning: cond else clause isn't last, ignoring tail: " << clauses << std::endl;

        return sequence_to_exp(cond_actions(first));
    }

    return make_if(cond_predicate(first), sequence_to_exp(cond_actions(first)), expand_clauses(rest));
}

static value cond_to_if(value exp)
{
    return expand_clauses(cond_clauses(exp));
}

static value empty_arglist()
{
    return list();
}

static value adjoin_arg(value arg, value arglist)
{
    return append(arglist, list(arg));
}

static value splice_arg(value arg, value arglist)
{
    if (is_null(arg))
        return arglist;

    check_type(is_pair, arg, "splice_arg: expected list");
    return append(arglist, arg);
}

static value extend_environment(value vars, value vals, value base_env)
{
    auto env = mk_environment(base_env);

    while (!is_null(vars) && !is_null(vals)) {
        environment_define(env, car(vars), car(vals));
        vars = cdr(vars);
        vals = cdr(vals);
    }

    if (!is_null(vars))
        std::cerr << "extend_environment: extra variables: " << vars << std::endl;
    else if (!is_null(vals))
        std::cerr << "extend_environment: extra values: " << vals << std::endl;

    return env;
}

static value interpret(value exp, value env)
{
#define X_LABELS(X) \
 X(unknown_expression_type) \
 X(unknown_procedure_type) \
 X(eval_finished) \
 X(eval_dispatch) \
 X(ev_cond) \
 X(ev_self_eval) \
 X(ev_variable) \
 X(ev_quoted) \
 X(ev_quasiquoted) \
 X(ev_qq_operand_loop) \
 X(ev_qq_done) \
 X(ev_qq_arg_splice) \
 X(ev_qq_arg_append) \
 X(ev_qq_arg_unquote_splicing) \
 X(ev_qq_arg_unquote) \
 X(en_qq_arg_dispatch) \
 X(ev_lambda) \
 X(ev_application) \
 X(ev_appl_did_operator) \
 X(ev_appl_operand_loop) \
 X(ev_appl_accumulate_arg) \
 X(ev_appl_last_arg) \
 X(ev_appl_accum_last_arg) \
 X(apply_dispatch) \
 X(primitive_apply) \
 X(compound_apply) \
 X(ev_begin) \
 X(ev_sequence) \
 X(ev_sequence_continue) \
 X(ev_sequence_last_exp) \
 X(ev_if) \
 X(ev_if_decide) \
 X(ev_if_alternative) \
 X(ev_if_consequent) \
 X(ev_assignment) \
 X(ev_assignment_1) \
 X(ev_definition) \
 X(ev_definition_1)

#define X(LABEL) LABEL_##LABEL,
enum : uint64_t { X_LABELS(X) };
#undef X

//#define NOLDOR_TRACE_INTERPRETER

#ifdef NOLDOR_TRACE_INTERPRETER
auto trace_args = [] (std::initializer_list<std::string> parts) -> std::string {
    return std::accumulate(parts.begin(), parts.end(), std::string{}, [&] (const std::string &acc, const std::string &part) {
        if (acc.empty()) return part;
        return acc + ", " + part;
    });
};
# define TRACE(INDENT, OP, ...) {std::cerr << std::string(INDENT, ' ') << #OP << "(" << trace_args({__VA_ARGS__}) << ")" << std::endl;}
# define TRACE_BARE(MSG) {std::cerr << #MSG << std::endl;}
#else
# define TRACE(...)
# define TRACE_BARE(...)
#endif

#if defined(__GNUC__)
#  define COMPUTED_GOTO
#endif

#ifdef COMPUTED_GOTO
# define X(LABEL) &&LABEL,
    static const void *JUMPS[] = { X_LABELS(X) };
# undef X
# define DISPATCH(DEST)    goto *JUMPS[DEST]
# define MAKE_LABEL(NAME)  NAME: TRACE_BARE(\n##NAME);
# define ENTER_INTERPRETER TRACE_BARE(\n##ENTER_COMPUTED_GOTOS);
# define EXIT_INTERPRETER
#else // COMPUTED_GOTO
# define DISPATCH(DEST)    next_interp_branch = DEST; break
# define MAKE_LABEL(NAME)  case LABEL_##NAME: TRACE_BARE(\n##NAME);
# define ENTER_INTERPRETER TRACE_BARE(\n##ENTER_WHILE_TRUE_SWITCH); uint64_t next_interp_branch = ~0; while (true) { switch (next_interp_branch) { default:
# define EXIT_INTERPRETER  }}
#endif // COMPUTED_GOTO

#define GOTO(DEST)       { TRACE(4, GOTO,    #DEST);      DISPATCH(DEST);                     }
#define BRANCH(DEST)     { TRACE(4, BRANCH,  #DEST);      DISPATCH(DEST);                     }

#define ASSIGN(REG, VAL) { TRACE(4, ASSIGN,  #REG, #VAL); thread.assign(reg::REG, VAL);       }
#define RESTORE(REG)     { TRACE(4, RESTORE, #REG);       thread.restore(reg::REG);           }
#define SAVE(REG)        { TRACE(4, SAVE,    #REG);       thread.save(reg::REG);              }
#define ERROR(MSG, IRR)  { TRACE(4, ERROR,   MSG, #IRR);  throw noldor::base_error(MSG, IRR); }
#define PERFORM(ACTION)  { TRACE(4, PERFORM, #ACTION);    ACTION;                             }
#define TEST(tst)          TRACE(4, TEST,    #tst);       if (tst)
#define RETURN(VAL)        TRACE(4, RETURN,  #VAL);       return VAL;

#define CONST(NAME)        SYMBOL_LITERAL(NAME)
#define LABEL(NAME)        LABEL_##NAME
#define REG(NAME)          thread.getreg(reg::NAME)
#define OP(O, ...)         O(__VA_ARGS__)

    thread_t thread;

    ASSIGN(exp, exp)
    ASSIGN(env, env)
    ASSIGN(continu, LABEL(eval_finished))

ENTER_INTERPRETER
    GOTO(LABEL(eval_dispatch))

MAKE_LABEL(eval_finished)
    RETURN(REG(val))

MAKE_LABEL(unknown_expression_type)
    ERROR("unknown expression type", REG(exp))
    GOTO(LABEL(eval_finished))

MAKE_LABEL(unknown_procedure_type)
    RESTORE(continu)
    ERROR("unknown procedure type", REG(proc))
    GOTO(LABEL(eval_finished))

MAKE_LABEL(eval_dispatch)
    TEST(OP(is_self_evaluating, REG(exp)))
    BRANCH(LABEL(ev_self_eval))

    TEST(OP(is_variable, REG(exp)))
    BRANCH(LABEL(ev_variable))

    TEST(OP(is_quoted, REG(exp)))
    BRANCH(LABEL(ev_quoted))

    TEST(OP(is_quasiquoted, REG(exp)))
    BRANCH(LABEL(ev_quasiquoted))

    TEST(OP(is_assignment, REG(exp)))
    BRANCH(LABEL(ev_assignment))

    TEST(OP(is_definition, REG(exp)))
    BRANCH(LABEL(ev_definition))

    TEST(OP(is_if, REG(exp)))
    BRANCH(LABEL(ev_if))

    TEST(OP(is_lambda, REG(exp)))
    BRANCH(LABEL(ev_lambda))

    TEST(OP(is_begin, REG(exp)))
    BRANCH(LABEL(ev_begin))

    TEST(OP(is_cond, REG(exp)))
    BRANCH(LABEL(ev_cond))

    TEST(OP(is_application, REG(exp)))
    BRANCH(LABEL(ev_application))

    GOTO(LABEL(unknown_expression_type))

MAKE_LABEL(ev_cond)
    ASSIGN(exp, OP(cond_to_if, REG(exp)))
    GOTO(LABEL(eval_dispatch))

MAKE_LABEL(ev_self_eval)
    ASSIGN(val, REG(exp))
    GOTO(REG(continu))

MAKE_LABEL(ev_variable)
    ASSIGN(val, OP(environment_get, REG(env), REG(exp)))
    GOTO(REG(continu))

MAKE_LABEL(ev_quoted)
    ASSIGN(val, OP(text_of_quotation, REG(exp)))
    GOTO(REG(continu))

MAKE_LABEL(ev_quasiquoted)
    ASSIGN(unev, OP(text_of_quotation, REG(exp)))
    ASSIGN(argl, OP(empty_arglist))
    GOTO(LABEL(ev_qq_operand_loop))

MAKE_LABEL(ev_qq_operand_loop)
    TEST(OP(has_no_operands, REG(unev)))
    BRANCH(LABEL(ev_qq_done))
    ASSIGN(exp, OP(first_operand, REG(unev)))
    TEST(OP(is_unquoted, REG(exp)))
    BRANCH(LABEL(ev_qq_arg_unquote))
    TEST(OP(is_unquoted_splicing, REG(exp)))
    BRANCH(LABEL(ev_qq_arg_unquote_splicing))
    ASSIGN(argl, OP(adjoin_arg, REG(exp), REG(argl)))
    ASSIGN(unev, OP(rest_operands, REG(unev)))
    GOTO(LABEL(ev_qq_operand_loop))

MAKE_LABEL(ev_qq_done)
    ASSIGN(val, REG(argl))
    GOTO(REG(continu))

MAKE_LABEL(ev_qq_arg_splice)
    RESTORE(argl)
    RESTORE(env)
    RESTORE(unev)
    RESTORE(continu)
    ASSIGN(argl, OP(splice_arg, REG(val), REG(argl)))
    ASSIGN(unev, OP(rest_operands, REG(unev)))
    GOTO(LABEL(ev_qq_operand_loop))

MAKE_LABEL(ev_qq_arg_append)
    RESTORE(argl)
    RESTORE(env)
    RESTORE(unev)
    RESTORE(continu)
    ASSIGN(argl, OP(adjoin_arg, REG(val), REG(argl)))
    ASSIGN(unev, OP(rest_operands, REG(unev)))
    GOTO(LABEL(ev_qq_operand_loop))

MAKE_LABEL(ev_qq_arg_unquote_splicing)
    SAVE(continu)
    ASSIGN(continu, LABEL(ev_qq_arg_splice))
    GOTO(LABEL(en_qq_arg_dispatch))

MAKE_LABEL(ev_qq_arg_unquote)
    SAVE(continu)
    ASSIGN(continu, LABEL(ev_qq_arg_append))
    GOTO(LABEL(en_qq_arg_dispatch))

MAKE_LABEL(en_qq_arg_dispatch)
    SAVE(unev)
    SAVE(env)
    SAVE(argl)
    ASSIGN(exp, OP(cadr, REG(exp)))
    GOTO(LABEL(eval_dispatch))

MAKE_LABEL(ev_lambda)
    ASSIGN(unev, OP(lambda_parameters, REG(exp)))
    ASSIGN(exp, OP(lambda_body, REG(exp)))
    ASSIGN(val, OP(mk_procedure, REG(unev), REG(exp), REG(env)))
    GOTO(REG(continu))

MAKE_LABEL(ev_application)
    SAVE(continu)
    SAVE(env)
    ASSIGN(unev, OP(operands, REG(exp)))
    SAVE(unev)
    ASSIGN(exp, OP(operator_, REG(exp)))
    ASSIGN(continu, LABEL(ev_appl_did_operator))
    GOTO(LABEL(eval_dispatch))

MAKE_LABEL(ev_appl_did_operator)
    RESTORE(unev)
    RESTORE(env)
    ASSIGN(proc, REG(val))
    ASSIGN(argl, OP(empty_arglist))
    TEST(OP(has_no_operands, REG(unev)))
    BRANCH(LABEL(apply_dispatch))
    SAVE(proc)
    GOTO(LABEL(ev_appl_operand_loop))

MAKE_LABEL(ev_appl_operand_loop)
    SAVE(argl)
    ASSIGN(exp, OP(first_operand, REG(unev)))
    TEST(OP(is_last_operand, REG(unev)))
    BRANCH(LABEL(ev_appl_last_arg))
    SAVE(env)
    SAVE(unev)
    ASSIGN(continu, LABEL(ev_appl_accumulate_arg))
    GOTO(LABEL(eval_dispatch))

MAKE_LABEL(ev_appl_accumulate_arg)
    RESTORE(unev)
    RESTORE(env)
    RESTORE(argl)
    ASSIGN(argl, OP(adjoin_arg, REG(val), REG(argl)))
    ASSIGN(unev, OP(rest_operands, REG(unev)))
    GOTO(LABEL(ev_appl_operand_loop))

MAKE_LABEL(ev_appl_last_arg)
    ASSIGN(continu, LABEL(ev_appl_accum_last_arg))
    GOTO(LABEL(eval_dispatch))

MAKE_LABEL(ev_appl_accum_last_arg)
    RESTORE(argl)
    ASSIGN(argl, OP(adjoin_arg, REG(val), REG(argl)))
    RESTORE(proc)
    GOTO(LABEL(apply_dispatch))

MAKE_LABEL(apply_dispatch)
    TEST(OP(is_primitive_procedure, REG(proc)))
    BRANCH(LABEL(primitive_apply))
    TEST(OP(is_compound_procedure, REG(proc)))
    BRANCH(LABEL(compound_apply))
    GOTO(LABEL(unknown_procedure_type))

MAKE_LABEL(primitive_apply)
    ASSIGN(val, OP(apply_primitive_procedure, REG(proc), REG(argl)))
    RESTORE(continu)
    GOTO(REG(continu))

MAKE_LABEL(compound_apply)
    ASSIGN(unev, OP(procedure_parameters, REG(proc)))
    ASSIGN(env, OP(procedure_environment, REG(proc)))
    ASSIGN(env, OP(extend_environment, REG(unev), REG(argl), REG(env)))
    ASSIGN(unev, OP(procedure_body, REG(proc)))
    GOTO(LABEL(ev_sequence))

MAKE_LABEL(ev_begin)
    ASSIGN(unev, OP(begin_actions, REG(exp)))
    SAVE(continu)
    GOTO(LABEL(ev_sequence))

MAKE_LABEL(ev_sequence)
    ASSIGN(exp, OP(first_exp, REG(unev)))
    TEST(OP(is_last_exp, REG(unev)))
    BRANCH(LABEL(ev_sequence_last_exp))
    SAVE(unev)
    SAVE(env)
    ASSIGN(continu, LABEL(ev_sequence_continue))
    GOTO(LABEL(eval_dispatch))

MAKE_LABEL(ev_sequence_continue)
    RESTORE(env)
    RESTORE(unev)
    ASSIGN(unev, OP(rest_exps, REG(unev)))
    GOTO(LABEL(ev_sequence))

MAKE_LABEL(ev_sequence_last_exp)
    RESTORE(continu)
    GOTO(LABEL(eval_dispatch))

MAKE_LABEL(ev_if)
    SAVE(exp)
    SAVE(env)
    SAVE(continu)
    ASSIGN(continu, LABEL(ev_if_decide))
    ASSIGN(exp, OP(if_predicate, REG(exp)))
    GOTO(LABEL(eval_dispatch))

MAKE_LABEL(ev_if_decide)
    RESTORE(continu)
    RESTORE(env)
    RESTORE(exp)
    TEST(OP(is_false, REG(val)))
    BRANCH(LABEL(ev_if_alternative))
    GOTO(LABEL(ev_if_consequent))

MAKE_LABEL(ev_if_alternative)
    ASSIGN(exp, OP(if_alternative, REG(exp)))
    GOTO(LABEL(eval_dispatch))

MAKE_LABEL(ev_if_consequent)
    ASSIGN(exp, OP(if_consequent, REG(exp)))
    GOTO(LABEL(eval_dispatch))

MAKE_LABEL(ev_assignment)
    ASSIGN(unev, OP(assignment_variable, REG(exp)))
    SAVE(unev)
    ASSIGN(exp, OP(assignment_value, REG(exp)))
    SAVE(env)
    SAVE(continu)
    ASSIGN(continu, LABEL(ev_assignment_1))
    GOTO(LABEL(eval_dispatch))

MAKE_LABEL(ev_assignment_1)
    RESTORE(continu)
    RESTORE(env)
    RESTORE(unev)
    PERFORM(OP(environment_set, REG(env), REG(unev), REG(val)))
    ASSIGN(val, CONST(ok))
    GOTO(REG(continu))

MAKE_LABEL(ev_definition)
    ASSIGN(unev, OP(definition_variable, REG(exp)))
    SAVE(unev)
    ASSIGN(exp, OP(definition_value, REG(exp)))
    SAVE(env)
    SAVE(continu)
    ASSIGN(continu, LABEL(ev_definition_1))
    GOTO(LABEL(eval_dispatch))

MAKE_LABEL(ev_definition_1)
    RESTORE(continu)
    RESTORE(env)
    RESTORE(unev)
    PERFORM(OP(environment_define, REG(env), REG(unev), REG(val)))
    ASSIGN(val, CONST(ok))
    GOTO(REG(continu))

EXIT_INTERPRETER
}

value eval(value exp, value env)
{
    return interpret(exp, env);
}

} // namespace noldor

