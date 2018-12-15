#include "mdcc.h"

static Node *arr_to_ptr(Node *node) {
  if (node->cty->ty != TY_ARR)
    return node;
  Node *node2 = new_node(ND_ADDR, new_node_null(), node);
  node2->cty = ptr(node->cty->arr_of);
  return node2;
}

static Node *walk(Node *node) {
  if (node == NULL)
    return NULL;
  switch (node->ty) {
  case ND_NUM:
    return node;
  case ND_COMP_STMT:
    for (int i = 0; i < node->stmts->len; i++) {
      Node *stmt = node->stmts->data[i];
      node->stmts->data[i] = (void *)walk(stmt);
    }
    return node;
  case ND_NULL:
    return node;
  case ND_IDENT:
    return arr_to_ptr(node);
  case ND_CALL:
    for (int i = 0; i < node->args->len; i++)
      node->args->data[i] = walk(node->args->data[i]);
    return node;
  case ND_ADDR:
    node->rhs = walk(node->rhs);
    node->cty = ptr(node->rhs->cty);
    return node;
  case ND_DEREF:
    node->rhs = walk(node->rhs);
    if (node->rhs->cty->ty != TY_PTR)
      error("operand for dereference must be a pointer");
    node->cty = node->rhs->cty->ptr_to;
    return arr_to_ptr(node);
  case ND_ROOT:
    for (int i = 0; i < node->funcs->len; i++) {
      Node *func = node->funcs->data[i];
      func->body = walk(func->body);
      node->funcs->data[i] = (void *)func;
    }
    return node;
    break;
  case ND_RETURN:
    node->expr = walk(node->expr);
    return node;
  case ND_IF:
    node->cond = walk(node->cond);
    node->then = walk(node->then);
    node->els = walk(node->els);
    return node;
  case ND_FOR:
    node->init = walk(node->init);
    node->cond = walk(node->cond);
    node->after = walk(node->after);
    node->body = walk(node->body);
    return node;
  case ND_WHILE:
    node->cond = walk(node->cond);
    node->body = walk(node->body);
    return node;
  case ND_EQ:
  case ND_NEQ:
  case '<':
  case '>':
  case '=':
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    node->cty = node->lhs->cty;
    return node;
  case '+':
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    node->cty = new_int_ty();
    return node;
  case '-':
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    if (node->lhs->cty->ty == TY_PTR) {
      node->rhs =
          new_node('*', node->rhs, new_node_num(node->lhs->cty->ptr_to->size));
      node->cty = node->lhs->cty;
    } else {
      node->cty = new_int_ty();
    }
    return node;
  case '*':
  case '/':
  case '%':
  case ND_SHL:
  case ND_SHR:
  case ND_AND:
  case ND_OR:
  case '&':
  case '|':
  case '^':
  case ND_INC:
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    node->cty = new_int_ty();
    return node;
  }
  return NULL;
}

Node *conv(Node *node) {
  node = walk(node);
  return node;
}
