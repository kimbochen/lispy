#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <editline/readline.h>
#include "mpc/mpc.h"


typedef struct
{
    int type;
    long num;
    int err;
} lval;

enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };


lval create_lval_num(long num_)
{
    lval v;
    v.type = LVAL_NUM;
    v.num = num_;
    return v;
}

lval create_lval_err(int err_)
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


lval eval_op(char *op, lval lval_x, lval lval_y)
{
    if (lval_x.type == LVAL_ERR) return lval_x;
    if (lval_y.type == LVAL_ERR) return lval_y;

    long x = lval_x.num;
    long y = lval_y.num;

    switch (op[0]) {
        case '+':
            return create_lval_num(x + y);
        case '-':
            return create_lval_num(x - y);
        case '*':
            return create_lval_num(x * y);
        case '/':
            return (y == 0) ? create_lval_err(LERR_DIV_ZERO) : create_lval_num(x / y);
        default:
            return create_lval_err(LERR_BAD_OP);
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
        return (errno == ERANGE) ?
            create_lval_err(LERR_BAD_NUM) : create_lval_num(x);
    }
}


int main(int argc, char **argv)
{
    // Parsers
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Operator = mpc_new("operator");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    // Define the language
    mpca_lang(
        MPCA_LANG_DEFAULT,
        "                                                 \
        number   : /-?[0-9]+/;                            \
        operator : '+' | '-' | '*' | '/';                 \
        expr     : <number> | '(' <operator> <expr>+ ')'; \
        lispy    : /^/ <operator> <expr>+ /$/;            \
        ",
        Number, Operator, Expr, Lispy
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
    mpc_cleanup(4, Number, Operator, Expr, Lispy);

    return 0;
}
