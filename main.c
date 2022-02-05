#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <editline/readline.h>
#include "mpc/mpc.h"


enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

typedef struct lval
{
    int type;
    long num;
    char* err;
    char* sym;
    int count;
    struct lval** cell;
} lval;

lval* lval_num(long x)
{
    lval* v = malloc(sizeof(lval));
    v -> type = LVAL_NUM;
    v -> num = x;
    return v;
}

lval* lval_err(char *m)
{
    lval* v = malloc(sizeof(lval));
    v -> type = LVAL_ERR;
    v -> err = malloc(strlen(m) + 1);
    strcpy(v -> err, m);
    return v;
}

lval* lval_sym(char* s)
{
    lval* v = malloc(sizeof(lval));
    v -> type = LVAL_SYM;
    v -> sym = malloc(strlen(s) + 1);
    strcpy(v -> sym, s);
    return v;
}

lval* lval_sexpr(void)
{
    lval* v = malloc(sizeof(lval));
    v -> type = LVAL_SEXPR;
    v -> count = 0;
    v -> cell = NULL;
    return v;
}

lval* lval_qexpr(void)
{
    lval* v = malloc(sizeof(lval));
    v -> type = LVAL_QEXPR;
    v -> count = 0;
    v -> cell = NULL;
    return v;
}

void lval_del(lval* v)
{
    switch (v -> type) {
        case LVAL_NUM:
            break;
        case LVAL_ERR:
            free(v -> err);
            break;
        case LVAL_SYM:
            free(v -> sym);
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for (int i = 0; i < v -> count; i++) {
                lval_del(v -> cell[i]);
            }
            free(v -> cell);
            break;
        default:
            puts("Error: Invalid lval type.");
    }
    free(v);
}


void lval_print_expr(char, lval*, char);

void lval_print(lval* v)
{
    switch (v -> type) {
        case LVAL_NUM:
            printf("%li", v -> num);
            break;
        case LVAL_ERR:
            printf("Error: %s", v -> err);
            break;
        case LVAL_SYM:
            printf("%s", v -> sym);
            break;
        case LVAL_SEXPR:
            lval_print_expr('(', v, ')');
            break;
        case LVAL_QEXPR:
            lval_print_expr('{', v, '}');
            break;
        default:
            puts("Error: Invalid lval type.");
    }
}

void lval_print_expr(char open_bkt, lval* v, char close_bkt)
{
    putchar(open_bkt);
    for (int i = 0; i < v -> count; i++) {
        lval_print(v -> cell[i]);
        if (i < (v -> count) - 1) {
            putchar(' ');
        }
    }
    putchar(close_bkt);
}

void lval_println(lval* v)
{
    lval_print(v);
    putchar('\n');
}


lval* lval_append(lval* v, lval* x)
{
    int end_idx = v -> count;
    v -> cell = realloc(v -> cell, sizeof(lval*) * (end_idx + 1));
    v -> cell[end_idx] = x;
    v -> count = end_idx + 1;
    return v;
}

lval* lval_pop(lval* v, int i)
{
    int count = v -> count;
    lval* x = v -> cell[i];

    memmove(&(v -> cell[i]), &(v -> cell[i + 1]), sizeof(lval*) * (count - i - 1));
    v -> cell = realloc(v -> cell, sizeof(lval*) * (count - 1));
    v -> count = count - 1;

    return x;
}

lval* lval_read(mpc_ast_t* t)
{
    lval* x = NULL;

    if (strstr(t -> tag, "number") != NULL) {
        errno = 0;
        long n = strtol(t -> contents, NULL, 10);
        x = (errno != ERANGE) ? lval_num(n) : lval_err("Invalid number.");
    }
    else if (strstr(t -> tag, "symbol") != NULL) {
        x = lval_sym(t -> contents);
    }
    else if (strstr(t -> tag, "qexpr") != NULL) {
        x = lval_qexpr();
    }
    else if (strcmp(t -> tag, ">") == 0 || strstr(t -> tag, "sexpr") != NULL) {
        x = lval_sexpr();
    }
    else {
        puts("Invalid abstract syntax tree.");
    }

    if (x != NULL) {
        mpc_ast_t** child = t -> children;

        for (int i = 0; i < t -> children_num; i++) {
            if (
                strcmp(child[i] -> tag, "regex") != 0 &&
                (strlen(child[i] -> contents) == 0 || strstr("(){}", child[i] -> contents) == NULL)
            ) {
                lval* v = lval_read(child[i]);
                x = lval_append(x, v);
            }
        }
    }

    return x;
}


enum { QEXPR_ERR_NARG, QEXPR_ERR_TYPE, QEXPR_ERR_ARG };

lval* qexpr_err(int err_type, char* fn_name)
{
    char err_msg[64];

    switch (err_type) {
        case QEXPR_ERR_NARG:
            snprintf(err_msg, 64, "Function '%s' received an invalid number of arguments.", fn_name);
            break;
        case QEXPR_ERR_TYPE:
            snprintf(err_msg, 64, "Function '%s' received an argument of invalid type.", fn_name);
            break;
        case QEXPR_ERR_ARG:
            snprintf(err_msg, 64, "Function '%s' received an invalid argument, {}.", fn_name);
            break;
        default:
            snprintf(err_msg, 64, "Invalid Q-expression error type.");
    }

    return lval_err(err_msg);
}

lval* lval_eval(lval*);

lval* builtin_list(lval* v)
{
    v -> type = LVAL_QEXPR;
    return v;
}

lval* builtin_head(lval* v)
{
    lval* x = NULL;

    if (v -> count != 1) {
        x = qexpr_err(QEXPR_ERR_NARG, "head");
    }
    else if (v -> cell[0] -> type != LVAL_QEXPR) {
        x = qexpr_err(QEXPR_ERR_TYPE, "head");
    }
    else if (v -> cell[0] -> count == 0) {
        x = qexpr_err(QEXPR_ERR_ARG, "head");
    }
    else {
        lval* arg = lval_pop(v, 0);
        x = lval_qexpr();
        x = lval_append(x, lval_pop(arg, 0));
        lval_del(arg);
    }

    return x;
}

lval* builtin_tail(lval* v)
{
    lval* x = NULL;

    if (v -> count != 1) {
        x = qexpr_err(QEXPR_ERR_NARG, "tail");
    }
    else if (v -> cell[0] -> type != LVAL_QEXPR) {
        x = qexpr_err(QEXPR_ERR_TYPE, "tail");
    }
    else if (v -> cell[0] -> count == 0) {
        x = qexpr_err(QEXPR_ERR_ARG, "tail");
    }
    else {
        x = lval_pop(v, 0);
        lval* head = lval_pop(x, 0);
        lval_del(head);
    }

    return x;
}

lval* builtin_eval(lval* v)
{
    lval* x = NULL;

    if (v -> count != 1) {
        x = qexpr_err(QEXPR_ERR_NARG, "eval");
    }
    else if (v -> cell[0] -> type != LVAL_QEXPR) {
        x = qexpr_err(QEXPR_ERR_TYPE, "eval");
    }
    else {
        lval* arg = lval_pop(v, 0);
        arg -> type = LVAL_SEXPR;
        x = lval_eval(arg);
    }

    return x;
}

lval* builtin_join(lval* v)
{
    lval* x = NULL;

    for (int i = 0; i < v -> count; i++) {
        if (v -> cell[i] -> type != LVAL_QEXPR) {
            x = qexpr_err(QEXPR_ERR_TYPE, "join");
            break;
        }
    }

    if (x == NULL) {
        x = lval_pop(v, 0);

        while (v -> count > 0) {
            lval* y = lval_pop(v, 0);

            while (y -> count > 0) {
                lval* e = lval_pop(y, 0);
                x = lval_append(x, e);
            }

            lval_del(y);
        }
    }

    return x;
}

lval* builtin_op(lval* v, char* op)
{
    lval* x = lval_pop(v, 0);

    if (x -> type != LVAL_NUM) {
        lval_del(x);
        return lval_err("Cannot operate on non-number.");
    }

    if (strcmp(op, "-") == 0 && v -> count == 0) {
        x -> num = -(x -> num);
    }
    else {
        while (v -> count > 0 && x -> type != LVAL_ERR) {
            lval* y = lval_pop(v, 0);

            if (y -> type != LVAL_NUM) {
                lval_del(x);
                x = lval_err("Cannot operate on non-number.");
            }

            if (strcmp(op, "+") == 0) {
                x -> num += y -> num;
            }
            else if (strcmp(op, "-") == 0) {
                x -> num -= y -> num;
            }
            else if (strcmp(op, "*") == 0) {
                x -> num *= y -> num;
            }
            else if (strcmp(op, "/") == 0) {
                if (y -> num == 0) {
                    lval_del(x);
                    x = lval_err("Division by zero.");
                }
                else {
                    x -> num /= y -> num;
                }
            }
            else {
                lval_del(x);
                x = lval_err("Invalid symbol.");
            }

            lval_del(y);
        }
    }

    return x;
}

lval* builtin(lval* v, char* fn_name)
{
    if (strcmp("list", fn_name) == 0) {
        return builtin_list(v);
    }
    else if (strcmp("head", fn_name) == 0) {
        return builtin_head(v);
    }
    else if (strcmp("tail", fn_name) == 0) {
        return builtin_tail(v);
    }
    else if (strcmp("join", fn_name) == 0) {
        return builtin_join(v);
    }
    else if (strcmp("eval", fn_name) == 0) {
        return builtin_eval(v);
    }
    else if (strstr("+-*/", fn_name) != NULL) {
        lval* result = builtin_op(v, fn_name);
        lval_del(v);
        return result;
    }
    else {
        lval_del(v);
        return lval_err("Unknown function.");
    }
}


lval* lval_eval(lval* v)
{
    // Not s-expr or empty s-expr
    if (v -> type != LVAL_SEXPR || v -> count == 0) {
        return v;
    }

    // Error checking
    for (int i = 0; i < v -> count; i++) {
        v -> cell[i] = lval_eval(v -> cell[i]);

        if (v -> cell[i] -> type == LVAL_ERR) {
            lval* err = lval_pop(v, i);
            lval_del(v);
            return err;
        }
    }

    lval* result = NULL;

    if (v -> count == 1) { // Single expression
        result = lval_pop(v, 0);
        lval_del(v);
    }
    else if (v -> cell[0] -> type == LVAL_SYM) { // Check for symbol
        lval* f = lval_pop(v, 0);
        result = builtin(v, f -> sym);
        lval_del(f);
    }
    else {
        result = lval_err("An S-expression must start with a symbol.");
        lval_del(v);
    }

    return result;
}


int main(int argc, char **argv)
{
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Symbol = mpc_new("symbol");
    mpc_parser_t *Sexpr = mpc_new("sexpr");
    mpc_parser_t *Qexpr = mpc_new("qexpr");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    mpca_lang(
        MPCA_LANG_DEFAULT,
        "                                                     \
        number : /-?[0-9]+/ ;                                 \
        symbol : \"list\" | \"head\" | \"tail\"               \
               | \"join\" | \"eval\" | '+' | '-' | '*' | '/'; \
        sexpr  : '(' <expr>* ')' ;                            \
        qexpr  : '{' <expr>* '}' ;                            \
        expr   : <number> | <symbol> | <sexpr> | <qexpr>;     \
        lispy  : /^/ <expr>* /$/ ;                            \
        ",
        Number, Symbol, Sexpr, Qexpr, Expr, Lispy
    );

    puts("Lispy Version 0.1");
    puts("Press Ctrl + c to Exit\n");

    while (true) {
        char *input = readline("lispy> ");
        if (input == NULL) {
            puts("");
            break;
        }
        add_history(input);

        mpc_result_t r;

        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval* x = lval_read(r.output);
            lval* result = lval_eval(x);
            lval_println(result);
            lval_del(result);
            mpc_ast_delete(r.output);
        }
        else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    return 0;
}
