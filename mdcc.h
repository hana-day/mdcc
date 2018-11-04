#ifndef __MDCC_H__
#define __MDCC_H__

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

enum {
    TK_NUM = 256,
    TK_EOF,
};

enum {
    ND_NUM = 256,
};


typedef struct {
    int ty; // Token type
    char *name;  // Operator
    int val; // Number value
} Token;


typedef struct Node {
    int ty; // Node type
    struct Node *lhs;
    struct Node *rhs;
    int val;
} Node;


#endif
