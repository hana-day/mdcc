#include "mdcc.h"

static void emit(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printf("  ");
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
}

static void emit_label(char *s) { printf("%s:\n", s); }

static void emit_prologue(Node *func) {
  emit("push rbp");
  emit("mov rbp, rsp");
  emit("sub rsp, %d", func->func_vars->len * 8);
}

static void emit_epilogue() {
  emit("mov rsp, rbp");
  emit("pop rbp");
}

static void emit_directive(char *s) { printf(".%s\n", s); }

static void gen(Node *node);

static void gen_binary(Node *node) {
  gen(node->lhs);
  gen(node->rhs);

  emit("pop rdi");
  emit("pop rax");

  switch (node->ty) {
  case '+':
    emit("add rax, rdi");
    break;
  case '-':
    emit("sub rax, rdi");
    break;
  case '*':
    emit("imul rax, rdi");
    break;
  case '/':
    emit("mov rdx, 0");
    emit("div rdi");
    break;
  default:
    error("Unknown binary operator %d", node->ty);
  }
  emit("push rax");
}

static void gen_lval(Node *node) {
  if (node->ty == ND_IDENT) {
    emit("mov rax, rbp");
    emit("sub rax, %d", node->var->offset * 8);
    emit("push rax");
    return;
  }
  error("lvalue is not identifier.");
}

static void gen_ident(Node *node) {
  gen_lval(node);
  emit("pop rax");
  emit("mov rax, [rax]");
  emit("push rax");
}

static void gen(Node *node) {
  switch (node->ty) {
  case ND_NUM:
    emit("push %d", node->val);
    return;
  case ND_COMP_STMT:
    for (int i = 0; i < node->stmts->len; i++) {
      Node *stmt = node->stmts->data[i];
      if (stmt->ty == ND_NULL)
        break;
      gen(stmt);
      emit("pop rax");
    }
    break;
  case ND_NULL:
    break;
  case ND_IDENT:
    gen_ident(node);
    return;
    break;
  case ND_CALL:
    emit("call %s", node->name);
    emit("push rax");
    break;
  case ND_ADDR:
    gen_lval(node->rhs);
    break;
  case ND_DEREF:
    gen_ident(node->rhs);
    emit("pop rax");
    emit("mov rax, [rax]");
    emit("push rax");
    break;
  case ND_ROOT:
    for (int i = 0; i < node->funcs->len; i++) {
      Node *func = node->funcs->data[i];
      if (!strcmp(func->name, "main")) {
        emit_label(format("_%s", "main"));
      } else {
        emit_label(func->name);
      }
      emit_prologue(func);
      gen(func->body);
      emit_epilogue();
      emit("ret");
    }
    break;
  case '=':
    gen_lval(node->lhs);
    gen(node->rhs);

    emit("pop rdi");
    emit("pop rax");
    emit("mov [rax], rdi");
    emit("push rdi");
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

void gen_x64(Node *node) {
  emit_directive("intel_syntax noprefix");
  emit_directive("global _main");

  // Definition of sample function z.
  // z needs no argument and returns 1.
  emit_label("z");
  emit("push rbp");
  emit("mov rbp, rsp");
  emit("mov rax, 1");
  emit("mov rsp, rbp");
  emit("pop rbp");
  emit("ret");

  gen(node);
}
