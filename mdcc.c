#include "mdcc.h"

int pos = 0;
int nvars;

__attribute__((noreturn)) void error(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(1);
}

static void usage() { error("Usage: mdcc <source file>"); }

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
