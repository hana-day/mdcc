#include "mdcc.h"

typedef struct Scope {
  struct Scope *outer;
  Map *vars;
} Scope;

Vector *tokens;
Scope *scope;
Vector *func_vars;

inline static Token *peek(int p) { return tokens->data[p]; }

static bool istypename() { return peek(pos)->ty == TK_INT; }

static Scope *new_scope(Scope *outer) {
  Scope *scope = malloc(sizeof(Scope));
  scope->vars = new_map();
  scope->outer = outer;
  return scope;
}

static Var *lookup_var(char *name) {
  for (Scope *s = scope; s != NULL; s = s->outer) {
    Var *var = map_get(s->vars, name);
    if (var)
      return var;
  }
  return NULL;
}

static Var *new_var(Type *ty, char *name) {
  Var *var = malloc(sizeof(Var));
  var->ty = ty;
  var->name = name;
  map_set(scope->vars, name, (void *)var);
  vec_push(func_vars, (void *)var);
  return var;
}

__attribute__((noreturn)) static void bad_token(Token *t, char *msg) {
  if (t->ty < 256) {
    fprintf(stderr, "Error at token %c", t->ty);
  } else {
    fprintf(stderr, "Error at token %d", t->ty);
  }
  fprintf(stderr, ", line: %d, offset: %d\n", t->pos->line, t->pos->offset);
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

static bool consume(int ty) {
  if (peek(pos)->ty != ty)
    return false;
  pos++;
  return true;
}

static void expect(int ty) {
  Token *t = peek(pos);
  if (t->ty == ty) {
    pos++;
    return;
  }
  bad_token(t, format("Expected token %d", ty));
}

static Node *new_node_ident(char *name, Var *var) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->name = name;
  node->var = var;
  node->cty = var->ty;
  return node;
}

static Node *expr();

static Node *assignment_expr();

static Node *primary_expr() {
  if (peek(pos)->ty == TK_IDENT) {
    Token *tok = peek(pos++);
    char *name = strdup(tok->name);
    if (consume('(')) {
      Node *node = malloc(sizeof(Node));
      node->ty = ND_CALL;
      node->name = name;
      node->args = new_vec();
      while (!consume(')')) {
        vec_push(node->args, (void *)assignment_expr());
        if (consume(')'))
          break;
        expect(',');
      }
      return node;
    } else {
      Var *var;
      if ((var = lookup_var(name)) == NULL)
        bad_token(tok, format("Undefined identifier %s", tok->name));
      return new_node_ident(name, var);
    }
  }
  if (peek(pos)->ty == TK_NUM) {
    return new_node_num(peek(pos++)->val);
  }
  if (peek(pos)->ty == '(') {
    pos++;
    Node *node = expr();
    if (peek(pos)->ty != ')') {
      bad_token(peek(pos), "No closing parenthesis.");
    }
    pos++;
    return node;
  }
  return new_node_null();
}

static Node *postfix_expr() {
  Node *lhs = primary_expr();
  bool arrmode = false;
  while (1) {
    if (consume('[')) {
      arrmode = true;
      Node *node = new_node('-', lhs, expr());
      lhs = new_node(ND_DEREF, new_node_null(), node);
      expect(']');
    } else if (consume(TK_INC)) {
      lhs = new_node(ND_INC, new_node_null(), lhs);
    } else if (consume(TK_DEC)) {
      lhs = new_node(ND_DEC, new_node_null(), lhs);
    } else {
      return lhs;
    }
  }
}

static Node *cast_expr();

static Node *unary_expr() {
  if (consume('&'))
    return new_node(ND_ADDR, new_node_null(), cast_expr());
  if (consume('*'))
    return new_node(ND_DEREF, new_node_null(), cast_expr());
  if (consume(TK_INC)) {
    Node *lhs = unary_expr();
    return new_node('=', lhs, new_node('+', lhs, new_node_num(1)));
  }
  if (consume(TK_DEC)) {
    Node *lhs = unary_expr();
    return new_node('=', lhs, new_node('-', lhs, new_node_num(1)));
  }
  return postfix_expr();
}

static Node *cast_expr() { return unary_expr(); }

static void *decl();

static Node *conditional_expr();

static Node *assignment_expr() {
  int prev_pos = pos;
  Node *lhs = unary_expr();

  // If next token is assignment operator, parse assignment-expression as rhs.
  // Otherwise, rollback the position of the token and parse
  // conditional-expression.
  if (consume('=')) {
    return new_node('=', lhs, assignment_expr());
  } else if (consume(TK_ADD_EQ)) {
    return new_node('=', lhs, new_node('+', lhs, assignment_expr()));
  } else if (consume(TK_SUB_EQ)) {
    return new_node('=', lhs, new_node('-', lhs, assignment_expr()));
  } else if (consume(TK_MUL_EQ)) {
    return new_node('=', lhs, new_node('*', lhs, assignment_expr()));
  } else if (consume(TK_DIV_EQ)) {
    return new_node('=', lhs, new_node('/', lhs, assignment_expr()));
  } else if (consume(TK_SHL_EQ)) {
    return new_node('=', lhs, new_node(ND_SHL, lhs, assignment_expr()));
  } else if (consume(TK_SHR_EQ)) {
    return new_node('=', lhs, new_node(ND_SHR, lhs, assignment_expr()));
  } else if (consume(TK_BAND_EQ)) {
    return new_node('=', lhs, new_node('&', lhs, assignment_expr()));
  } else if (consume(TK_BOR_EQ)) {
    return new_node('=', lhs, new_node('|', lhs, assignment_expr()));
  } else if (consume(TK_XOR_EQ)) {
    return new_node('=', lhs, new_node('^', lhs, assignment_expr()));
  } else {
    pos = prev_pos;
    return conditional_expr();
  }
}

static Node *multiplicative_expr() {
  Node *lhs = cast_expr();
  int ty = peek(pos)->ty;
  if (ty == '*' || ty == '/' || ty == '%') {
    pos++;
    return new_node(ty, lhs, multiplicative_expr());
  } else {
    return lhs;
  }
}

static Node *additive_expr() {
  Node *lhs = multiplicative_expr();
  int ty = peek(pos)->ty;
  if (ty == '+' || ty == '-') {
    pos++;
    return new_node(ty, lhs, additive_expr());
  } else {
    return lhs;
  }
}

static Node *shift_expr() {
  Node *lhs = additive_expr();
  if (consume(TK_SHL))
    return new_node(ND_SHL, lhs, shift_expr());
  else if (consume(TK_SHR))
    return new_node(ND_SHR, lhs, shift_expr());
  return lhs;
}

static Node *relational_expr() {
  Node *lhs = shift_expr();
  if (consume('<'))
    return new_node('<', lhs, relational_expr());
  else if (consume('>'))
    return new_node('>', lhs, relational_expr());
  return lhs;
}

static Node *equality_expr() {
  Node *lhs = relational_expr();
  if (consume(TK_EQ))
    return new_node(ND_EQ, lhs, equality_expr());
  else if (consume(TK_NEQ))
    return new_node(ND_NEQ, lhs, equality_expr());
  else
    return lhs;
}

static Node *and_expr() {
  Node *lhs = equality_expr();
  if (consume('&'))
    return new_node('&', lhs, and_expr());
  return lhs;
}

static Node *exclusive_or_expr() {
  Node *lhs = and_expr();
  if (consume('^'))
    return new_node('^', lhs, exclusive_or_expr());
  return lhs;
}

static Node *inclusive_or_expr() {
  Node *lhs = exclusive_or_expr();
  if (consume('|'))
    return new_node('|', lhs, exclusive_or_expr());
  return lhs;
}

static Node *logical_and_expr() {
  Node *lhs = inclusive_or_expr();
  if (consume(TK_AND))
    return new_node(ND_AND, lhs, logical_and_expr());
  return lhs;
}

static Node *logical_or_expr() {
  Node *lhs = logical_and_expr();
  if (consume(TK_OR))
    return new_node(ND_OR, lhs, logical_or_expr());
  return lhs;
}

static Node *conditional_expr() { return logical_or_expr(); }

static Node *expr() { return assignment_expr(); }

static Node *expr_stmt() {
  if (consume(';'))
    return new_node_null();
  Node *e = expr();
  expect(';');
  return e;
}

static Type *decl_specifier() {
  if (consume(TK_INT)) {
    return new_int_ty();
  }
  bad_token(peek(pos),
            format("Unknown declaration specifier %d", peek(pos)->val));
}

static Type *arr(Type *base, int len) {
  Type *ty = malloc(sizeof(Type));
  ty->ty = TY_ARR;
  ty->size = base->size * len;
  ty->align = base->align;
  ty->arr_of = base;
  ty->len = len;
  return ty;
}

static Node *declr(Type *ty);

static Node *param_decl() {
  Type *ty = decl_specifier();
  Node *node = declr(ty);
  return node;
}

static Vector *param_list() {
  Vector *params = new_vec();
  Node *param;
  param = param_decl();
  vec_push(params, (void *)param);
  while (consume(',')) {
    param = param_decl();
    vec_push(params, param);
  }
  return params;
}

static Vector *param_type_list() { return param_list(); }
static Node *comp_stmt();

static Type *read_arr(Type *ty) {
  Vector *v = new_vec();

  while (consume('[')) {
    if (consume(']')) {
      vec_push(v, (void *)-1);
      continue;
    }
    Token *t = peek(pos);
    if (t->ty != TK_NUM)
      bad_token(t, "Expected number");
    pos++;
    vec_push(v, (void *)(uintptr_t)t->val);
    expect(']');
  }
  for (int i = v->len - 1; i >= 0; i--) {
    ty = arr(ty, (int)v->data[i]);
  }
  return ty;
}

static Node *direct_declr(Type *ty) {
  if (peek(pos)->ty != TK_IDENT)
    bad_token(peek(pos), "Token is not identifier.");
  char *name = peek(pos)->name;
  pos++;

  // Function parameters
  if (consume('(')) {
    Vector *params;
    if (consume(')')) {
      params = new_vec();
    } else {
      params = param_type_list();
      expect(')');
    }
    Node *node = new_node(ND_FUNC, NULL, NULL);
    node->name = name;
    node->params = params;
    return node;
    // Variable definition
  } else {
    Node *node = new_node(ND_IDENT, NULL, NULL);
    ty = read_arr(ty);
    node->var = new_var(ty, name);
    node->cty = node->var->ty;
    return node;
  }
}

static Node *declr(Type *ty) {
  while (consume('*'))
    ty = ptr(ty);
  return direct_declr(ty);
}

static Node *init_declr(Type *ty) {
  Node *node = declr(ty);
  if (consume('=')) {
    Node *lhs = new_node_ident(node->var->name, node->var);
    Node *rhs = expr();
    return new_node('=', lhs, rhs);
  } else {
    return new_node_null();
  }
}

static void *decl() {
  Type *ty = decl_specifier();
  if (consume(';'))
    return new_node_null();
  Node *node = init_declr(ty);
  expect(';');
  return node;
}

static Node *jmp_stmt() {
  Node *node;
  if (consume(TK_RETURN)) {
    node = new_node(ND_RETURN, NULL, NULL);
    node->expr = expr();
    return node;
  }
  bad_token(peek(pos), "Unknown jump statement");
}

static Node *stmt();

static Node *selection_stmt() {
  consume(TK_IF);
  Node *node = new_node(ND_IF, NULL, NULL);
  expect('(');
  node->cond = expr();
  expect(')');
  node->then = stmt();
  if (consume(TK_ELSE))
    node->els = stmt();
  return node;
}

static Node *iter_stmt() {
  if (consume(TK_FOR)) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_FOR;
    expect('(');
    scope = new_scope(scope);
    node->init = expr();
    expect(';');
    node->cond = expr();
    expect(';');
    node->after = expr();
    expect(')');
    node->body = stmt();
    scope = scope->outer;
    return node;
  } else if (consume(TK_WHILE)) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_WHILE;
    expect('(');
    node->cond = expr();
    expect(')');
    node->body = stmt();
    return node;
  }
  return new_node_null();
}

static Node *stmt() {
  int ty = peek(pos)->ty;
  if (ty == TK_RETURN) {
    return jmp_stmt();
  } else if (ty == '{') {
    return comp_stmt();
  } else if (ty == TK_IF) {
    return selection_stmt();
  } else if (ty == TK_FOR) {
    return iter_stmt();
  } else if (ty == TK_WHILE) {
    return iter_stmt();
  } else {
    return expr_stmt();
  }
}

static Node *comp_stmt() {
  expect('{');
  scope = new_scope(scope);
  Node *node = new_node(ND_COMP_STMT, NULL, NULL);
  node->stmts = new_vec();
  while (!consume('}')) {
    if (istypename())
      vec_push(node->stmts, decl());
    else
      vec_push(node->stmts, stmt());
  }
  scope = scope->outer;
  return node;
}

static Node *func_def() {
  func_vars = new_vec();
  Type *ty = decl_specifier();
  Node *node = declr(ty);
  if (node->ty != ND_FUNC)
    bad_token(peek(pos),
              format("expected function definition but got %d", node->ty));
  node->body = comp_stmt();
  node->func_vars = func_vars;
  return node;
}

static Node *root() {
  Node *node = new_node(ND_ROOT, NULL, NULL);
  node->funcs = new_vec();
  while (peek(pos)->ty != TK_EOF) {
    vec_push(node->funcs, func_def());
  }
  return node;
}

Node *parse(Vector *_tokens) {
  tokens = _tokens;

  Node *node;
  scope = new_scope(NULL);
  node = root();
  return node;
}
