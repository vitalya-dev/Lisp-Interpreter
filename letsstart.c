#include <stdio.h>
#include "mpc.h"


#define LASSERT(args, cond, err) if (!(cond)) {lval_del(args); return lval_err(err);}

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

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR};
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef struct lval {
  int type;
  long num;
  char* err;
  char* sym;

  int count;
  struct lval** cell;
} lval;

lval* lval_join(lval* x, lval* y);
lval* lval_eval_sexpr(lval* v);
lval* lval_eval(lval* v);

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

lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
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
  case LVAL_QEXPR:
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
  if (strcmp(t->tag, "expr|>") == 0) { return lval_read(t->children[0]); }
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr")) {x = lval_sexpr();}
  if (strstr(t->tag, "qexpr")) {x = lval_qexpr();}


  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) {continue;}
    if (strcmp(t->children[i]->contents, ")") == 0) {continue;}
    if (strcmp(t->children[i]->contents, "`") == 0) {continue;}
    if (strcmp(t->children[i]->contents, "{") == 0) {continue;}
    if (strcmp(t->children[i]->contents, "}") == 0) {continue;}
    if (strcmp(t->children[i]->contents, "regex") == 0) {continue;}
    x = lval_add(x, lval_read(t->children[i]));
  }
  return x;
}




void lval_print(lval* v);

void lval_expr_print(lval* v, char* open, char* close) {
  printf("%s", open);
  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);
    if (i != v->count - 1)
      putchar(' ');
  }
  printf("%s", close);
}

void lval_print(lval* v) {
  switch (v->type) {
  case LVAL_NUM:   printf("%li", v->num);        break;
  case LVAL_ERR:   printf("Error: %s", v->err);  break;
  case LVAL_SYM:   printf("%s", v->sym);         break;
  case LVAL_SEXPR: lval_expr_print(v, "(", ")"); break;
  case LVAL_QEXPR: lval_expr_print(v, "`(", ")"); break;
  }
}

void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}

lval* lval_pop(lval* v, int i) {
  lval* x = v->cell[i];
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count - 1 - i));
  v->count--;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}



lval* builtin_head(lval* v) {
  LASSERT(v, v->count == 1, "Tail: to many arguments");
  LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Tail: incorect types");
  LASSERT(v, v->cell[0]->count > 0, "Tail: empty list");
  /* =================================================== */
  lval* a = lval_take(v, 0);
  /* =================================================== */
  while (a->count > 1) { lval_del(lval_pop(a, 1)); }
  /* =================================================== */
  return a;
}

lval* builtin_tail(lval* v) {
  LASSERT(v, v->count == 1, "Tail: to many arguments");
  LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Tail: incorect types");
  LASSERT(v, v->cell[0]->count > 0, "Tail: empty list");
  /* =================================================== */
  lval* a = lval_take(v, 0);
  /* =================================================== */
  lval_del(lval_pop(a, 0));
  /* =================================================== */
  return a;
}

lval* builtin_list(lval* v) {
  v->type = LVAL_QEXPR;
  return v;
}

lval* builtin_join(lval* v) {
  for (int i = 0; i < v->count; i++) {
    LASSERT(v, v->cell[i]->type == LVAL_QEXPR, "Join: incorect types");
  }

  lval* a = lval_pop(v, 0);
  while (v->count > 0) {
    a = lval_join(a, lval_pop(v, 0));
  }

  return NULL;
}



lval* builtin_eval(lval* v) {
  LASSERT(v, v->count == 1, "Eval: to many arguments");
  LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Eval: incorect types");
  LASSERT(v, v->cell[0]->count > 0, "Eval: empty list");
  
  lval* a = lval_take(v, 0);
  a->type = LVAL_SEXPR;
  return lval_eval(a);
}

lval* builtin_op(lval* v, char* op) {
  /* =================================================== */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type != LVAL_NUM) {
      lval_del(v);
      return lval_err("Can't operate on non number!");
    }
  }
  /* =================================================== */
  lval* a = lval_pop(v, 0);
  /* =================================================== */
  if ((strcmp(op, "-") == 0) && v->count == 0) {
    a->num *= -1;
  }
  /* =================================================== */
  while (v->count > 0) {
    lval* b = lval_pop(v, 0);
    /* ============================================= */
    if (strcmp(op, "+") == 0) { a->num += b->num; }
    if (strcmp(op, "-") == 0) { a->num -= b->num; }
    if (strcmp(op, "*") == 0) { a->num *= b->num; }
    if (strcmp(op, "/") == 0) {
      if (b->num == 0) {
        lval_del(a); lval_del(b); lval_del(v);
        a = lval_err("Division By Zero");
      } else {
        a->num = a->num / b->num;
      }
    }
    /* ============================================= */
    lval_del(b);
  }
  /* =================================================== */
  lval_del(v);
  /* =================================================== */
  return a;
  /* =================================================== */
}


lval* builtin(lval* a, char* func) {
  /* =================================================== */
  if (strcmp(func, "list") == 0) { return builtin_list(a); }
  if (strcmp(func, "head") == 0) { return builtin_head(a); }
  if (strcmp(func, "tail") == 0) { return builtin_tail(a); }
  if (strcmp(func, "join") == 0) { return builtin_join(a); }
  if (strcmp(func, "eval") == 0) { return builtin_eval(a); }
  if (strstr("+-/*", func))      { return builtin_op(a, func);   }
  /* =================================================== */
  lval_del(a);
  /* =================================================== */
  return lval_err("Unknown op");
}



lval* lval_join(lval* x, lval* y) {
  /* ======================================== */
  while (y->count > 0) {
    x = lval_add(x, lval_pop(y, 0));
  }
  /* ======================================== */
  lval_del(y);
  /* ======================================== */
  return x;
}

lval* lval_eval(lval* v) {
  /* ======================================== */
  if (v->type == LVAL_SEXPR)
    return lval_eval_sexpr(v);
  /* ======================================== */
  return v;
}

lval* lval_eval_sexpr(lval* v) {
  /* ======================================== */
  for (int i = 0; i < v->count; i++)
    v->cell[i] = lval_eval(v->cell[i]);
  /* ======================================== */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR)
      return lval_take(v, i);
  }
  /* ======================================== */
  if (v->count == 0)
    return v;
  /* ======================================== */
  if (v->count == 1)
    return lval_take(v, 0);
  /* ======================================== */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f), lval_del(v);
    return lval_err("Symbol Expected");
  }
  /* ======================================== */
  lval* result = builtin(v, f->sym);
  /* ======================================== */
  lval_del(f);
  /* ======================================== */
  return result;
}



int main(int argc, char** argv) {
  mpc_parser_t* number   =  mpc_new("number");
  mpc_parser_t* symbol   =  mpc_new("symbol");
  mpc_parser_t* sexpr    =  mpc_new("sexpr");
  mpc_parser_t* qexpr    =  mpc_new("qexpr");
  mpc_parser_t* expr     =  mpc_new("expr");
  mpc_parser_t* lispy    =  mpc_new("lispy");
  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                        \
             number    : /-?[0-9]+/                                 ;\
             symbol    : '+'|'-'|'*'|'/' |\"list\"| \"head\" | \"tail\" | \"join\" | \"eval\"    ;\
             sexpr     : '(' <expr>* ')'                            ;\
             qexpr     : '`' '(' <expr>* ')'                            ;\
             expr      : <number> | <symbol> | <sexpr> | <qexpr>    ;\
             lispy     : <expr>+                                    ;\
            ",
            number, symbol, sexpr, qexpr, expr, lispy);

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
      lval* val = (lval_eval(lval_read(r.output)));
      lval_println(val);
      /* ====================================== */
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  mpc_cleanup(4, number, symbol, expr, sexpr, qexpr, lispy);
  return 0;
}
