#include <stdio.h>
#include "mpc.h"


#define LASSERT(args, cond, ...)         \
  if (!(cond)) {                         \
    lval* err = lval_err(__VA_ARGS__);   \
    lval_del(args);                      \
    return err;                          \
  }                                      \

static char buffer[2048];

#ifdef _WIN32
char* readline(char* promt) {
  fputs("lispy> ", stdout);
  if (fgets(buffer, 2048, stdin) == NULL)
    return NULL;
  char* cpy = (char*)malloc(strlen(buffer) + 1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy) - 1] = '\0';
  return cpy;
}

void add_history(char* unused) {

}

#endif

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_STR, LVAL_SEXPR, LVAL_QEXPR, LVAL_FN};
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

typedef lval* (*lbuiltin)(lenv*, lval*);

lenv* lenv_copy(lenv* v);
/* ========================================== */
lval* lval_println(lval* v);
lval* lval_join(lval* x, lval* y);
lval* lval_eval_sexpr(lenv* e, lval* v);
lval* lval_eval(lenv* e, lval* v);
lval* lval_copy(lval* v);
void  lval_del(lval* v);
lval* lval_sym(char* s);
lval* lval_num(long x);
lval* lval_err(char* fmt, ...);
lval* lval_builtin(lbuiltin f);
lval* lval_lambda(lval* formals, lval* body);
/* ========================================== */
lval* builtin_eval(lenv* e, lval* v);
lval* builtin_list(lenv* e, lval* v);
lval* builtin_head(lenv* e, lval* v);
lval* builtin_tail(lenv* e, lval* v);
lval* builtin_join(lenv* e, lval* v);
lval* builtin_def(lenv* e, lval* a);
lval* builtin_add(lenv* e, lval* v);
lval* builtin_sub(lenv* e, lval* v);
lval* builtin_mul(lenv* e, lval* v);
lval* builtin_div(lenv* e, lval* v);
lval* builtin_puts(lenv* e, lval* v);
lval* builtin_lambda(lenv* e, lval* v);
/* ========================================== */
lval* builtin_op(lval* v, char* o);

struct lval {
  int type;
  /* ==================================*/
  long num;
  char* err;
  char* sym;
  char* str;
  /* ==================================*/
  lbuiltin builtin;
  lenv* env;
  lval* formals;
  lval* body;
  /* ==================================*/
  int count;
  lval** cell;
}; 

struct lenv {
  int count;
  char** syms;
  lval** vals;
};


char* ltype_name(int t) {
  printf("%d\n", t);
  switch (t) {
    case LVAL_NUM:
      return "Number";
    case LVAL_ERR:
      return "Function";
    case LVAL_SEXPR:
      return "S-Expression";
    case LVAL_QEXPR:
      return "Q-Expression";
    case LVAL_FN:
      return "Function";
    case LVAL_SYM:
      return "Symbol";
    default:
      return "Unknown type";
  }
}


lenv* lenv_new() {
  lenv* e = malloc(sizeof(lenv));
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

void lenv_del(lenv* e) {
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
}

lenv* lenv_copy(lenv* e) {
  return lenv_new();
}

lval* lenv_get(lenv* e, lval* k) { 
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }
  return lval_err("Unbound symbol:%s!", k->sym);
}

lval* lenv_put(lenv* e, lval* k, lval* v) {
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }
  /* ========================================== */
  e->count++;
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);
  /* ========================================== */
  e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
  strcpy(e->syms[e->count - 1], k->sym);
  /* ========================================== */
  e->vals[e->count - 1] = lval_copy(v);
  /* ========================================== */
  return e->vals[e->count - 1];
}

lenv* lenv_add_builtin(lenv* e, char* name, lbuiltin f) { 
  lval* k = lval_sym(name);
  lval* v = lval_builtin(f);
  lenv_put(e, k, v);
  /* ========================================== */
  lval_del(k);
  lval_del(v);
  /* ========================================== */
  return e;
}

lenv* lenv_add_builtins(lenv* e) {
  lenv_add_builtin(e, "list",   builtin_list);
  lenv_add_builtin(e, "head",   builtin_head);
  lenv_add_builtin(e, "eval",   builtin_eval);
  lenv_add_builtin(e, "puts",   builtin_puts);
  lenv_add_builtin(e, "tail",   builtin_tail);
  lenv_add_builtin(e, "join",   builtin_join);
  lenv_add_builtin(e, "def",    builtin_def);
  lenv_add_builtin(e, "lambda", builtin_lambda);
  lenv_add_builtin(e, "+",      builtin_add);
  lenv_add_builtin(e, "-",      builtin_sub);
  lenv_add_builtin(e, "*",      builtin_mul);
  lenv_add_builtin(e, "/",      builtin_div);
  return e;
}


lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval* lval_err(char* fmt, ...) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  /* =========================================== */
  va_list va;
  va_start(va, fmt);
  /* =========================================== */
  v->err  = malloc(512);
  vsnprintf(v->err, 511, fmt, va);
  v->err = realloc(v->err, strlen(v->err) + 1);
  /* =========================================== */
  va_end(va);
  /* =========================================== */
  return v;
}

lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_str(char* s) { 
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_STR;
  /* ========================================== */
  v->str = malloc(strlen(s) - 2 + 1);
  strncpy(v->str, s + 1, strlen(s) - 2);
  v->str[strlen(s) - 2] = '\0';
  /* ========================================== */
  return v;
}


lval* lval_builtin(lbuiltin f) { 
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FN;
  v->builtin = f;
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


lval* lval_lambda(lval* formals, lval* body) { 
  lval* v = malloc(sizeof(lval));
  /* ======================================= */
  v->type = LVAL_FN;
  v->builtin = NULL;
  /* ======================================= */
  v->env = lenv_new();
  v->formals = formals;
  v->body = body;
  /* ======================================= */
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
    case LVAL_FN:
      if (v->builtin == NULL) {
        lenv_del(v->env);
        lval_del(v->formals);
        lval_del(v->body);
      }
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
  if (strstr(t->tag, "string")) {return lval_str(t->contents);}

  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { return lval_read(t->children[0]); }
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
  if (v == NULL)
    return;
  switch (v->type) {
    case LVAL_NUM:   printf("%li", v->num);         break;
    case LVAL_ERR:   printf("Error: %s", v->err);   break;
    case LVAL_SYM:   printf("%s", v->sym);          break;
    case LVAL_STR:   printf("'%s'", v->str);          break;
    case LVAL_SEXPR: lval_expr_print(v, "(", ")");  break;
    case LVAL_QEXPR: lval_expr_print(v, "`(", ")"); break;
    case LVAL_FN:
      if (v->builtin) printf("<FN>");
      else { printf("Lambda "); lval_print(v->formals); putchar(' '); lval_print(v->body); }
      break;
  }
}

lval* lval_println(lval* v) {
  if (v == NULL)
    return;
  lval_print(v);
  putchar('\n');
  return v;
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

lval* lval_eval(lenv* e, lval* v) {
  if (v == NULL)
    return NULL;
  /* ======================================== */
  if (v->type == LVAL_SEXPR)
    return lval_eval_sexpr(e, v);
  /* ======================================== */
  if (v->type == LVAL_SYM) {
    return lenv_get(e, v);
  }
  /* ======================================== */
  return v;
}

lval* lval_eval_sexpr(lenv* e, lval* v) { 
  /* ======================================== */
  for (int i = 0; i < v->count; i++)
    v->cell[i] = lval_eval(e, v->cell[i]);
  /* ======================================== */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR)
      return lval_take(v, i);
  }
  /* ======================================== */
  if (v->count == 0)
    return v;
  /* ======================================== */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_FN) {
    lval_del(f), lval_del(v);
    return lval_err("Function Expected");
  }
  /* ======================================== */
  lval* result = NULL;
  if (f->builtin)
    result = f->builtin(e, v);
  else {
    result = builtin_eval(e, lval_add(lval_sexpr(), lval_copy(f->body)));
  }
  /* ======================================== */
  lval_del(f);
  /* ======================================== */
  return result;
}

lval* lval_copy(lval* v) {
  lval* x = malloc(sizeof(lval));
  x->type = v->type;
  switch (v->type) {
    case LVAL_NUM:   x->num  = v->num;         break;
    case LVAL_FN:
      x->builtin = v->builtin;
      if (x->builtin == NULL) {
        x->env = lenv_copy(v->env);
        x->formals = lval_copy(v->formals);
        x->body = lval_copy(v->body);
      }
      break;
    case LVAL_ERR:   x->err  = malloc(strlen(v->err + 1)); strcpy(x->err, v->err);    break;
    case LVAL_SYM:   x->sym  = malloc(strlen(v->sym + 1)); strcpy(x->sym, v->sym);    break;
    case LVAL_SEXPR:
    case LVAL_QEXPR:
      x->cell = malloc(sizeof(lval*) * v->count);
      x->count = v->count;
      for (int i = 0; i < v->count; i++) {
        x->cell[i] = lval_copy(v->cell[i]);
      }
      break;
  }
  return x;
}



lval* builtin_puts(lenv* e, lval* v) {
  /* ================================================= */
  LASSERT(v, v->count == 1, "Puts: (puts %s)");
  LASSERT(v, v->cell[0]->type == LVAL_STR, "Puts: (puts %s)");
  /* ================================================= */
  puts(v->cell[0]->str);
  /* ================================================= */
  lval_del(v);
  /* ================================================= */
  return NULL;
}

lval* builtin_head(lenv* e, lval* v) {
  LASSERT(v, v->count == 1, "Head: to many arguments: %d", v->count);
  LASSERT(v, v->cell[0]->type == LVAL_QEXPR,
          "Head: incorect type: %s", ltype_name(v->cell[0]->type));
  LASSERT(v, v->cell[0]->count > 0, "Head: empty list");
  /* =================================================== */
  lval* a = lval_take(v, 0);
  /* =================================================== */
  while (a->count > 1) { lval_del(lval_pop(a, 1)); }
  /* =================================================== */
  return a;
}

lval* builtin_tail(lenv* e, lval* v) {
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

lval* builtin_list(lenv* e, lval* v) {
  v->type = LVAL_QEXPR;
  return v;
}

lval* builtin_join(lenv* e, lval* v) {
  for (int i = 0; i < v->count; i++) {
    LASSERT(v, v->cell[i]->type == LVAL_QEXPR, "Join: incorect types");
  }

  lval* a = lval_pop(v, 0);
  while (v->count > 0) {
    a = lval_join(a, lval_pop(v, 0));
  }

  return NULL;
}


lval* builtin_eval(lenv* e, lval* v) {
  LASSERT(v, v->count == 1, "Eval: wrong number of arguments");
  LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Eval: incorect types");
  LASSERT(v, v->cell[0]->count > 0, "Eval: empty list");
  
  lval* a = lval_take(v, 0);
  a->type = LVAL_SEXPR;
  return lval_eval(e, a);
}

lval* builtin_def(lenv* e, lval* v) {
  /* ================================================= */
  LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Def's: qexpr expected");
  lval* syms = v->cell[0];
  for (int i = 0; i < syms->count; i++) {
    LASSERT(v, syms->cell[i]->type == LVAL_SYM, "Def's: symbol expected");
  }
  /* ================================================= */
  LASSERT(v, syms->count == v->count - 1, "Def's: sym/val violet");
  for (int i = 0; i < syms->count; i++) {
    lenv_put(e, syms->cell[i], v->cell[i+1]);
  }
  /* ================================================= */
  lval_del(v);
  /* ================================================= */
  return lval_num(1);
}

lval* builtin_lambda(lenv* e, lval* v) {
  LASSERT(v, v->count == 2, "Lamda: 2 args expected");
  LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Lamda: 1 arg QEXPR expected");
  LASSERT(v, v->cell[1]->type == LVAL_QEXPR, "Lamda: 2 arg QEXPR expected");
  /* ================================================= */
  for (int i = 0; i < v->cell[0]->count; i++) {
    LASSERT(v, v->cell[0]->cell[i]->type == LVAL_SYM, "Lambda: Symbols expected");
  }
  /* ================================================= */
  lval* formals = lval_pop(v, 0);
  lval* body = lval_pop(v, 0);
  /* ================================================= */
  lval_del(v);
  /* ================================================= */
  return lval_lambda(formals, body);
}


lval* builtin_add(lenv* e, lval* v) {
  return builtin_op(v, "+");
}

lval* builtin_sub(lenv* e, lval* v) {
  return builtin_op(v, "-");
}

lval* builtin_mul(lenv* e, lval* v) {
  return builtin_op(v, "*");
}

lval* builtin_div(lenv* e, lval* v) {
  return builtin_op(v, "/");
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


int main(int argc, char** argv) {
  mpc_parser_t* number   =  mpc_new("number");
  mpc_parser_t* string   =  mpc_new("string");
  mpc_parser_t* symbol   =  mpc_new("symbol");
  mpc_parser_t* sexpr    =  mpc_new("sexpr");
  mpc_parser_t* qexpr    =  mpc_new("qexpr");
  mpc_parser_t* expr     =  mpc_new("expr");
  mpc_parser_t* lispy    =  mpc_new("lispy");
  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                        \
             number    : /-?[0-9]+/                                 ;\
             symbol    : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/             ;\
             string    : /\"[^\"]*\"/                                 ;\
             sexpr     : '(' <expr>* ')'                            ;\
             qexpr     : '`' '(' <expr>* ')'                            ;\
             expr      : <number> | <string> | <symbol> | <sexpr> | <qexpr>    ;\
             lispy     : <sexpr>                                  ;\
            ",
            number, symbol, string, sexpr, qexpr, expr, lispy);
  /* =================================================== */
  lenv* env = lenv_new();
  env = lenv_add_builtins(env);
  /* =================================================== */
  puts("Lispy version 0.0.0.0.1");
  puts("Press ctrl-c to Exit\n");
  /* =================================================== */
  while (1) {
    char* input = readline("lispy> ");
    if (input == NULL) break;
    add_history(input);
    /* =================================================== */
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, lispy, &r)) {
      //mpc_ast_print(r.output);
      /* ====================================== */
      /* lval* val = lval_read(((mpc_ast_t*)r.output)->children[0]); */
      lval* val = lval_eval(env, lval_read(r.output));
      lval_println(val);
      /* ====================================== */
      //lval_println(lval_read(r.output));
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }
  /* =================================================== */
  mpc_cleanup(4, number, symbol, expr, sexpr, qexpr, lispy);
  return 0;
}
