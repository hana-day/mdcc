#include "mdcc.h"

static int nlabel = 1;

enum {
  RAX = 0,
  RDI,
  RSI,
  RDX,
  RCX,
  R8,
  R9,
  R10,
  R11,
  RBP,
  RSP,
  RBX,
  R12,
  R13,
  R14,
  R15,
};

char *regs64[] = {"rax", "rdi", "rsi", "rdx", "rcx", "r8",  "r9",  "r10",
                  "r11", "rbp", "rsp", "rbx", "r12", "r13", "r14", "r15"};
char *regs32[] = {"eax",  "edi", "esi", "edx", "ecx",  "r8d",  "r9d",  "r10d",
                  "r11d", "ebp", "esp", "ebx", "r12d", "r13d", "r14d", "r15d"};
char *regs8[] = {"al",   "dil", "sil", "dl", "cl",   "r8b",  "r9b",  "r10b",
                 "r11b", "bpl", "spl", "bl", "r12b", "r13b", "r14b", "r15b"};

static char *reg(int r, int size) {
  if (size == 1)
    return regs8[r];
  else if (size == 4)
    return regs32[r];
  else if (size == 8)
    return regs64[r];
  else
    error("Unsupported register size %d", size);
}

static void emit(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printf("\t");
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
}

static void emit_label(char *s) { printf("%s:\n", s); }

static char *bb_label() { return format("MDCC_BB_%d", nlabel++); }

static void emit_directive(char *s) { printf(".%s\n", s); }

static void emit_conv_to_full(int r, int size) {
  if (size != 8 && size != 4)
    emit("movzx %s, %s", reg(r, 8), reg(r, size));
}

static void emit_push(int r, int size) {
  emit_conv_to_full(r, size);
  emit("push %s", reg(r, 8));
}

static void gen(Node *node);

static void gen_binary(Node *node) {
  gen(node->lhs);
  gen(node->rhs);

  int sz = node->cty->size;
  char *r11 = reg(R11, sz);
  char *rax = reg(RAX, sz);
  char *rdx = reg(RDX, sz);

  emit("pop r11");
  emit("pop rax");

  switch (node->ty) {
  case '+':
    emit("add %s, %s", rax, r11);
    break;
  case '-':
    emit("sub %s, %s", rax, r11);
    break;
  case '*':
    emit("imul %s, %s", rax, r11);
    break;
  case '/':
    emit("mov %s, 0", rdx);
    emit("div %s", r11);
    break;
  case '%':
    emit("mov %s, 0", rdx);
    emit("div %s", r11);
    emit("mov %s, %s", rax, rdx);
    break;
  case ND_SHL:
    emit("shl %s, %s", rax, r11);
    break;
  default:
    error("Unknown binary operator %d", node->ty);
  }
  emit_push(RAX, sz);
}

static void gen_cmp(Node *node) {
  char *true_label = bb_label();
  char *last_label = bb_label();
  gen(node->lhs);
  gen(node->rhs);
  emit("pop r11");
  emit("pop rax");
  int sz = node->cty->size;
  emit("cmp %s, %s", reg(RAX, sz), reg(R11, sz));
  switch (node->ty) {
  case ND_EQ:
    emit("je %s", true_label);
    break;
  case ND_NEQ:
    emit("jne %s", true_label);
    break;
  case '<':
    emit("jl %s", true_label);
    break;
  case '>':
    emit("jg %s", true_label);
    break;
  default:
    error("Unknown comparator %d", node->ty);
  }
  emit("push 0");
  emit("jmp %s", last_label);
  emit_label(true_label);
  emit("push 1");
  emit_label(last_label);
}

static void gen_logical(Node *node) {
  char *true_label = bb_label();
  char *false_label = bb_label();
  char *last_label = bb_label();

  // If the first operand compares equal to 0,
  // the second operand is not evaluated.
  switch (node->ty) {
  case ND_AND:
    gen(node->lhs);
    emit("pop rax");
    emit("cmp rax, 0");
    emit("je %s", false_label);
    gen(node->rhs);
    emit("pop rax");
    emit("cmp rax, 0");
    emit("je %s", false_label);
    break;
  case ND_OR:
    gen(node->lhs);
    emit("pop rax");
    emit("cmp rax, 0");
    emit("jne %s", true_label);
    gen(node->rhs);
    emit("pop rax");
    emit("cmp rax, 0");
    emit("jne %s", true_label);
    emit("jmp %s", false_label);
    break;
  default:
    error("Unknown operator %d", node->ty);
  }
  emit_label(true_label);
  emit("push 1");
  emit("jmp %s", last_label);
  emit_label(false_label);
  emit("push 0");
  emit_label(last_label);
}

static void gen_bitwise(Node *node) {
  gen(node->lhs);
  gen(node->rhs);
  emit("pop r11");
  emit("pop rax");
  int sz = node->cty->size;
  char *rax = reg(RAX, sz);
  char *r11 = reg(R11, sz);
  switch (node->ty) {
  case '&':
    emit("and %s, %s", rax, r11);
    break;
  case '|':
    emit("or %s, %s", rax, r11);
    break;
  case '^':
    emit("xor %s, %s", rax, r11);
    break;
  default:
    error("Unknown operator %d", node->ty);
  }
  emit_push(RAX, sz);
}

static void gen_shift(Node *node) {
  gen(node->lhs);
  gen(node->rhs);
  emit("pop rcx");
  emit("pop rax");
  int sz = node->cty->size;
  char *rax = reg(RAX, sz);
  switch (node->ty) {
  case ND_SHL:
    emit("shl %s, cl", rax);
    break;
  case ND_SHR:
    emit("shr %s, cl", rax);
    break;
  }
  emit_push(RAX, sz);
}

static void gen_lval(Node *node) {
  if (node->ty == ND_DEREF) {
    gen(node->expr);
    return;
  }
  if (node->ty == ND_IDENT) {
    emit("mov rax, rbp");
    emit("sub rax, %d", node->var->offset);
    emit("push rax");
    if (node->var->has_address) {
      emit("pop rax");
      emit("mov rax, [rax]");
      emit("push rax");
    }
    return;
  }
  error("Invalid lvalue %d.", node->ty);
}

static void gen_postfix_incdec(Node *node) {
  gen_lval(node->expr);
  gen(node->expr);
  emit("pop r11");
  // Save the value before postfix inc/dec into the register.
  emit("mov rcx, r11");

  int sz = node->cty->size;
  char *r11 = reg(R11, sz);

  switch (node->ty) {
  case ND_INC:
    emit("add %s, 1", r11);
    break;
  case ND_DEC:
    emit("sub %s, 1", r11);
    break;
  default:
    error("Unknown node type %d", node->ty);
  }
  emit("pop rax");
  emit("mov [rax], %s", r11);
  emit("push rcx");
}

static void assign(Node *node) {
  gen_lval(node->lhs);
  gen(node->rhs);

  emit("pop r11");
  emit("pop rax");

  int sz = node->cty->size;
  emit("mov [rax], %s", reg(R11, sz));
  emit_push(R11, sz);
}

static void gen_param_lval(Node *node) {
  assert(node->ty == ND_IDENT);
  emit("mov rax, rbp");
  emit("sub rax, %d", node->var->offset);
  emit("push rax");
}

static void load_args(Node *func) {
  for (int i = 0; i < func->params->len; i++) {
    Node *param = func->params->data[i];
    gen_param_lval(param);
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
    int sz;
    if (param->var->has_address)
      sz = 8;
    else
      sz = param->cty->size;
    emit("pop r11");
    emit("pop rax");
    emit("mov [rax], %s", reg(R11, sz));
    emit_push(R11, sz);
  }
}

static void gen_ident(Node *node) {
  gen_lval(node);
  emit("pop rax");
  int sz = node->cty->size;
  emit("mov %s, [rax]", reg(RAX, sz));
  emit_push(RAX, sz);
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
  int off = 0; // Offset from rbp
  for (int i = 0; i < func->func_vars->len; i++) {
    Var *var = func->func_vars->data[i];
    if (var->has_address) {
      off += 8;
      off = roundup(off, 8);
    } else {
      off += var->ty->size;
      off = roundup(off, var->ty->align);
    }
    var->offset = off;
  }
  emit("sub rsp, %d", roundup(off, 16));
  load_args(func);
}

static void emit_epilogue() {
  emit("mov rsp, rbp");
  emit("pop rbp");
  emit("ret");
}

static void gen(Node *node) {
  if (node == NULL)
    return;

  switch (node->ty) {
  case ND_NUM:
    emit("push %d", node->val);
    return;
  case ND_COMP_STMT:
    for (int i = 0; i < node->stmts->len; i++) {
      Node *stmt = node->stmts->data[i];
      if (stmt->ty == ND_NULL)
        continue;
      gen(stmt);
    }
    break;
  case ND_NULL:
    break;
  case ND_IDENT:
    gen_ident(node);
    break;
  case ND_CALL:
    store_args(node);
    emit("mov al, 0");
    emit("call _%s", node->name);
    emit("push rax");
    break;
  case ND_ADDR:
    gen_lval(node->expr);
    break;
  case ND_DEREF:
    gen(node->expr);
    emit("pop rax");
    emit("mov rax, [rax]");
    emit("push rax");
    break;
  case ND_ROOT:
    for (int i = 0; i < node->funcs->len; i++) {
      Node *func = node->funcs->data[i];
      emit_label(format("_%s", func->name));
      emit_prologue(func);
      gen(func->body);
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
    gen(node->els);
    emit_label(last_label);
    break;
  }
  case ND_FOR: {
    char *cond_label = bb_label();
    char *last_label = bb_label();
    gen(node->init);
    emit_label(cond_label);
    gen(node->cond);
    emit("pop rax");
    emit("cmp rax, 0");
    emit("jz %s", last_label);
    gen(node->body);
    gen(node->after);
    emit("jmp %s", cond_label);
    emit_label(last_label);
    break;
  }
  case ND_WHILE: {
    char *cond_label = bb_label();
    char *last_label = bb_label();
    emit_label(cond_label);
    gen(node->cond);
    emit("pop rax");
    emit("cmp rax, 0");
    emit("jz %s", last_label);
    gen(node->body);
    emit("jmp %s", cond_label);
    emit_label(last_label);
    break;
  }
  case ND_EQ:
  case ND_NEQ:
  case '<':
  case '>':
    gen_cmp(node);
    break;
  case '=':
    assign(node);
    break;
  case '+':
  case '-':
  case '*':
  case '/':
  case '%':
    gen_binary(node);
    break;
  case ND_SHL:
  case ND_SHR:
    gen_shift(node);
    break;
  case ND_AND:
  case ND_OR:
    gen_logical(node);
    break;
  case '&':
  case '|':
  case '^':
    gen_bitwise(node);
    break;
  case ND_INC:
  case ND_DEC:
    gen_postfix_incdec(node);
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
