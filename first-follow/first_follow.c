#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXSYM 128
#define MAXPROD 256
#define MAXRHS 32
#define NAMELEN 32
#define LINELEN 512

static char name[MAXSYM][NAMELEN];
static int isNT[MAXSYM];
static int nsym = 0;

typedef struct { int lhs, rhs[MAXRHS], len; } Prod;
static Prod prods[MAXPROD];
static int nprod = 0;

static char firstSet [MAXSYM][MAXSYM];
static char followSet[MAXSYM][MAXSYM];
static int changed;

static int EPS, EOI, startSym = -1;

static void die(const char *msg, const char *arg)
{
   fprintf(stderr, "error: %s%s%s\n", msg, arg ? ": " : "", arg ? arg : "");
   exit(EXIT_FAILURE);
}

static int intern(const char *s)
{
   int i;
   for (i = 0; i < nsym; i++)
      if (strcmp(name[i], s) == 0) return i;
   if (nsym == MAXSYM)           die("too many distinct symbols", NULL);
   if (strlen(s) >= NAMELEN) die("symbol name too long", s);
   strcpy(name[nsym], s);
   return nsym++;
}
static void addSym(char *set, int s)
{
   if (!set[s]) { set[s] = 1; changed = 1; }
}

static void addAll(char *dst, const char *src)
{
   int i;
   for (i = 0; i < nsym; i++) if (src[i]) addSym(dst, i);
}

static void addAllButEps(char *dst, const char *src)
{
   int i;
   for (i = 0; i < nsym; i++) if (src[i] && i != EPS) addSym(dst, i);
}

static void addAlternative(int lhs, char *text)
{
   Prod *p;
   char *tok;
   int i, k;

    if (nprod == MAXPROD) die("too many productions", NULL);
    p = &prods[nprod++];
    p->lhs = lhs;
    p->len = 0;

    for (tok = strtok(text, " \t\r\n"); tok; tok = strtok(NULL, " \t\r\n")) {
       if (p->len == MAXRHS) die("right-hand side too long", name[lhs]);
       p->rhs[p->len++] = intern(tok);
    }

    if (p->len > 1) {
       for (i = 0, k = 0; i < p->len; i++)
          if (p->rhs[i] != EPS) p->rhs[k++] = p->rhs[i];
       p->len = k;
    }
    if (p->len == 0) { p->rhs[0] = EPS; p->len = 1; }
}

static void parseLine(char *line)
{
   char *arrow, *lhsTok, *rest, *bar;
   int A;
    arrow = strstr(line, "->");
    if (!arrow) die("missing '->' in line", line);
    *arrow = '\0';
    rest = arrow + 2;

    lhsTok = strtok(line, " \t\r\n");
    if (!lhsTok || strtok(NULL, " \t\r\n"))
       die("left-hand side must be exactly one symbol", NULL);

    A = intern(lhsTok);
    isNT[A] = 1;
    if (startSym < 0) startSym = A;

    for (;;) {
       bar = strchr(rest, '|');
       if (bar) *bar = '\0';
       addAlternative(A, rest);
       if (!bar) break;
       rest = bar + 1;
    }
}

static void readGrammar(FILE *in, int interactive)
{
   char line[LINELEN];

    if (interactive) {
       printf("Enter the grammar, one production per line.\n");
       printf(" format : A -> X Y | b | #    (symbols separated by spaces)\n");
       printf(" epsilon: #    finish input : blank line or the word 'end'\n\n");
    }

    for (;;) {
       if (interactive) { printf("> "); fflush(stdout); }
       if (!fgets(line, sizeof line, in)) break;

      if (strncmp(line + strspn(line, " \t"), "end", 3) == 0 &&
         line[strspn(line, " \t") + 3] <= ' ')
         break;

      if (line[strspn(line, " \t\r\n")] == '\0') {
         if (interactive) break;
         continue;
      }
      if (strncmp(line + strspn(line, " \t"), "//", 2) == 0) continue;
        parseLine(line);
    }

    if (nprod == 0) die("no productions were given", NULL);
    if (interactive) putchar('\n');
}

static void computeFirst(void)
{
   int i, p, X, A, allNullable;

    for (i = 0; i < nsym; i++)
       if (!isNT[i]) firstSet[i][i] = 1;

    do {
      changed = 0;
      for (p = 0; p < nprod; p++) {
         A = prods[p].lhs;
         allNullable = 1;
         for (i = 0; i < prods[p].len; i++) {
            X = prods[p].rhs[i];
            if (X == EPS) continue;
            addAllButEps(firstSet[A], firstSet[X]);
            if (!firstSet[X][EPS]) { allNullable = 0; break; }
         }
         if (allNullable) addSym(firstSet[A], EPS);
      }
    } while (changed);
}

static int firstOfSequence(const int *rhs, int from, int to, char *out)
{
   int i, j, X;
   memset(out, 0, MAXSYM);
   for (i = from; i < to; i++) {
      X = rhs[i];
      if (X == EPS) continue;
      for (j = 0; j < nsym; j++)
          if (firstSet[X][j] && j != EPS) out[j] = 1;
      if (!firstSet[X][EPS]) return 0;
   }
   return 1;
}

static void computeFollow(void)
{
    char tail[MAXSYM];
    int p, i, j, B, nullableTail;

    followSet[startSym][EOI] = 1;

    do {
      changed = 0;
      for (p = 0; p < nprod; p++) {
         for (i = 0; i < prods[p].len; i++) {
            B = prods[p].rhs[i];
            if (!isNT[B]) continue;

            nullableTail = firstOfSequence(prods[p].rhs, i + 1,
                                 prods[p].len, tail);
            for (j = 0; j < nsym; j++)
               if (tail[j]) addSym(followSet[B], j);

            if (nullableTail)
               addAll(followSet[B], followSet[prods[p].lhs]);
        }
      }
    } while (changed);
}

static void printSet(const char *label, int A, const char *set)
{
   int i, n = 0;
   printf("%-6s( %-4s ) = { ", label, name[A]);
   for (i = 0; i < nsym; i++)
      if (set[i]) printf("%s%s", n++ ? ", " : "", name[i]);
   printf(" }\n");
}

static void report(void)
{
   int i, p;

    printf("Grammar as parsed (start symbol: %s)\n", name[startSym]);
    for (p = 0; p < nprod; p++) {
       printf(" %s ->", name[prods[p].lhs]);
       for (i = 0; i < prods[p].len; i++) printf(" %s", name[prods[p].rhs[i]]);
       putchar('\n');
    }

    printf("\nNon-terminals:");
    for (i = 0; i < nsym; i++) if (isNT[i]) printf(" %s", name[i]);
    printf("\nTerminals :");
    for (i = 0; i < nsym; i++)
       if (!isNT[i] && i != EPS && i != EOI) printf(" %s", name[i]);

    printf("\n\n");
    for (i = 0; i < nsym; i++) if (isNT[i]) printSet("FIRST", i, firstSet[i]);
    putchar('\n');
    for (i = 0; i < nsym; i++) if (isNT[i]) printSet("FOLLOW", i, followSet[i]);
}

int main(int argc, char **argv)
{
   FILE *in = stdin;

    EPS = intern("#");
    EOI = intern("$");

    if (argc > 1) {
       in = fopen(argv[1], "r");
       if (!in) die("cannot open", argv[1]);
    }

    readGrammar(in, in == stdin);
    if (in != stdin) fclose(in);

    computeFirst();
    computeFollow();
    report();
    return 0;
}
