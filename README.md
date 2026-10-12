# Compiler Design Labs

C programs from a university compiler design course. Each builds with `gcc`. Most read from stdin; the loop-interchange program takes the matrix size as an argument.

| Directory | Program |
|---|---|
| `first-follow/` | FIRST and FOLLOW set computation for a context-free grammar (`#` is epsilon) |
| `shift-reduce-parser/` | Shift-reduce parser for a user-supplied grammar. Backtracking search with a visited-state check. It is not an LR parser. Prints stack, input and action at every step |
| `three-address-code/` | Three-address code generator for arithmetic expressions. Recursive descent with precedence, unary minus and right-associative `^` |
| `code-optimization/` | Loop interchange on matrix multiplication (ijk to ikj) with timing and a result check against the naive version |

## Build and run

```
gcc -Wall -o first_follow first-follow/first_follow.c
gcc -Wall -o shift_reduce shift-reduce-parser/shift_reduce.c
gcc -Wall -o tac three-address-code/tac_generator.c
```

### FIRST / FOLLOW
```
E -> T E1
E1 -> + T E1 | #
T -> F T1
T1 -> * F T1 | #
F -> ( E ) | id
```
End input with a blank line.

### Shift-reduce
Grammar, one production per line (`|` allowed), blank line, then the input tokens separated by spaces:
```
E -> E + T | T
T -> T * F | F
F -> ( E ) | id

id + id * id
```

### Three-address code
```
a = b + c * d - e / f
```
```
t1 = c * d
t2 = b + t1
t3 = e / f
t4 = t2 - t3
a = t4
```

### Loop interchange
Prints the naive (ijk) and interchanged (ikj) times, the speedup, and the maximum absolute difference between the two results. The speedup depends on the machine and the matrix size.

Build and run:

    gcc -O2 -o loop_interchange code-optimization/loop_interchange.c -lm
    ./loop_interchange 1024
