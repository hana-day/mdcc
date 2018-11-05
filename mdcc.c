#include "mdcc.h"

char *buf;
Token tokens[100];
int pos;

__attribute__((noreturn)) void err(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    exit(1);
}


void usage() {
    err("Usage: mdcc <source file>");
}


Token* new_token() {
    Token *tok = malloc(sizeof(Token));
    return tok;
}


Node *new_node(int ty, Node *lhs, Node *rhs) {
    Node *node = malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}


Node *new_node_num(int val) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}


static Node *add_expr();

static Node *cast_expr() {
    if (tokens[pos].ty == TK_NUM) {
        return new_node_num(tokens[pos++].val);
    }
    if (tokens[pos].ty == '(') {
        pos++;
        Node *node = add_expr();
        if (tokens[pos].ty != ')') {
            err("No closing parenthesis.");
        }
        pos++;
        return node;
    }
    err("Unexpected token %d.\n", tokens[pos].ty);
}


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


static Node *add_expr() {
    Node *lhs = mul_expr();
    int ty = tokens[pos].ty;
    if (ty == '+' || ty == '-') {
        pos++;
        return new_node(ty, lhs, add_expr());
    } else {
        return lhs;
    }
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
        tokens[i].ty = *buf;
        buf++;
        i++;
    }
    tokens[i].ty = TK_EOF;
}

void gen(Node *node) {
    if (node->ty == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }
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
    }
    printf("  push rax\n");
}

int main(int argc, char **argv) {
    if (argc == 1)
        usage();

    buf = argv[1];
    tokenize();

    pos = 0;
    Node *node = add_expr();

    printf(".intel_syntax noprefix\n");
    printf(".global _main\n");
    printf("_main:\n");
    gen(node);
    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}
