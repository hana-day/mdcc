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
  TK_CHAR,
  TK_IDENT,
  TK_RETURN,
  TK_IF,
  TK_ELSE,
  TK_EQ,
  TK_NEQ,
  TK_FOR,
  TK_WHILE,
  TK_SHL,
  TK_SHR,
  TK_ADD_EQ,  // +=
  TK_SUB_EQ,  // -=
  TK_MUL_EQ,  // *=
  TK_DIV_EQ,  // /=
  TK_SHL_EQ,  // <<=
  TK_SHR_EQ,  // >>=
  TK_GEQ,     // >=
  TK_LEQ,     // <=
  TK_AND,     // &&
  TK_BAND_EQ, // &=
  TK_OR,      // ||
  TK_BOR_EQ,  // |=
  TK_XOR_EQ,  // ^=
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
  ND_EQ,
  ND_NEQ,
  ND_NULL,
  ND_FOR,
  ND_SHL,
  ND_SHR,
  ND_AND,
  ND_OR,
  ND_WHILE,
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
  TY_CHAR,
  TY_PTR,
  TY_ARR,
};

typedef struct Type {
  int ty;
  struct Type *ptr_to;
  struct Type *arr_of;
  int len; // for array

  // Byte size of types
  // char: 1, int: 4, ptr: 8
  int size;

  int align;
} Type;

typedef struct Var {
  Type *ty;
  char *name;
  // Offset from rbp
  int offset;

  // has_address is true if the variable is passed as
  // an pointer-like argument of a function.
  bool has_address;
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
 *
 *  For statement
 *  for ("init"; "cond"; "after") "body"
 *
 *  While statement
 *  while ("cond") "body"
 */
typedef struct Node {
  int ty; // Node type

  struct Node *lhs;
  struct Node *rhs;

  int val;
  Var *var;
  Type *cty; // C type.
  char *name;
  Vector *params;
  struct Node *body;
  Vector *func_vars;
  Vector *args;
  struct Node *expr;
  struct Node *cond;
  struct Node *then;
  struct Node *els;
  struct Node *init;
  struct Node *after;

  // Vector of statements
  Vector *stmts;

  // For the root node
  Vector *funcs;

  // For array reference
  Vector *indice;
} Node;

// Basic block
typedef struct BB {
} BB;

// main.c

// util.c
__attribute__((noreturn)) void error(char *fmt, ...);
Vector *new_vec(void);
void vec_push(Vector *v, void *elm);
void *vec_pop(Vector *v);
Map *new_map(void);
void map_set(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);
void *map_get_def(Map *map, char *key, void *defv);
bool isnondigit(char c);
char *format(char *fmt, ...);
int roundup(int x, int align);
Type *new_type(int ty, int size);
Type *ptr(Type *ty);
Node *new_node(int ty, Node *lhs, Node *rhs);
Type *new_int_ty();
Node *new_node_num(int val);
Node *new_node_null();

// token.c
extern char *buf;
extern int pos;
Vector *tokenize();

// parse.c
Node *parse();
extern int nvars;

// conv.c
Node *conv(Node *node);

// x64.c
void gen_x64(Node *node);

// test_util.c
void test();

#endif
