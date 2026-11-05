%code requires {
    #include "ast.h"
    #include "interp.h"
}

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

int yylex(void);
void yyerror(const char *s);
extern int yylineno;

Node *root = NULL;

static Node *binop(const char *op, Node *l, Node *r) {
    return add_kid(add_kid(new_node(N_BINOP, op, 0), l), r);
}
%}

%union {
    int   num;
    char *str;
    Node *node;
}

%token <num> NUM
%token <str> ID
%token KW_INT KW_IF KW_ELSE KW_WHILE KW_FOR KW_RETURN
%token EQ NE LE GE

%type <node> program external func params param_list
%type <node> stmt block stmt_list simple_opt
%type <node> expr arg_list arg_list_ne

%nonassoc LOWER_THAN_ELSE
%nonassoc KW_ELSE

%right '='
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/'
%right UMINUS

%%

program
    : /* порожньо */   { $$ = new_node(N_PROGRAM, NULL, 0); root = $$; }
    | program external { $$ = add_kid($1, $2); }
    ;

external
    : func
    | stmt
    ;

func
    : KW_INT ID '(' params ')' block
        { $$ = add_kid(add_kid(new_node(N_FUNC, $2, 0), $4), $6); free($2); }
    ;

params
    : /* порожньо */  { $$ = new_node(N_PARAMS, NULL, 0); }
    | param_list
    ;

param_list
    : KW_INT ID
        { $$ = add_kid(new_node(N_PARAMS, NULL, 0), new_node(N_VAR, $2, 0)); free($2); }
    | param_list ',' KW_INT ID
        { $$ = add_kid($1, new_node(N_VAR, $4, 0)); free($4); }
    ;

block
    : '{' stmt_list '}'  { $$ = $2; }
    ;

stmt_list
    : /* порожньо */  { $$ = new_node(N_BLOCK, NULL, 0); }
    | stmt_list stmt  { $$ = add_kid($1, $2); }
    ;

stmt
    : KW_INT ID ';'
        { $$ = new_node(N_DECL, $2, 0); free($2); }
    | KW_INT ID '=' expr ';'
        { $$ = add_kid(new_node(N_DECL, $2, 0), $4); free($2); }
    | expr ';'
        { $$ = add_kid(new_node(N_EXPR_STMT, NULL, 0), $1); }
    | block
    | KW_IF '(' expr ')' stmt %prec LOWER_THAN_ELSE 

        { $$ = add_kid(add_kid(new_node(N_IF, NULL, 0), $3), $5); }
    | KW_IF '(' expr ')' stmt KW_ELSE stmt
        { $$ = add_kid(add_kid(add_kid(new_node(N_IF, NULL, 0), $3), $5), $7); }
    | KW_WHILE '(' expr ')' stmt
        { $$ = add_kid(add_kid(new_node(N_WHILE, NULL, 0), $3), $5); }
    | KW_FOR '(' simple_opt ';' simple_opt ';' simple_opt ')' stmt
        { $$ = add_kid(add_kid(add_kid(add_kid(new_node(N_FOR, NULL, 0), $3), $5), $7), $9); }
    | KW_RETURN expr ';'
        { $$ = add_kid(new_node(N_RETURN, NULL, 0), $2); }
    ;

simple_opt
    : /* порожньо */  { $$ = new_node(N_EMPTY, NULL, 0); }
    | expr
    ;

expr
    : NUM             { $$ = new_node(N_NUM, NULL, $1); }
    | ID              { $$ = new_node(N_VAR, $1, 0); free($1); }
    | ID '=' expr     { $$ = add_kid(new_node(N_ASSIGN, $1, 0), $3); free($1); }
    | ID '(' arg_list ')'
        { $$ = $3; $$->type = N_CALL; $$->str = strdup($1); free($1); }
    | expr '+' expr   { $$ = binop("+",  $1, $3); }
    | expr '-' expr   { $$ = binop("-",  $1, $3); }
    | expr '*' expr   { $$ = binop("*",  $1, $3); }
    | expr '/' expr   { $$ = binop("/",  $1, $3); }
    | expr EQ expr    { $$ = binop("==", $1, $3); }
    | expr NE expr    { $$ = binop("!=", $1, $3); }
    | expr '<' expr   { $$ = binop("<",  $1, $3); }
    | expr '>' expr   { $$ = binop(">",  $1, $3); }
    | expr LE expr    { $$ = binop("<=", $1, $3); }
    | expr GE expr    { $$ = binop(">=", $1, $3); }
    | '-' expr %prec UMINUS
        { $$ = add_kid(new_node(N_UNOP, "-", 0), $2); }
    | '(' expr ')'    { $$ = $2; }
    ;

arg_list
    : /* порожньо */  { $$ = new_node(N_ARGS, NULL, 0); }
    | arg_list_ne
    ;

arg_list_ne
    : expr                    { $$ = add_kid(new_node(N_ARGS, NULL, 0), $1); }
    | arg_list_ne ',' expr    { $$ = add_kid($1, $3); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Рядок %d: %s\n", yylineno, s);
}

int main(int argc, char **argv) {
    int show_ast = 0, run = 1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ast") == 0)    show_ast = 1;
        if (strcmp(argv[i], "--no-run") == 0) run = 0;
    }
    if (yyparse() != 0) return 1;
    if (show_ast) print_ast(root, 0);
    if (run) return interpret(root);
    return 0;
}
