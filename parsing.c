#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <editline/readline.h>
#include "mpc/mpc.h"


enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

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


lval* lval_append(lval* v, lval* x)
{
    int end_idx = v -> count;
    v -> cell = realloc(v -> cell, sizeof(lval*) * (end_idx + 1));
    v -> cell[end_idx] = x;
    v -> count = end_idx + 1;
    return v;
}

lval* lval_read(mpc_ast_t* t)
{
    lval* x = NULL;

    if (strstr(t -> tag, "number")) {
        errno = 0;
        long n = strtol(t -> contents, NULL, 10);
        x = (errno != ERANGE) ? lval_num(n) : lval_err("Invalid number");
    }
    else if (strstr(t -> tag, "symbol")) {
        x = lval_sym(t -> contents);
    }
    else if (strcmp(t -> tag, ">") == 0 || strstr(t -> tag, "sexpr")) {
        x = lval_sexpr();

        for (int i = 0; i < t -> children_num; i++) {
            if (
                strcmp(t -> children[i] -> contents, "(") != 0 &&
                strcmp(t -> children[i] -> contents, ")") != 0 &&
                strcmp(t -> children[i] -> tag, "regex") != 0
            ) {
                lval* v = lval_read(t -> children[i]);
                x = lval_append(x, v);
            }
        }
    }
    else {
        puts("Invalid abstract syntax tree.");
    }

    return x;
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

lval* builtin_op(lval* v, char* op)
{
    lval* x = lval_pop(v, 0);

    if (x -> type != LVAL_NUM) {
        lval_del(x);
        return lval_err("Cannot operate on non-number");
    }

    if (strcmp(op, "-") == 0 && v -> count == 0) {
        x -> num = -(x -> num);
    }
    else {
        while (v -> count > 0 && x -> type != LVAL_ERR) {
            lval* y = lval_pop(v, 0);

            if (y -> type != LVAL_NUM) {
                lval_del(x);
                x = lval_err("Cannot operate on non-number");
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
                    x = lval_err("Division by zero");
                }
                else {
                    x -> num /= y -> num;
                }
            }
            else {
                lval_del(x);
                x = lval_err("Invalid symbol");
            }

            lval_del(y);
        }
    }

    return x;
}

lval* lval_eval(lval* v)
{
    if (v -> type == LVAL_SEXPR && v -> count > 0) {
        lval* result = NULL;

        for (int i = 0; i < v -> count; i++) {
            v -> cell[i] = lval_eval(v -> cell[i]);

            if (v -> cell[i] -> type == LVAL_ERR) {
                result = lval_pop(v, i);
                lval_del(v);
                return result;
            }
        }

        if (v -> count == 1) {
            result = lval_pop(v, 0);
        }
        else if (v -> cell[0] -> type == LVAL_SYM) {
            lval* f = lval_pop(v, 0);
            result = builtin_op(v, f -> sym);
            lval_del(f);
        }
        else {
            result = lval_err("S-expression must start with a symbol");
        }

        lval_del(v);

        return result;
    }
    else {
        return v;
    }
}


void lval_print(lval* v)
{
    switch (v -> type) {
        case LVAL_NUM:
            printf("%li", v -> num);
            break;
        case LVAL_ERR:
            printf("Error: %s.", v -> err);
            break;
        case LVAL_SYM:
            printf("%s", v -> sym);
            break;
        case LVAL_SEXPR:
            putchar('(');
            for (int i = 0; i < v -> count; i++) {
                lval_print(v -> cell[i]);
                if (i < (v -> count) - 1) {
                    putchar(' ');
                }
            }
            putchar(')');
            break;
        default:
            puts("Error: Invalid lval type.");
    }
}

void lval_println(lval* v)
{
    lval_print(v);
    putchar('\n');
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

    // Cleanup parsers
    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

    return 0;
}
