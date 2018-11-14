#ifndef __MDCC_H__
#define __MDCC_H__

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__((noreturn)) void error(char *fmt, ...);

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

Vector *new_vec(void);
void vec_push(Vector *v, void *elm);
void *vec_pop(Vector *v);

typedef struct {
  Vector *keys;
  Vector *vals;
} Map;

// util.c
Map *new_map(void);
void map_set(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);
void *map_get_def(Map *map, char *key, void *defv);
bool isnondigit(char c);
bool istypename();
char *format(char *fmt, ...);

enum {
  TK_NUM = 256,
  TK_INT,
  TK_IDENT,
  TK_EOF,
};

enum {
  ND_NUM = 256,
  ND_COMP_STMT,
  ND_IDENT,
  ND_CALL,
  ND_NULL,
};

typedef struct {
  int ty;     // Token type
  char *name; // Operator
  int val;    // Number value
} Token;

typedef struct Var {
  int ty;
  char *name;
  int offset;
} Var;

typedef struct Node {
  int ty; // Node type
  struct Node *lhs;
  struct Node *rhs;
  int val;

  Var *var;

  char *name;

  // Vector of statements
  Vector *stmts;
} Node;

extern Token tokens[100];
extern char *buf;
extern int pos;
void tokenize();

Node *parse();
extern int nvars;

void test();

#endif
