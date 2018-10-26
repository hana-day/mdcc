#include "mdcc.h"


void err(char *fmt, ...) {
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


int main(int argc, char **argv) {
    if (argc == 1)
        usage();

    printf(".intel_syntax noprefix\n");
    printf(".global _main\n");
    printf("_main:\n");
    printf("mov rax, %d\n", atoi(argv[1]));
    printf("ret\n");

    return 0;
}
