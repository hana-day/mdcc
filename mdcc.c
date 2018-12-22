#include "mdcc.h"

int pos = 0;
int nvars;

static void usage() {
  error("Usage:\nmdcc -e <code>\nmdcc -f <source file>\nmdcc -test");
}

int main(int argc, char **argv) {
  if (argc == 1)
    usage();

  if (strcmp(argv[1], "-test") == 0) {
    test();
    return 0;
  }

  if (argc != 3)
    usage();

  if (strcmp(argv[1], "-e") == 0) {
    buf = argv[2];
  } else if (strcmp(argv[1], "-f") == 0) {
    FILE *f = fopen(argv[2], "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    buf = malloc(fsize + 1);
    fread(buf, fsize, 1, f);
    fclose(f);
  } else {
    usage();
  }

  Vector *tokens = tokenize();
  Node *node = parse(tokens);
  node = conv(node);
  gen_x64(node);
  return 0;
}
