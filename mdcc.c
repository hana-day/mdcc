#include "mdcc.h"

int pos = 0;
int nvars;
Map *vars;

__attribute__((noreturn)) void error(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(1);
}

__attribute__((noreturn)) void bad_token(Token *t, char *fmt) {
  error("Error at token %d", t->ty);
  error(fmt);
  exit(1);
}

char *format(char *fmt, ...) {
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  return strdup(buf);
}

static void usage() { error("Usage: mdcc <source file>"); }

static bool consume(int ty) {
  if (tokens[pos].ty != ty)
    return false;
  pos++;
  return true;
}

Token *new_token() {
  Token *tok = malloc(sizeof(Token));
  return tok;
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

static Node *init_declarator(int ty) {
  if (tokens[pos].ty != TK_IDENT)
    bad_token(&tokens[pos], "Token is not identifier.");
  char *name = tokens[pos].name;
  Var *var = malloc(sizeof(Var));
  var->name = name;
  var->offset = ++nvars;
  map_set(vars, name, (void *)var);
  pos++;
  if (consume('=')) {
    Node *lhs = new_node_ident(name, var);
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

static Node *parse() {
  Node *node;
  if (consume('{')) {
    node = compound_stmt();
  } else {
    node = expr();
  }
  return node;
}

static void emit(char *inst, char *arg0, char *arg1) {
  printf("  %s", inst);
  if (arg0 != NULL) {
    printf(" %s", arg0);
    if (arg1 != NULL)
      printf(", %s", arg1);
  }
  printf("\n");
}

static void emit_label(char *s) { printf("%s:\n", s); }

static void emit_directive(char *s) { printf(".%s\n", s); }

void gen(Node *node);

void gen_binary(Node *node) {
  gen(node->lhs);
  gen(node->rhs);

  emit("pop", "rdi", NULL);
  emit("pop", "rax", NULL);

  switch (node->ty) {
  case '+':
    emit("add", "rax", "rdi");
    break;
  case '-':
    emit("sub", "rax", "rdi");
    break;
  case '*':
    emit("imul", "rax", "rdi");
    break;
  case '/':
    emit("mov", "rdx", "0");
    emit("div", "rdi", NULL);
    break;
  default:
    error("Unknown binary operator %d", node->ty);
  }
  emit("push", "rax", NULL);
}

void gen_lval(Node *node) {
  if (node->ty == ND_IDENT) {
    emit("mov", "rax", "rbp");
    emit("sub", "rax", format("%d", node->var->offset * 8));
    emit("push", "rax", NULL);
    return;
  }
  error("lvalue is not identifier.");
}

void gen(Node *node) {
  switch (node->ty) {
  case ND_NUM:
    emit("push", format("%d", node->val), NULL);
    return;
  case ND_COMP_STMT:
    for (int i = 0; i < node->stmts->len; i++) {
      Node *stmt = node->stmts->data[i];
      if (stmt->ty == ND_NULL)
        break;
      gen(stmt);
      emit("pop", "rax", NULL);
    }
    break;
  case ND_NULL:
    break;
  case ND_IDENT:
    gen_lval(node);
    emit("pop", "rax", NULL);
    emit("mov", "rax", "[rax]");
    emit("push", "rax", NULL);
    return;
    break;
  case ND_CALL:
    emit("call", format("%s", node->name), NULL);
    emit("push", "rax", NULL);
    break;
  case '=':
    gen_lval(node->lhs);
    gen(node->rhs);

    emit("pop", "rdi", NULL);
    emit("pop", "rax", NULL);
    emit("mov", "[rax]", "rdi");
    emit("push", "rdi", NULL);
    return;
    break;
  case '+':
  case '-':
  case '*':
  case '/':
    gen_binary(node);
    return;
    break;
  default:
    error("Unknown node type %d", node->ty);
  }
}

int main(int argc, char **argv) {
  if (argc == 1)
    usage();

  if (strcmp(argv[1], "-test") == 0) {
    test();
    return 0;
  }

  nvars = 0;
  vars = new_map();

  buf = argv[1];
  tokenize();

  Node *node = parse();
  emit_directive("intel_syntax noprefix");
  emit_directive("global _main");

  // Definition of sample function z.
  // z needs no argument and returns 1.
  emit_label("z");
  emit("push", "rbp", NULL);
  emit("mov", "rbp", "rsp");
  emit("mov", "rax", "1");
  emit("mov", "rsp", "rbp");
  emit("pop", "rbp", NULL);
  emit("ret", NULL, NULL);

  emit_label("_main");
  emit("push", "rbp", NULL);
  emit("mov", "rbp", "rsp");
  emit("sub", "rsp", format("%d", nvars * 8));

  gen(node);

  emit("mov", "rsp", "rbp");
  emit("pop", "rbp", NULL);
  emit("ret", NULL, NULL);
  return 0;
}
