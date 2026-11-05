#ifndef AST_H
#define AST_H

typedef enum {
    N_PROGRAM, N_NUM, N_VAR, N_BINOP, N_UNOP,
    N_ASSIGN, N_DECL, N_EXPR_STMT,
    N_BLOCK, N_IF, N_WHILE, N_FOR, N_RETURN,
    N_FUNC, N_PARAMS, N_CALL, N_ARGS, N_EMPTY
} NodeType;

typedef struct Node {
    NodeType type;
    int num;
    char *str;
    struct Node **kids;
    int nkids, cap;
    int line;
} Node;

Node *new_node(NodeType type, const char *str, int num);
Node *add_kid(Node *parent, Node *kid);
const char *node_type_name(NodeType t);
void print_ast(Node *n, int depth);
void print_json(Node *n);

#endif
