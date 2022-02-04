#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <editline/readline.h>
#include "mpc/mpc.h"


typedef struct
{
    int type;
    union {
        long num;
        int err;
    };
} lval;

enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };


lval lval_num(long num_)
{
    lval v;
    v.type = LVAL_NUM;
    v.num = num_;
    return v;
}

lval lval_err(int err_)
{
    lval v;
    v.type = LVAL_ERR;
    v.err = err_;
    return v;
}


void lval_print(lval v)
{
    switch (v.type) {
        case LVAL_NUM:
            printf("%li\n", v.num);
            break;
        case LVAL_ERR:
            switch (v.err) {
                case LERR_DIV_ZERO:
                    puts("Error: Division by zero.");
                    break;
                case LERR_BAD_OP:
                    puts("Error: Invalid operator.");
                    break;
                case LERR_BAD_NUM:
                    puts("Error: Invalid number.");
                    break;
                default:
                    puts("Error: Invalid error type.");
            }
            break;
        default:
            puts("Error: Invalid Lisp Value.");
    }
}


lval eval_op(char *op, lval x, lval y)
{
    if (x.type == LVAL_ERR) return x;
    if (y.type == LVAL_ERR) return y;

    long nx = x.num;
    long ny = y.num;

    switch (op[0]) {
        case '+':
            return lval_num(nx + ny);
        case '-':
            return lval_num(nx - ny);
        case '*':
            return lval_num(nx * ny);
        case '/':
            return (ny == 0) ? lval_err(LERR_DIV_ZERO) : lval_num(nx / ny);
        default:
            return lval_err(LERR_BAD_OP);
    }
}

lval eval(mpc_ast_t *node)
{
    // If node is not a number
    if (strstr(node->tag, "number") == NULL) {
        mpc_ast_t **children = node->children;

        char *op = children[1]->contents;
        lval x = eval(children[2]);

        for (int i = 3; strstr(children[i]->tag, "expr"); i++)
        {
            lval y = eval(children[i]);
            x = eval_op(op, x, y);
        }
        return x;
    }
    else { // node is a number, return the number
        errno = 0;
        long x = strtol(node->contents, NULL, 10);
        return (errno == ERANGE) ? lval_err(LERR_BAD_NUM) : lval_num(x);
    }
}


int main(int argc, char **argv)
{
    // Parsers
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Symbol = mpc_new("symbol");
    mpc_parser_t *Sexpr = mpc_new("sexpr");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    // Define the language
    mpca_lang(
        MPCA_LANG_DEFAULT,
        "                                        \
        number : /-?[0-9]+/ ;                    \
        symbol : '+' | '-' | '*' | '/' ;         \
        sexpr  : '(' <expr>* ')' ;               \
        expr   : <number> | <symbol> | <sexpr> ; \
        lispy  : /^/ <expr>* /$/ ;               \
        ",
        Number, Symbol, Sexpr, Expr, Lispy
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
            lval result = eval(r.output);
            lval_print(result);
            mpc_ast_delete(r.output);
        }
        else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    // Cleanup parsers
    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

    return 0;
}
