#include "mdcc.h"

static int nlabel = 1;

static void emit(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printf("  ");
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
}

static void emit_label(char *s) { printf("%s:\n", s); }

static char *bb_label() { return format("MDCC_BB_%d", nlabel++); }

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

static void assign(Node *lhs, Node *rhs) {
  gen_lval(lhs);
  gen(rhs);

  emit("pop rdi");
  emit("pop rax");
  emit("mov [rax], rdi");
  emit("push rdi");
}

static void load_args(Node *func) {
  for (int i = 0; i < func->params->len; i++) {
    Node *param = func->params->data[i];
    gen_lval(param);
    switch (i) {
    case 0:
      emit("push rdi");
      break;
    case 1:
      emit("push rsi");
      break;
    case 2:
      emit("push rdx");
      break;
    case 3:
      emit("push rcx");
      break;
    case 4:
      emit("push r8");
      break;
    case 5:
      emit("push r9");
      break;
    default:
      error("Too many parameters");
    }
    emit("pop rdi");
    emit("pop rax");
    emit("mov [rax], rdi");
    emit("push rdi");
  }
}

static void gen_ident(Node *node) {
  gen_lval(node);
  emit("pop rax");
  emit("mov rax, [rax]");
  emit("push rax");
}

static void store_args(Node *node) {
  for (int i = 0; i < node->args->len; i++) {
    Node *arg = node->args->data[i];
    gen(arg);
    emit("pop rax");
    switch (i) {
    case 0:
      emit("mov rdi, rax");
      break;
    case 1:
      emit("mov rsi, rax");
      break;
    case 2:
      emit("mov rdx, rax");
      break;
    case 3:
      emit("mov rcx, rax");
      break;
    case 4:
      emit("mov r8, rax");
      break;
    case 5:
      emit("mov r9, rax");
      break;
    default:
      error("Too many arguments");
    }
  }
}

static void emit_prologue(Node *func) {
  emit("push rbp");
  emit("mov rbp, rsp");
  emit("sub rsp, %d", func->func_vars->len * 8);
  load_args(func);
}

static void emit_epilogue() {
  emit("mov rsp, rbp");
  emit("pop rbp");
  emit("ret");
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
    }
    break;
  case ND_NULL:
    break;
  case ND_IDENT:
    gen_ident(node);
    return;
    break;
  case ND_CALL:
    store_args(node);
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
    }
    break;
  case ND_RETURN:
    gen(node->expr);
    emit("pop rax");
    emit_epilogue();
    break;
  case ND_IF: {
    char *then_label = bb_label();
    char *else_label = bb_label();
    char *last_label = bb_label();
    gen(node->cond);
    emit("pop rax");
    emit("cmp rax, 0");
    emit("jz %s", else_label);
    emit_label(then_label);
    gen(node->then);
    emit_label(else_label);
    emit_label(last_label);
    break;
  }
  case '=':
    assign(node->lhs, node->rhs);
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

  gen(node);
}
