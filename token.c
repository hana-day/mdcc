#include "mdcc.h"

static Map *keywords;
char *buf;
Token tokens[100];

static void load_keywords() {
  keywords = new_map();
  map_set(keywords, "int", (void *)TK_INT);
  map_set(keywords, "return", (void *)TK_RETURN);
}

bool istypename() {
  Token tok = tokens[pos];
  return tok.ty == TK_INT;
}

void tokenize() {
  load_keywords();

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
    if (isnondigit(*buf)) {
      int slen = 0;
      while (isnondigit(buf[slen]) || isdigit(buf[slen]))
        slen++;
      char *name = strndup(buf, slen);
      tokens[i].ty = (int)map_get_def(keywords, name, (void *)TK_IDENT);
      tokens[i].name = name;
      i++;
      buf += slen;
      continue;
    }
    if (*buf == '\'') {
      buf++;
      tokens[i].ty = TK_NUM;
      tokens[i].val = *buf;
      i++;
      buf++;
      if (*buf++ != '\'')
        error("Unclosed character literal.");
    }
    tokens[i].ty = *buf;
    buf++;
    i++;
  }
  tokens[i].ty = TK_EOF;
}
