#include "mdcc.h"

int nvars = 0;
static Map *vars;

__attribute__((noreturn)) static void bad_token(Token *t, char *fmt) {
  error("Error at token %d", t->ty);
  error(fmt);
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
  bad_token(&tokens[pos], "Bad token");
}

static Node *postfix_expr() { return primary_expr(); }

static Node *unary_expr() { return postfix_expr(); }

static Node *cast_expr() { return unary_expr(); }

static Node *mul_expr() {
  Node *lhs = cast_expr();
  int ty = tokens[pos].ty;
  if (ty == '*' || ty == '/' || ty == '%') {
    pos++;
    return new_node(ty, lhs, mul_expr());
  } else {
    return lhs;
  }
}

static void *decl();

static Node *expr() {
  if (istypename())
    return decl();
  Node *lhs = mul_expr();
  int ty = tokens[pos].ty;
  if (ty == '+' || ty == '-') {
    pos++;
    return new_node(ty, lhs, expr());
  } else if (ty == '=') {
    pos++;
    return new_node(ty, lhs, expr());
  } else {
    return lhs;
  }
}

static Node *expr_stmt() {
  if (consume(';'))
    return new_node_null();
  Node *e = expr();
  expect(';');
  return e;
}

static int decl_specifier() {
  if (tokens[pos].ty == TK_INT) {
    pos++;
    return TK_INT;
  }
  bad_token(&tokens[pos], format("Unknown declaration specifier"));
}

static Var *declarator() {
  if (tokens[pos].ty != TK_IDENT)
    bad_token(&tokens[pos], "Token is not identifier.");
  char *name = tokens[pos].name;
  Var *var = malloc(sizeof(Var));
  var->name = name;
  var->offset = ++nvars;
  map_set(vars, name, (void *)var);
  pos++;
  return var;
}

static Node *init_declarator(int ty) {
  Var *var = declarator();
  if (consume('=')) {
    Node *lhs = new_node_ident(var->name, var);
    Node *rhs = expr();
    return new_node('=', lhs, rhs);
  } else {
    return new_node_null();
  }
}

static void *decl() {
  int ty = decl_specifier();
  if (consume(';'))
    return new_node_null();
  Node *node = init_declarator(ty);
  return node;
}

static Node *stmt() { return expr_stmt(); }

static Node *compound_stmt() {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_COMP_STMT;
  node->stmts = new_vec();
  while (!consume('}')) {
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
