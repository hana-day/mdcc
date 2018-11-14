#include "mdcc.h"

#define DEFAULT_VEC_SIZE 16

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

bool istypename() {
  Token tok = tokens[pos];
  return tok.ty == TK_INT;
}

char *format(char *fmt, ...) {
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  return strdup(buf);
}
