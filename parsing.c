#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <editline/readline.h>
#include "mpc.h"


long eval(mpc_ast_t *node)
{
    // If node is not a number
    if (strstr(node->tag, "number") == NULL) {
        mpc_ast_t **children = node->children;

        char *op = children[1]->contents;
        long x = eval(children[2]);

        for (int i = 3; strstr(children[i]->tag, "expr"); i++)
        {
            long y = eval(children[i]);
            switch (op[0]) {
                case '+':
                    x = x + y;
                    break;
                case '-':
                    x = x - y;
                    break;
                case '*':
                    x = x * y;
                    break;
                case '/':
                    x = x / y;
                    break;
                default:
                    return -1;
            }
        }
        return x;
    }
    else { // node is a number, return the number
        return atoi(node->contents);
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
        else
            add_history(input);

        mpc_result_t r;

        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            // mpc_ast_print(r.output);
            long result = eval(r.output);
            printf("%li\n", result);
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
