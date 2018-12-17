#include "mdcc.h"

#define DEFAULT_VEC_SIZE 16

__attribute__((noreturn)) void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

Vector *new_vec() {
  Vector *v = malloc(sizeof(Vector));
  v->data = malloc(sizeof(void *) * DEFAULT_VEC_SIZE);
  v->len = 0;
  v->capacity = DEFAULT_VEC_SIZE;
  return v;
}

void vec_push(Vector *v, void *elm) {
  if (v->len == v->capacity) {
    v->capacity *= 2;
    v->data = realloc(v->data, sizeof(void *) * v->capacity);
  }
  v->data[v->len++] = elm;
}

void *vec_pop(Vector *v) { return v->data[--v->len]; }

Map *new_map(void) {
  Map *map = malloc(sizeof(Map));
  map->keys = new_vec();
  map->vals = new_vec();
  return map;
}

void map_set(Map *map, char *key, void *val) {
  vec_push(map->keys, key);
  vec_push(map->vals, val);
}

void *map_get(Map *map, char *key) {
  for (int i = map->keys->len - 1; i >= 0; i--)
    if (!strcmp(map->keys->data[i], key))
      return map->vals->data[i];
  return NULL;
}

void *map_get_def(Map *map, char *key, void *defv) {
  void *v = map_get(map, key);
  return v == NULL ? defv : v;
}

bool isnondigit(char c) { return isalpha(c) || c == '_'; }

char *format(char *fmt, ...) {
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  return strdup(buf);
}

/**
 * Round up the given value to a multiple of size. Size must be a power of 2.
 *
 * Example:
 *   roundup(4, 4) // => 4
 *   roundup(3, 4) // => 4
 *   roundup(5, 4) // => 8
 *
 */
inline int roundup(int x, int align) { return (x + align - 1) & ~(align - 1); }

Type *new_type(int ty, int size) {
  Type *t = malloc(sizeof(Type));
  t->ty = ty;
  t->size = size;
  t->align = size;
  return t;
}

Type *ptr(Type *ty) {
  Type *t = new_type(TY_PTR, 8);
  t->ptr_to = ty;
  return t;
}

Node *new_node(int ty, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_one(int ty, Node *expr) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->expr = expr;
  return node;
}

Type *new_int_ty() { return new_type(TY_INT, 8); }

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  node->cty = new_int_ty();
  return node;
}

Node *new_node_null() {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NULL;
  return node;
}
