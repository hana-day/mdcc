#include "mdcc.h"

int pos = 0;
int nvars;

static void usage() { error("Usage: mdcc <source file>"); }

int main(int argc, char **argv) {
  if (argc == 1)
    usage();

  if (strcmp(argv[1], "-test") == 0) {
    test();
    return 0;
  }

  buf = argv[1];

  Vector *tokens = tokenize();
  Node *node = parse(tokens);
  node = conv(node);
  gen_x64(node);
  return 0;
}
