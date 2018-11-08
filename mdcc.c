#include "mdcc.h"

char *buf;
Token tokens[100];
Map *keywords;
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

Token* new_token() {
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


Node *new_node_null() {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NULL;
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


static Node *expr_stmt() {
    if (consume(';'))
        return new_node_null();
    return add_expr();
}


static Node *stmt() {
    return expr_stmt();
}

static Node *compound_stmt() {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_COMP_STMT;
    Vector *v = new_vec();
    while (!consume('}')) {
        vec_push(v, stmt());
    }
    node->stmts = v;
    return node;
}


static Node *parse() {
    Node *node;
    if (consume('{')) {
        node = compound_stmt();
    } else {
        node = add_expr();
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
        if (isalpha(*buf) || *buf == '_') {
            int slen = 0;
            while (isalpha(buf[slen]) || isdigit(buf[slen]) || buf[slen] == '_')
                slen++;
            char *ident = strndup(buf, slen);
            tokens[i].ty = (int)map_get_def(keywords, ident, (void *)TK_IDENT);
            tokens[i].name = ident;
            buf += slen;
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
    case '+':
    case '-':
    case '*':
    case '/':
        gen_binary(node);
        break;
    default:
        err("Unknown node type %c", node->ty);
    }
}

int main(int argc, char **argv) {
    if (argc == 1)
        usage();

    if (strcmp(argv[1], "-test") == 0) {
        test();
        return 0;
    }

    load_keywords();

    buf = argv[1];
    tokenize();

    pos = 0;
    Node *node = parse();

    printf(".intel_syntax noprefix\n");
    printf(".global _main\n");
    printf("_main:\n");
    gen(node);
    printf("  ret\n");
    return 0;
}
