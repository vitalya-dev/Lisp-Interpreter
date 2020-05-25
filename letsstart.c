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

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef struct lval{
  int type;
  long num;
  char* err;
  char* sym;

  int count;
  struct lval** cell;
} lval;

lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval* lval_err(char* e) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err  = malloc(strlen(e) + 1);
  strcpy(v->err, e);
  return v;
}

lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}


lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lval_del(lval* v) {
  switch (v->type) {
  case LVAL_NUM:
    break;
  case LVAL_ERR:
    free(v->err);
    break;
  case LVAL_SYM:
    free(v->sym);
    break;
  case LVAL_SEXPR:
    for (int i = 0; i < v->count; i++)
      lval_del(v->cell[i]);
    free(v->cell);
    break;
  }
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}


lval* lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {return lval_num(strtol(t->contents, NULL, 10));}
  if (strstr(t->tag, "symbol")) {return lval_sym(t->contents);}

  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr")) {x = lval_sexpr();}
  if (strcmp(t->tag, "expr|>") == 0) { return lval_read(t->children[0]); }


  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) {continue;}
    if (strcmp(t->children[i]->contents, ")") == 0) {continue;}
    if (strcmp(t->children[i]->contents, "{") == 0) {continue;}
    if (strcmp(t->children[i]->contents, "}") == 0) {continue;}
    if (strcmp(t->children[i]->contents, "regex") == 0) {continue;}
    x = lval_add(x, lval_read(t->children[i]));
  }
  return x;
}


void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);
    if (i != v->count - 1)
      putchar(' ');
  }
  putchar(close);
}

void lval_print(lval* v) {
  switch (v->type) {
  case LVAL_NUM:   printf("%li", v->num);        break;
  case LVAL_ERR:   printf("Error: %s", v->err);  break;
  case LVAL_SYM:   printf("%s", v->sym);         break;
  case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
  }
}

void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}


/* lval eval_expr(mpc_ast_t* t) { */
/*   if (strstr(t->tag, "number")) */
/*     return lval_num(atoi(t->contents)); */
/*   else if (strstr(t->tag, "sexpr")) { */
/*     if (t->children_num == 2) */
/*       return lval_num(0); */
/*     /\* ============================================= *\/ */
/*     lval result = eval_expr(t->children[2]); */
/*     if (result.type == LVAL_ERR) return result; */
/*     /\* ============================================= *\/ */
/*     for (int i = 3; i < t->children_num - 1; i++) { */
/*       lval j = eval_expr(t->children[i]); */
/*       /\* ============================================= *\/ */
/*       if (j.type == LVAL_ERR) return j; */
/*       /\* ============================================= *\/ */
/*       if (strcmp(t->children[1]->contents, "+") == 0) */
/*         result.num += j.num; */
/*       else if (strcmp(t->children[1]->contents, "-") == 0) */
/*         result.num -= j.num; */
/*       else if (strcmp(t->children[1]->contents, "*") == 0) */
/*         result.num *= j.num; */
/*       else if (strcmp(t->children[1]->contents, "/") == 0) { */
/*         /\* ============================================= *\/ */
/*         if (j.num == 0) return lval_err(LERR_DIV_ZERO); */
/*         /\* ============================================= *\/ */
/*         result.num /= j.num; */
/*       } */
/*       return result; */
/*     } */
/*   } else if (strstr(t->tag, "expr")) { */
/*     return eval_expr(t->children[0]); */
/*   } else */
/*     return lval_num(0); */
/* } */
    

/* lval eval(mpc_ast_t* t) { */
/*   return eval_expr(t->children[0]); */
/* } */
                
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
            "                                                        \
             number    : /-?[0-9]+/                                 ;\
             symbol    : '+'|'-'|'*'                                ;\
             sexpr     : '(' <expr>* ')'                            ;\
             expr      : <number> | <symbol> | <sexpr>              ;\
             lispy     : <expr>+                                    ;\
            ",
            number, symbol, sexpr, expr, lispy);

  puts("Lispy version 0.0.0.0.1");
  puts("Press ctrl-c to Exit\n");

  while (1) {
    char* input = readline("lispy> ");
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, lispy, &r)) {
      mpc_ast_print(r.output);
      /* ====================================== */
      /* lval* val = lval_read(((mpc_ast_t*)r.output)->children[0]); */
      lval* val = lval_read(r.output);
      lval_println(val);
      /* ====================================== */
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  mpc_cleanup(4, number, symbol, expr, sexpr, lispy);
  return 0;
}
