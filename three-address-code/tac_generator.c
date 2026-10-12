#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXTOK 64

typedef struct { char s[MAXTOK]; } Name;

static const char *p;
static int tcount = 0;
static int nops = 0;

static Name expr(void);

static void skip(void) { while (isspace((unsigned char)*p)) p++; }

static void error(const char *msg) {
    fprintf(stderr, "Error: %s at \"%s\"\n", msg, *p ? p : "<end>");
    exit(1);
}

static Name newtemp(void) {
    Name n;
    snprintf(n.s, MAXTOK, "t%d", ++tcount);
    return n;
}

static Name emit(const char *op, Name a, const Name *b) {
    Name t = newtemp();
    if (b) printf("%s = %s %s %s\n", t.s, a.s, op, b->s);
    else    printf("%s = %s%s\n", t.s, op, a.s);
    nops++;
    return t;
}

static Name primary(void) {
    Name n;
    skip();
    if (*p == '(') {
        p++;
        n = expr();
        skip();
        if (*p != ')') error("expected ')'");
        p++;
        return n;
    }
    if (isalpha((unsigned char)*p) || *p == '_') {
        int i = 0;
        while ((isalnum((unsigned char)*p) || *p == '_') && i < MAXTOK - 1)
n.s[i++] = *p++;
        n.s[i] = '\0';
        return n;
    }
    if (isdigit((unsigned char)*p) || *p == '.') {
        int i = 0, dots = 0;
        while ((isdigit((unsigned char)*p) || *p == '.') && i < MAXTOK - 1) {
             if (*p == '.' && ++dots > 1) error("malformed number");
             n.s[i++] = *p++;
        }
        n.s[i] = '\0';
        return n;
    }
    error("expected operand");
    return n;
}

static Name unary(void);

static Name power(void) {
    Name base = primary();
    skip();
    if (*p == '^') {
        p++;
        Name exp = unary();
        return emit("^", base, &exp);
    }
    return base;
}

static Name unary(void) {
    skip();
    if (*p == '-') {
        p++;
        Name x = unary();
        return emit("-", x, NULL);
    }
    return power();
}

static Name term(void) {
    Name l = unary();
    for (;;) {
        skip();
        if (*p == '*' || *p == '/') {
            char op[2] = { *p++, '\0' };
            Name r = unary();
            l = emit(op, l, &r);
        } else return l;
    }
}

static Name expr(void) {
    Name l = term();
    for (;;) {
        skip();
        if (*p == '+' || *p == '-') {
            char op[2] = { *p++, '\0' };
            Name r = term();
            l = emit(op, l, &r);
        } else return l;
    }
}

int main(void) {
    char line[512];
      printf("Enter an arithmetic expression (e.g. a = b + c * d - e / f):\n> ");
      if (!fgets(line, sizeof line, stdin)) return 1;
      line[strcspn(line, "\r\n")] = '\0';

      p = line;
      char lhs[MAXTOK] = "";

    const char *save = p;
    skip();
    if (isalpha((unsigned char)*p) || *p == '_') {
        int i = 0;
        const char *q = p;
        while ((isalnum((unsigned char)*q) || *q == '_') && i < MAXTOK - 1)
lhs[i++] = *q++;
        lhs[i] = '\0';
        while (isspace((unsigned char)*q)) q++;
        if (*q == '=') p = q + 1; else { lhs[0] = '\0'; p = save; }
    } else p = save;

      printf("\nThree-Address Code:\n");
      Name r = expr();
      skip();
      if (*p) error("unexpected character");

      if (lhs[0]) printf("%s = %s\n", lhs, r.s);
      else if (nops == 0) printf("result = %s\n", r.s);

      return 0;
}
