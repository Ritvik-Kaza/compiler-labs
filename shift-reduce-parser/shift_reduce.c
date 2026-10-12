#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXRULES        100
#define MAXRHS          20
#define MAXSYMLEN       20
#define MAXSTACK        500
#define MAXINPUT        500
#define MAXSTEPS        4000

typedef struct {
    char lhs[MAXSYMLEN];
    char rhs[MAXRHS][MAXSYMLEN];
    int rhsLen;
} Rule;

Rule rules[MAXRULES];
int numRules = 0;
char startSymbol[MAXSYMLEN] = "";

char nonterminals[MAXRULES][MAXSYMLEN];
int numNT = 0;

char stack[MAXSTACK][MAXSYMLEN];
char inputTok[MAXINPUT][MAXSYMLEN];
int ntok = 0;

typedef struct {
    char stackStr[600];
    char inputStr[600];
    char action[100];
} Step;
Step trace[MAXSTEPS];
int traceLen = 0;

long long nodesVisited = 0;
#define NODE_LIMIT 2000000

#define MAXVISITED 200000
char visited[MAXVISITED][700];
int nVisited = 0;

int isVisited(char *key){
    for(int i=0;i<nVisited;i++) if(strcmp(visited[i], key)==0) return 1;
    if(nVisited < MAXVISITED) strcpy(visited[nVisited++], key);
    return 0;
}

int isNonterminal(char *s){
    for(int i=0;i<numNT;i++) if(strcmp(nonterminals[i], s)==0) return 1;
    return 0;
}

void trim(char *s){
    int n = strlen(s);
    while(n>0 && (s[n-1]=='\n' || s[n-1]=='\r' || s[n-1]==' ' || s[n-1]=='\t')) s[--n]='\0';
    int start=0;
    while(s[start]==' ' || s[start]=='\t') start++;
    if(start>0) memmove(s, s+start, strlen(s+start)+1);
}

int lineIsBlank(char *s){
    for(int i=0;s[i];i++) if(!isspace((unsigned char)s[i])) return 0;
    return 1;
}

void readGrammar(){
     printf("Enter the grammar (one production per line, e.g. E -> E + T).\n");
     printf("Use '|' for alternatives on the same line if you like. Leave a blank line to finish:\n");
     char line[400];
     while(fgets(line, sizeof(line), stdin)){
         trim(line);
         if(lineIsBlank(line)) break;

        char *arrow = strstr(line, "->");
        if(!arrow) continue;
        *arrow = '\0';
        char lhs[MAXSYMLEN];
        sscanf(line, "%s", lhs);
        char *rest = arrow + 2;

        char *saveAlt;
        char *alt = strtok_r(rest, "|", &saveAlt);
        while(alt){
            Rule r;
            strcpy(r.lhs, lhs);
            r.rhsLen = 0;
            char altCopy[300];
            strcpy(altCopy, alt);
            char *tok = strtok(altCopy, " \t");
            while(tok && r.rhsLen < MAXRHS){
                strcpy(r.rhs[r.rhsLen++], tok);
                tok = strtok(NULL, " \t");
            }
            if(r.rhsLen > 0) rules[numRules++] = r;
            alt = strtok_r(NULL, "|", &saveAlt);
        }
    }
    if(numRules == 0){ printf("No grammar entered.\n"); return; }
    strcpy(startSymbol, rules[0].lhs);
    for(int i=0;i<numRules;i++){
        if(!isNonterminal(rules[i].lhs))
            strcpy(nonterminals[numNT++], rules[i].lhs);
    }
}

void readInput(){
    printf("Enter the input string (tokens separated by spaces, e.g. id + id * id): ");
    char line[400];
    fgets(line, sizeof(line), stdin);
    trim(line);
    char *tok = strtok(line, " \t");
    while(tok){
        strcpy(inputTok[ntok++], tok);
        tok = strtok(NULL, " \t");
    }
    strcpy(inputTok[ntok++], "$");
}

void snapshotStack(char out[], int sp){
    out[0]='\0';
    for(int i=0;i<sp;i++) strcat(out, stack[i]);
}
void snapshotInput(char out[], int ip){
    out[0]='\0';
    for(int i=ip;i<ntok;i++) strcat(out, inputTok[i]);
}

int matchesRHS(Rule *r, int sp){
    if(sp < r->rhsLen) return 0;
    for(int i=0;i<r->rhsLen;i++)
        if(strcmp(stack[sp - r->rhsLen + i], r->rhs[i]) != 0) return 0;
    return 1;
}

char *rhsString(Rule *r, char *buf){
    buf[0]='\0';
    for(int i=0;i<r->rhsLen;i++){
        strcat(buf, r->rhs[i]);
        if(i != r->rhsLen-1) strcat(buf, " ");
    }
    return buf;
}

int solve(int sp, int ip){
    if(++nodesVisited > NODE_LIMIT) return 0;

    if(sp==2 && strcmp(stack[1], startSymbol)==0 && ip==ntok-1){
        snapshotStack(trace[traceLen].stackStr, sp);
        snapshotInput(trace[traceLen].inputStr, ip);
        strcpy(trace[traceLen].action, "Accept");
        traceLen++;
        return 1;
    }
    char key[1300];
    char sKey[600], iKey[600];
    snapshotStack(sKey, sp);
    snapshotInput(iKey, ip);
    sprintf(key, "%s#%s", sKey, iKey);
    if(isVisited(key)) return 0;

    char backup[MAXSTACK][MAXSYMLEN];

    for(int r=0; r<numRules; r++){
        if(matchesRHS(&rules[r], sp)){
            memcpy(backup, stack, sizeof(stack));

            snapshotStack(trace[traceLen].stackStr, sp);
            snapshotInput(trace[traceLen].inputStr, ip);
            char rb[100];
            sprintf(trace[traceLen].action, "Reduce %s -> %s", rules[r].lhs,
rhsString(&rules[r], rb));
            traceLen++;

            int newsp = sp - rules[r].rhsLen;
            strcpy(stack[newsp], rules[r].lhs);

            if(solve(newsp+1, ip)) return 1;
            traceLen--;
            memcpy(stack, backup, sizeof(stack));
        }
    }

    if(ip < ntok-1){
        memcpy(backup, stack, sizeof(stack));

        snapshotStack(trace[traceLen].stackStr, sp);
        snapshotInput(trace[traceLen].inputStr, ip);
        strcpy(trace[traceLen].action, "Shift");
        traceLen++;

        strcpy(stack[sp], inputTok[ip]);
        if(solve(sp+1, ip+1)) return 1;
        traceLen--;
        memcpy(stack, backup, sizeof(stack));
    }
    return 0;
}

int main(){
    readGrammar();
    if(numRules==0) return 0;
    readInput();

    strcpy(stack[0], "$");
    int ok = solve(1, 0);

    printf("\n%-25s %-25s %-25s\n", "Stack", "Input Buffer", "Action");
    for(int k=0;k<75;k++) printf("-");
    printf("\n");
    for(int i=0;i<traceLen;i++)
        printf("%-25s %-25s %-25s\n", trace[i].stackStr, trace[i].inputStr,
trace[i].action);

    if(!ok) printf("\nReject: no valid shift-reduce derivation found for this input.\n");
    return 0;
}
