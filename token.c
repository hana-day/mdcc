#include "mdcc.h"

static Map *keywords;
char *buf;

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

static Position *new_position(Scanner *s) {
  Position *pos = malloc(sizeof(Position));
  pos->offset = s->offset;
  pos->line = s->line;
  return pos;
}

static Token *new_token(Scanner *s, int ty) {
  Token *tok = malloc(sizeof(Token));
  tok->ty = ty;
  tok->pos = new_position(s);
  return tok;
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
  map_set(keywords, "if", (void *)TK_IF);
  map_set(keywords, "else", (void *)TK_ELSE);
  map_set(keywords, "for", (void *)TK_FOR);
  map_set(keywords, "while", (void *)TK_WHILE);
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

static int scan_number(Scanner *s) {
  int n = 0;
  while (isdigit(s->ch)) {
    n = n * 10 + (s->ch - 48);
    next(s);
  }
  return n;
}

static int switch2(Scanner *s, int tok0, int tok1) {
  next(s);
  if (s->ch == '=') {
    next(s);
    return tok1;
  }
  return tok0;
}

static int switch3(Scanner *s, int tok0, int tok1, int tok2) {
  char ch = s->ch;
  next(s);
  if (s->ch == '=') {
    next(s);
    return tok1;
  }
  if (s->ch == ch) {
    next(s);
    return tok2;
  }
  return tok0;
}

static int switch4(Scanner *s, int tok0, int tok1, int tok2, int tok3) {
  char ch = s->ch;
  next(s);
  if (s->ch == '=') {
    next(s);
    return tok1;
  }
  if (s->ch == ch) {
    next(s);
    if (s->ch == '=') {
      next(s);
      return tok3;
    }
    return tok2;
  }
  return tok0;
}

Vector *tokenize() {
  load_keywords();

  Vector *tokens = new_vec();
  Scanner *s = new_scanner(buf);

  while (1) {
    char ch = s->ch;
    Token *tok = NULL;

    if (ch < 0) {
      tok = new_token(s, TK_EOF);
    } else if (isspace(ch)) {
      skipSpaces(s);
    } else if (isnondigit(ch)) {
      char *name = scan_ident(s);
      tok = new_token(s, (int)map_get_def(keywords, name, (void *)TK_IDENT));
      tok->name = name;
    } else if (isdigit(ch)) {
      tok = new_token(s, TK_NUM);
      tok->val = scan_number(s);
    } else if (ch == '\'') {
      next(s);
      tok = new_token(s, TK_NUM);
      tok->val = scan_char(s);
    } else if (ch == '+') {
      tok = new_token(s, switch2(s, '+', TK_ADD_EQ));
    } else if (ch == '-') {
      tok = new_token(s, switch2(s, '-', TK_SUB_EQ));
    } else if (ch == '*') {
      tok = new_token(s, switch2(s, '*', TK_MUL_EQ));
    } else if (ch == '/') {
      tok = new_token(s, switch2(s, '/', TK_DIV_EQ));
    } else if (ch == '!') {
      tok = new_token(s, switch2(s, '!', TK_NEQ));
    } else if (ch == '=') {
      tok = new_token(s, switch2(s, '=', TK_EQ));
    } else if (ch == '&') {
      tok = new_token(s, switch3(s, '&', TK_BAND_EQ, TK_AND));
    } else if (ch == '<') {
      tok = new_token(s, switch4(s, '<', TK_LEQ, TK_SHL, TK_SHL_EQ));
    } else if (ch == '>') {
      tok = new_token(s, switch4(s, '>', TK_GEQ, TK_SHR, TK_SHR_EQ));
    } else {
      tok = new_token(s, ch);
      next(s);
    }
    if (tok != NULL) {
      vec_push(tokens, tok);
      if (tok->ty == TK_EOF)
        break;
    }
  }
  return tokens;
}
