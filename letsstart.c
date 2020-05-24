#include <stdio.h>
#include "mpc.h"

static char buffer[2048];

#ifdef _WIN32
char* readline(char* promt) {
  fputs("lispy> ", stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = (char*)malloc(strlen(buffer) + 1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy) - 1] = '\0';
  return cpy;
}

void add_history(char* unused) {

}

#endif

typedef struct {
  int type;
  long num;
  int err;
} lval;

enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

lval lval_err(long x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

void lval_print(lval v) {
  switch (v.type) {
  case LVAL_NUM:
    printf("%li", v.num);
    break;
  case LVAL_ERR:
    if (v.err == LERR_DIV_ZERO) printf("Error: Division By Zero!");
    if (v.err == LERR_BAD_NUM)  printf("Error: Invalid Number!");
    if (v.err == LERR_BAD_OP)   printf("Error:  Invalid Operator!");
  }
}

void lval_println(lval v) {
  lval_print(v);
  putchar('\n');
}


lval eval_expr(mpc_ast_t* t) {
  if (strstr(t->tag, "number"))
    return lval_num(atoi(t->contents));
  else if (strstr(t->tag, "expr")) {
    /* ============================================= */
    lval result = eval_expr(t->children[2]);
    if (result.type == LVAL_ERR) return result;
    /* ============================================= */
    for (int i = 3; i < t->children_num - 1; i++) {
      lval j = eval_expr(t->children[i]);
      /* ============================================= */
      if (j.type == LVAL_ERR) return j;
      /* ============================================= */
      if (strcmp(t->children[1]->contents, "+") == 0)
        result.num += j.num;
      if (strcmp(t->children[1]->contents, "-") == 0)
        result.num -= j.num;
      if (strcmp(t->children[1]->contents, "*") == 0)
        result.num *= j.num;
      if (strcmp(t->children[1]->contents, "/") == 0) {
        /* ============================================= */
        if (j.num == 0) return lval_err(LERR_DIV_ZERO);
        /* ============================================= */
        result.num /= j.num;
      }
    }
    return result;
  }
  return lval_num(0);
}
    

lval eval(mpc_ast_t* t) {
  return eval_expr(t->children[0]);
}
                
/*   if (strstr(t->tag, "number")) return atoi(t->contents); */

/*   char* op  = t->children[1]->contents; */

/*   long x = eval(t->children[2]); */

/*   int i = 3; */
/*   while (strstr(t->children[i]->tag, "expr")) */
/* } */



int main(int argc, char** argv) {
  mpc_parser_t* number   =  mpc_new("number");
  mpc_parser_t* symbol   =  mpc_new("symbol");
  mpc_parser_t* sexpr    =  mpc_new("sexpr");
  mpc_parser_t* expr     =  mpc_new("expr");
  mpc_parser_t* lispy    =  mpc_new("lispy");
  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                       ;\
             number    : /-?[0-9]+/                                 ;\
             symbol    : '+'|'-'|'*'                                ;\
             sexpr     : '(' <expr>* ')'                            ;\
             expr      : <number> | <symbol> | <sexpr>              ;\
             lispy     : <expr>+                             ;\
            ",
            number, symbol, sexpr, expr, lispy);

  puts("Lispy version 0.0.0.0.1");
  puts("Press ctrl-c to Exit\n");

  while (1) {
    char* input = readline("lispy> ");
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, expr, &r)) {
      mpc_ast_print(r.output);
      //      lval_println(eval(r.output));
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  mpc_cleanup(4, number, symbol, expr, sexpr, lispy);
  return 0;
}
