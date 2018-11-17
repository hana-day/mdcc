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
  gen_x64(node);
  return 0;
}
