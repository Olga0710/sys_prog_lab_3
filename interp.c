#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "interp.h"

/* Середовища (області видимості) */
typedef struct Var {
    char *name;
    int val;
    struct Var *next;
} Var;

typedef struct Env {
    Var *vars;
    struct Env *parent;
} Env;

static Env *global_env;
static Node *funcs[256];
static int nfuncs = 0;
static int ret_val = 0;

static Env *env_new(Env *parent) {
    Env *e = calloc(1, sizeof(Env));
    e->parent = parent;
    return e;
}

static void env_free(Env *e) {
    Var *v = e->vars;
    while (v) {
        Var *n = v->next;
        free(v->name);
        free(v);
        v = n;
    }
    free(e);
}

static void env_declare(Env *e, const char *name, int val) {
    Var *v = malloc(sizeof(Var));
    v->name = strdup(name);
    v->val = val;
    v->next = e->vars;
    e->vars = v;
}

static Var *env_lookup(Env *e, const char *name) {
    for (; e; e = e->parent)
        for (Var *v = e->vars; v; v = v->next)
            if (strcmp(v->name, name) == 0) return v;
    return NULL;
}

static void rt_error(Node *n, const char *msg, const char *name) {
    fprintf(stderr, "Помилка виконання (рядок %d): %s %s\n",
            n->line, msg, name ? name : "");
    exit(1);
}

/* Прототипи */
static int eval(Node *n, Env *e);
static int exec(Node *n, Env *e);

/* Виклик функції */
static int call_function(Node *call, Env *e) {
    int nargs = call->nkids;
    int args[32];
    if (nargs > 32) rt_error(call, "забагато аргументів у виклику", call->str);

    for (int i = 0; i < nargs; i++)
        args[i] = eval(call->kids[i], e);

    if (strcmp(call->str, "print") == 0) {
        if (nargs != 1) rt_error(call, "print приймає 1 аргумент", NULL);
        printf("%d\n", args[0]);
        return args[0];
    }

    Node *f = NULL;
    for (int i = 0; i < nfuncs; i++)
        if (strcmp(funcs[i]->str, call->str) == 0) { f = funcs[i]; break; }
    if (!f) rt_error(call, "невизначена функція", call->str);

    Node *params = f->kids[0];
    Node *body   = f->kids[1];
    if (params->nkids != nargs)
        rt_error(call, "невірна кількість аргументів для", call->str);

    Env *fenv = env_new(global_env);
    for (int i = 0; i < nargs; i++)
        env_declare(fenv, params->kids[i]->str, args[i]);

    ret_val = 0;
    exec(body, fenv);
    int result = ret_val;
    env_free(fenv);
    return result;
}

/* Обчислення виразів */
static int eval(Node *n, Env *e) {
    switch (n->type) {
    case N_NUM:
        return n->num;
    case N_VAR: {
        Var *v = env_lookup(e, n->str);
        if (!v) rt_error(n, "невизначена змінна", n->str);
        return v->val;
    }
    case N_ASSIGN: {
        int val = eval(n->kids[0], e);
        Var *v = env_lookup(e, n->str);
        if (!v) rt_error(n, "присвоєння невизначеній змінній", n->str);
        v->val = val;
        return val;
    }
    case N_UNOP:
        return -eval(n->kids[0], e);
    case N_BINOP: {
        int a = eval(n->kids[0], e);
        int b = eval(n->kids[1], e);
        const char *op = n->str;
        if (!strcmp(op, "+"))  return a + b;
        if (!strcmp(op, "-"))  return a - b;
        if (!strcmp(op, "*"))  return a * b;
        if (!strcmp(op, "/")) {
            if (b == 0) rt_error(n, "ділення на нуль", NULL);
            return a / b;
        }
        if (!strcmp(op, "==")) return a == b;
        if (!strcmp(op, "!=")) return a != b;
        if (!strcmp(op, "<"))  return a < b;
        if (!strcmp(op, ">"))  return a > b;
        if (!strcmp(op, "<=")) return a <= b;
        if (!strcmp(op, ">=")) return a >= b;
        rt_error(n, "невідомий оператор", op);
    }
    case N_CALL:
        return call_function(n, e);
    default:
        rt_error(n, "вузол не є виразом:", node_type_name(n->type));
    }
    return 0;
}

/* Виконання операторів */
/* повертає 1, якщо виконано return */
static int exec(Node *n, Env *e) {
    switch (n->type) {
    case N_DECL: {
        int v = n->nkids ? eval(n->kids[0], e) : 0;
        env_declare(e, n->str, v);
        return 0;
    }
    case N_EXPR_STMT:
        eval(n->kids[0], e);
        return 0;
    case N_BLOCK: {
        Env *inner = env_new(e);
        int r = 0;
        for (int i = 0; i < n->nkids; i++) {
            r = exec(n->kids[i], inner);
            if (r) break;
        }
        env_free(inner);
        return r;
    }
    case N_IF:
        if (eval(n->kids[0], e)) return exec(n->kids[1], e);
        if (n->nkids == 3)       return exec(n->kids[2], e);
        return 0;
    case N_WHILE:
        while (eval(n->kids[0], e))
            if (exec(n->kids[1], e)) return 1;
        return 0;
    case N_FOR: {
        Node *init = n->kids[0], *cond = n->kids[1];
        Node *step = n->kids[2], *body = n->kids[3];
        if (init->type != N_EMPTY) eval(init, e);
        while (cond->type == N_EMPTY || eval(cond, e)) {
            if (exec(body, e)) return 1;
            if (step->type != N_EMPTY) eval(step, e);
        }
        return 0;
    }
    case N_RETURN:
        ret_val = eval(n->kids[0], e);
        return 1;
    case N_FUNC:
        return 0;   
    default:
        return 0;
    }
}

/* Точка входу  */
int interpret(Node *root) {
    global_env = env_new(NULL);

    /* 1-й прохід: реєструємо всі функції */
    for (int i = 0; i < root->nkids; i++)
        if (root->kids[i]->type == N_FUNC && nfuncs < 256)
            funcs[nfuncs++] = root->kids[i];

    /* 2-й прохід: виконуємо оператори верхнього рівня */
    for (int i = 0; i < root->nkids; i++)
        if (root->kids[i]->type != N_FUNC)
            if (exec(root->kids[i], global_env)) return 0;

    /* якщо є main, викликаємо її */
    for (int i = 0; i < nfuncs; i++) {
        if (strcmp(funcs[i]->str, "main") == 0) {
            Node call = {0};
            call.type = N_CALL;
            call.str = "main";
            call.line = funcs[i]->line;
            int r = call_function(&call, global_env);
            printf("main повернула: %d\n", r);
            return 0;
        }
    }
    return 0;
}
