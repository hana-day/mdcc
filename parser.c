#include "mdcc.h"

int nvars = 0;
static Map *vars;

__attribute__((noreturn)) static void bad_token(Token *t, char *msg) {
  fprintf(stderr, "Error at token %d\n", t->ty);
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

static bool consume(int ty) {
  if (tokens[pos].ty != ty)
    return false;
  pos++;
  return true;
}

static void expect(int ty) {
  Token t = tokens[pos];
  if (t.ty == ty) {
    pos++;
    return;
  }
  bad_token(&t, format("Expected token %d", ty));
}

static Node *new_node(int ty, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}

static Node *new_node_null() {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NULL;
  return node;
}

static Node *new_node_ident(char *name, Var *var) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->name = name;
  node->var = var;
  return node;
}

static Node *expr();

static Node *primary_expr() {
  if (tokens[pos].ty == TK_IDENT) {
    char *name = strdup(tokens[pos].name);
    if (tokens[pos + 1].ty == '(') {
      Node *node = malloc(sizeof(Node));
      node->ty = ND_CALL;
      node->name = name;
      pos += 2;
      expect(')');
      return node;
    } else {
      Var *var;
      if ((var = map_get(vars, name)) == NULL)
        bad_token(&tokens[pos],
                  format("Undefined identifier %s", tokens[pos].name));
      pos++;
      return new_node_ident(name, var);
    }
  }
  if (tokens[pos].ty == TK_NUM) {
    return new_node_num(tokens[pos++].val);
  }
  if (tokens[pos].ty == '(') {
    pos++;
    Node *node = expr();
    if (tokens[pos].ty != ')') {
      bad_token(&tokens[pos], "No closing parenthesis.");
    }
    pos++;
    return node;
  }
  return new_node_null();
}

static Node *postfix_expr() { return primary_expr(); }

static Node *cast_expr();

static Node *unary_expr() {
  if (consume('&'))
    return new_node(ND_ADDR, new_node_null(), cast_expr());
  if (consume('*'))
    return new_node(ND_DEREF, new_node_null(), cast_expr());
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
  } else {
    pos = prev_pos;
    return conditional_expr();
  }
}

static Node *multiplicative_expr() {
  Node *lhs = cast_expr();
  int ty = tokens[pos].ty;
  if (ty == '*' || ty == '/' || ty == '%') {
    pos++;
    return new_node(ty, lhs, multiplicative_expr());
  } else {
    return lhs;
  }
}

static Node *additive_expr() {
  Node *lhs = multiplicative_expr();
  int ty = tokens[pos].ty;
  if (ty == '+' || ty == '-') {
    pos++;
    return new_node(ty, lhs, additive_expr());
  } else {
    return lhs;
  }
}

static Node *shift_expr() { return additive_expr(); }

static Node *relational_expr() { return shift_expr(); }

static Node *equality_expr() { return relational_expr(); }

static Node *and_expr() { return equality_expr(); }

static Node *exclusive_or_expr() { return and_expr(); }

static Node *inclusive_or_expr() { return exclusive_or_expr(); }

static Node *logical_and_expr() { return inclusive_or_expr(); }

static Node *logical_or_expr() { return logical_and_expr(); }

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
  if (tokens[pos].ty == TK_INT) {
    pos++;
    Type *type = malloc(sizeof(Type));
    type->ty = TY_INT;
    return type;
  }
  bad_token(&tokens[pos], format("Unknown declaration specifier"));
}

static Type *ptr(Type *ty) {
  Type *newty = malloc(sizeof(Type));
  newty->ty = TY_PTR;
  newty->ptr_to = ty;
  return newty;
}

static Var *declarator(Type *ty) {
  while (consume('*'))
    ty = ptr(ty);
  if (tokens[pos].ty != TK_IDENT)
    bad_token(&tokens[pos], "Token is not identifier.");
  char *name = tokens[pos].name;
  Var *var = malloc(sizeof(Var));
  var->ty = ty;
  var->name = name;
  var->offset = ++nvars;
  map_set(vars, name, (void *)var);
  pos++;
  return var;
}

static Node *init_declarator(Type *ty) {
  Var *var = declarator(ty);
  if (consume('=')) {
    Node *lhs = new_node_ident(var->name, var);
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
  Node *node = init_declarator(ty);
  expect(';');
  return node;
}

static Node *stmt() { return expr_stmt(); }

static Node *compound_stmt() {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_COMP_STMT;
  node->stmts = new_vec();
  while (!consume('}')) {
    if (istypename(tokens[pos].ty))
      vec_push(node->stmts, decl());
    else
      vec_push(node->stmts, stmt());
  }
  return node;
}

Node *parse() {
  vars = new_map();

  Node *node;
  if (consume('{')) {
    node = compound_stmt();
  } else {
    node = expr();
  }
  return node;
}
