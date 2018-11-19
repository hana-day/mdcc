#include "mdcc.h"

static Map *keywords;
char *buf;
Token tokens[100];

typedef struct {
  char *src;  // source
  char ch;    // current character
  int offset; // character offset
  int line;   // line number
  int pos;    // reading position
} Scanner;

static void token_error(Scanner *s, char *msg) {
  fprintf(stderr, "Error at line:%d, offset:%d, character:'%c'\n", s->line,
          s->offset, s->src[s->pos]);
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

Scanner *new_scanner(char *src) {
  Scanner *s = malloc(sizeof(Scanner));
  s->src = strdup(src);
  s->ch = s->src[0];
  s->offset = 0;
  s->line = 1;
  s->pos = 0;
  return s;
}

static void next(Scanner *s) {
  if (s->pos >= strlen(s->src) - 1) {
    s->ch = -1;
  } else {
    if (s->ch == '\n') {
      s->line++;
      s->offset = 0;
    } else {
      s->offset++;
    }
    s->pos++;
    s->ch = s->src[s->pos];
  }
}

static void skipSpaces(Scanner *s) {
  while (s->ch == ' ' || s->ch == '\n' || s->ch == '\t' || s->ch == '\r')
    next(s);
}

static void load_keywords() {
  keywords = new_map();
  map_set(keywords, "int", (void *)TK_INT);
  map_set(keywords, "return", (void *)TK_RETURN);
}

bool istypename() {
  Token tok = tokens[pos];
  return tok.ty == TK_INT;
}

static char *scan_ident(Scanner *s) {
  int p = s->pos;
  while (isnondigit(s->ch) || isdigit(s->ch))
    next(s);
  return strndup(s->src + p, s->pos - p);
}

static char scan_char(Scanner *s) {
  char chr = s->ch;
  next(s);
  if (s->ch != '\'')
    token_error(s, "Unclosed character literal.");
  next(s);
  return chr;
}

void tokenize() {
  load_keywords();
  Scanner *s = new_scanner(buf);
  int i = 0;

  while (1) {
    char ch = s->ch;

    if (ch < 0) {
      tokens[i].ty = TK_EOF;
      break;
    } else if (isspace(ch)) {
      skipSpaces(s);
    } else if (isnondigit(ch)) {
      char *name = scan_ident(s);
      tokens[i].ty = (int)map_get_def(keywords, name, (void *)TK_IDENT);
      tokens[i].name = name;
      i++;
    } else if (isdigit(ch)) {
      tokens[i].ty = TK_NUM;
      tokens[i].val = ch - 48;
      next(s);
      i++;
    } else if (ch == '\'') {
      next(s);
      tokens[i].ty = TK_NUM;
      tokens[i].val = scan_char(s);
      i++;
    } else {
      tokens[i].ty = ch;
      i++;
      next(s);
    }
  }
}
