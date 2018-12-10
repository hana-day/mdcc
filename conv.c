#include "mdcc.h"

static Node *arr_to_ptr(Node *node) {
  if (node->var == NULL ||
      (node->var->ty->ty != TY_ARR && node->var->prev_ty == NULL))
    return node;
  if (node->var->prev_ty == NULL) {
    node->var->prev_ty = node->var->ty;
    node->var->ty = ptr(node->var->ty->arr_of);
  }
  return new_node(ND_ADDR, new_node_null(), node);
}

/**
 * Convert multi-dimentional array indice to one-dimentional array index.
 *
 * Example:
 *   arr_index(`3x2 array type`, [2, 1]); => 5
 */
static int arr_index(Type *arrtype, Vector *indice) {
  if (indice->len > 2)
    error("Unsupported dimension array");
  if (indice->len == 1)
    return (int)indice->data[0];
  else
    return (arrtype->arr_of->len * (int)indice->data[0] + (int)indice->data[1]);
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
    return node;
  case ND_ADDR:
    node->rhs = walk(node->rhs);
    return node;
  case ND_DEREF:
    node->rhs = walk(node->rhs);
    return node;
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
  case ND_EQ:
  case ND_NEQ:
  case '=':
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    return node;
  case '+':
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    return node;
  case '-':
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    if (node->lhs->ty == ND_ADDR && node->lhs->rhs->ty == ND_IDENT &&
        node->lhs->rhs->var->ty->ty == TY_PTR) {
      Node *node2 = new_node(
          '*', node->rhs, new_node_num(node->lhs->rhs->var->ty->ptr_to->size));
      node->rhs = node2;
    }
    return node;
  case '*':
  case '/':
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    return node;
  }
  return NULL;
}

Node *conv(Node *node) {
  node = walk(node);
  return node;
}
