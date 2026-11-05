#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

extern int yylineno;

Node *new_node(NodeType type, const char *str, int num) {
    Node *n = calloc(1, sizeof(Node));
    n->type = type;
    n->str  = str ? strdup(str) : NULL;
    n->num  = num;
    n->line = yylineno;
    return n;
}

Node *add_kid(Node *p, Node *k) {
    if (p->nkids == p->cap) {
        p->cap = p->cap ? p->cap * 2 : 4;
        p->kids = realloc(p->kids, p->cap * sizeof(Node *));
    }
    p->kids[p->nkids++] = k;
    return p;
}

const char *node_type_name(NodeType t) {
    static const char *names[] = {
        "PROGRAM", "NUM", "VAR", "BINOP", "UNOP",
        "ASSIGN", "DECL", "EXPR_STMT",
        "BLOCK", "IF", "WHILE", "FOR", "RETURN",
        "FUNC", "PARAMS", "CALL", "ARGS", "EMPTY"
    };
    return names[t];
}

void print_ast(Node *n, int depth) {
    if (!n) return;
    for (int i = 0; i < depth; i++) printf("  ");
    printf("%s", node_type_name(n->type));
    if (n->type == N_NUM) printf(" %d", n->num);
    else if (n->str)      printf(" %s", n->str);
    printf("\n");
    for (int i = 0; i < n->nkids; i++)
        print_ast(n->kids[i], depth + 1);
}
