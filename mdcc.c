#include "mdcc.h"

char *buf;
Token tokens[100];
Map *keywords;
int pos;
int nvars;
Map *vars;

__attribute__((noreturn)) void err(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
  exit(1);
}

void usage() { err("Usage: mdcc <source file>"); }

bool isnondigit(char c) { return isalpha(c) || c == '_'; }

bool istypename() {
  Token tok = tokens[pos];
  return tok.ty == TK_INT;
}

void load_keywords() {
  keywords = new_map();
  map_set(keywords, "int", (void *)TK_INT);
}

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
  err("Bad token %c", ty);
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
      if ((var = map_get(vars, name)) == NULL) {
        var = malloc(sizeof(Var));
        var->name = name;
        var->offset = ++nvars;
        map_set(vars, name, (void *)var);
      }
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
      err("No closing parenthesis.");
    }
    pos++;
    return node;
  }
  err("Unexpected token %d.\n", tokens[pos].ty);
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
  err("Unknown declaration specifier %d", tokens[pos].ty);
}

static Node *assignment_expr() { return unary_expr(); }

static Node *init_declarator(int ty) {
  if (tokens[pos].ty != TK_IDENT)
    err("Bad token %d", tokens[pos].ty);
  char *name = tokens[pos].name;
  Var *var = malloc(sizeof(Var));
  var->name = name;
  var->offset = ++nvars;
  map_set(vars, name, (void *)var);
  pos++;
  if (consume('=')) {
    Node *lhs = new_node_ident(name, var);
    Node *rhs = assignment_expr();
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

static void tokenize() {
  int i = 0;
  while (*buf) {
    if (isspace(*buf)) {
      buf++;
      continue;
    }
    if (isdigit(*buf)) {
      tokens[i].ty = TK_NUM;
      tokens[i].val = atoi(buf);
      i++;
      buf++;
      continue;
    }
    if (isnondigit(*buf)) {
      int slen = 0;
      while (isnondigit(buf[slen]) || isdigit(buf[slen]))
        slen++;
      char *name = strndup(buf, slen);
      tokens[i].ty = (int)map_get_def(keywords, name, (void *)TK_IDENT);
      tokens[i].name = name;
      i++;
      buf += slen;
      continue;
    }
    if (*buf == '\'') {
      buf++;
      tokens[i].ty = TK_NUM;
      tokens[i].val = *buf;
      i++;
      buf++;
      if (*buf++ != '\'')
        err("Unclosed character literal.");
    }
    tokens[i].ty = *buf;
    buf++;
    i++;
  }
  tokens[i].ty = TK_EOF;
}

void gen(Node *node);

void gen_binary(Node *node) {
  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->ty) {
  case '+':
    printf("  add rax, rdi\n");
    break;
  case '-':
    printf("  sub rax, rdi\n");
    break;
  case '*':
    printf("  imul rax, rdi\n");
    break;
  case '/':
    printf("  mov rdx, 0\n");
    printf("  div rdi\n");
    break;
  default:
    err("Unknown binary operator");
  }
  printf("  push rax\n");
}

void gen_lval(Node *node) {
  if (node->ty == ND_IDENT) {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->var->offset * 8);
    printf("  push rax\n");
    return;
  }
  err("lvalue is not identifier.");
}

void gen(Node *node) {
  switch (node->ty) {
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  case ND_COMP_STMT:
    for (int i = 0; i < node->stmts->len; i++) {
      Node *stmt = node->stmts->data[i];
      if (stmt->ty == ND_NULL)
        break;
      gen(stmt);
      printf("  pop rax\n");
    }
    break;
  case ND_NULL:
    break;
  case ND_IDENT:
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
    break;
  case ND_CALL:
    printf("  call %s\n", node->name);
    printf("  push rax\n");
    break;
  case '=':
    gen_lval(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
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
    err("Unknown node type %d", node->ty);
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

  load_keywords();

  buf = argv[1];
  tokenize();

  pos = 0;
  Node *node = parse();
  printf(".intel_syntax noprefix\n");
  printf(".global _main\n");

  // Definition of sample function z.
  // z needs no argument and returns 1.
  printf("z:\n");
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  mov rax, 1\n");
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");

  printf("_main:\n");
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", nvars * 8);

  gen(node);

  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}
