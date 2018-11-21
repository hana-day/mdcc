#ifndef __MDCC_H__
#define __MDCC_H__

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

typedef struct {
  Vector *keys;
  Vector *vals;
} Map;

enum {
  TK_NUM = 256,
  TK_INT,
  TK_IDENT,
  TK_RETURN,
  TK_IF,
  TK_ELSE,
  TK_EOF,
};

enum {
  ND_NUM = 256,
  ND_COMP_STMT,
  ND_IDENT,
  ND_CALL,
  ND_ADDR,
  ND_DEREF,
  ND_ROOT,
  ND_FUNC,
  ND_RETURN,
  ND_IF,
  ND_NULL,
};

typedef struct Position {
  int offset; // oofset, starting at 0
  int line;   // line number, starting at 1
} Position;

typedef struct {
  int ty;     // Token type
  char *name; // Operator
  int val;    // Number value

  Position *pos;
} Token;

enum {
  TY_INT = 1,
  TY_PTR,
};

typedef struct Type {
  int ty;
  struct Type *ptr_to;
} Type;

typedef struct Var {
  Type *ty;
  char *name;
  int offset;
} Var;

struct Function;

/**
 *  Function definition
 *  int "name" ("params") {"body"}
 *
 *  Function call
 *  "name"("args")
 *
 *  If statement
 *  if ("cond") "then" else "els"
 *
 *  Return statement
 *  return "expr"
 */
typedef struct Node {
  int ty; // Node type

  struct Node *lhs;
  struct Node *rhs;

  int val;
  Var *var;
  char *name;
  Vector *params;
  struct Node *body;
  Vector *func_vars;
  Vector *args;
  struct Node *expr;
  struct Node *cond;
  struct Node *then;
  struct Node *els;

  // Vector of statements
  Vector *stmts;

  // For the root node
  Vector *funcs;
} Node;

// Basic block
typedef struct BB {
} BB;

// main.c
__attribute__((noreturn)) void error(char *fmt, ...);

// util.c
Vector *new_vec(void);
void vec_push(Vector *v, void *elm);
void *vec_pop(Vector *v);
Map *new_map(void);
void map_set(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);
void *map_get_def(Map *map, char *key, void *defv);
bool isnondigit(char c);
char *format(char *fmt, ...);

// token.c
extern char *buf;
extern int pos;
Vector *tokenize();

// parse.c
Node *parse();
extern int nvars;

// x64.c
void gen_x64(Node *node);

// test_util.c
void test();

#endif
