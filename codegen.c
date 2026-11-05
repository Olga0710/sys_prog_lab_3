#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "codegen.h"

static FILE *out;

/* Перейменування змінних, щоб імітувати блочні області видимості */
typedef struct Sym {
    char *name;
    char *pyname;
    struct Sym *next;
} Sym;

typedef struct Scope {
    Sym *syms;
    struct Scope *parent;
} Scope;

static int uniq = 0;

static Scope *scope_new(Scope *parent) {
    Scope *s = calloc(1, sizeof(Scope));
    s->parent = parent;
    return s;
}

static const char *scope_declare(Scope *s, const char *name) {
    Sym *sym = malloc(sizeof(Sym));
    sym->name = strdup(name);
    int taken = 0;
    for (Scope *c = s; c && !taken; c = c->parent)
        for (Sym *x = c->syms; x; x = x->next)
            if (strcmp(x->name, name) == 0) { taken = 1; break; }
    if (taken) {
        char buf[128];
        snprintf(buf, sizeof buf, "%s_%d", name, ++uniq);
        sym->pyname = strdup(buf);
    } else {
        sym->pyname = strdup(name);
    }
    sym->next = s->syms;
    s->syms = sym;
    return sym->pyname;
}

static const char *scope_lookup(Scope *s, const char *name) {
    for (; s; s = s->parent)
        for (Sym *x = s->syms; x; x = x->next)
            if (strcmp(x->name, name) == 0) return x->pyname;
    return name;
}

static void indent(int d) {
    for (int i = 0; i < d; i++) fputs("    ", out);
}

static void gen_expr(Node *n, Scope *sc);
static void gen_stmt(Node *n, int d, Scope *sc);

static void gen_expr(Node *n, Scope *sc) {
    switch (n->type) {
    case N_NUM:
        fprintf(out, "%d", n->num);
        break;
    case N_VAR:
        fputs(scope_lookup(sc, n->str), out);
        break;
    case N_ASSIGN:
        fprintf(out, "(%s := ", scope_lookup(sc, n->str));
        gen_expr(n->kids[0], sc);
        fputc(')', out);
        break;
    case N_UNOP:
        fputs("(-", out);
        gen_expr(n->kids[0], sc);
        fputc(')', out);
        break;
    case N_BINOP: {
        const char *op = n->str;
        int is_cmp = !strcmp(op, "==") || !strcmp(op, "!=") ||
                     !strcmp(op, "<")  || !strcmp(op, ">")  ||
                     !strcmp(op, "<=") || !strcmp(op, ">=");
        if (is_cmp) fputs("int(", out);
        fputc('(', out);
        gen_expr(n->kids[0], sc);
        fprintf(out, " %s ", strcmp(op, "/") == 0 ? "//" : op);
        gen_expr(n->kids[1], sc);
        fputc(')', out);
        if (is_cmp) fputc(')', out);
        break;
    }
    case N_CALL:
        fprintf(out, "%s(", n->str);
        for (int i = 0; i < n->nkids; i++) {
            if (i) fputs(", ", out);
            gen_expr(n->kids[i], sc);
        }
        fputc(')', out);
        break;
    default:
        fprintf(out, "None");
    }
}

static void gen_stmt(Node *n, int d, Scope *sc) {
    switch (n->type) {
    case N_DECL: {
        const char *py = scope_declare(sc, n->str);
        indent(d);
        fprintf(out, "%s = ", py);
        if (n->nkids) gen_expr(n->kids[0], sc);
        else fputs("0", out);
        fputc('\n', out);
        break;
    }
    case N_EXPR_STMT:
        indent(d);
        gen_expr(n->kids[0], sc);
        fputc('\n', out);
        break;
    case N_BLOCK: {
        Scope *inner = scope_new(sc);
        if (n->nkids == 0) { indent(d); fputs("pass\n", out); }
        for (int i = 0; i < n->nkids; i++)
            gen_stmt(n->kids[i], d, inner);
        break;
    }
    case N_IF:
        indent(d); fputs("if ", out);
        gen_expr(n->kids[0], sc);
        fputs(":\n", out);
        gen_stmt(n->kids[1], d + 1, sc);
        if (n->nkids == 3) {
            indent(d); fputs("else:\n", out);
            gen_stmt(n->kids[2], d + 1, sc);
        }
        break;
    case N_WHILE:
        indent(d); fputs("while ", out);
        gen_expr(n->kids[0], sc);
        fputs(":\n", out);
        gen_stmt(n->kids[1], d + 1, sc);
        break;
    case N_FOR: {
        Node *init = n->kids[0], *cond = n->kids[1];
        Node *step = n->kids[2], *body = n->kids[3];
        if (init->type != N_EMPTY) {
            indent(d); gen_expr(init, sc); fputc('\n', out);
        }
        indent(d); fputs("while ", out);
        if (cond->type == N_EMPTY) fputs("True", out);
        else gen_expr(cond, sc);
        fputs(":\n", out);
        gen_stmt(body, d + 1, sc);
        if (step->type != N_EMPTY) {
            indent(d + 1); gen_expr(step, sc); fputc('\n', out);
        }
        break;
    }
    case N_RETURN:
        indent(d); fputs("return ", out);
        gen_expr(n->kids[0], sc);
        fputc('\n', out);
        break;
    default:
        break;
    }
}

static void gen_func(Node *f) {
    Scope *sc = scope_new(NULL);
    Node *params = f->kids[0], *body = f->kids[1];
    fprintf(out, "def %s(", f->str);
    for (int i = 0; i < params->nkids; i++) {
        if (i) fputs(", ", out);
        fputs(scope_declare(sc, params->kids[i]->str), out);
    }
    fputs("):\n", out);
    if (body->nkids == 0) fputs("    pass\n", out);
    gen_stmt(body, 1, sc);
    fputc('\n', out);
}

void generate_python(Node *root, FILE *o) {
    out = o;
    int has_main = 0;
    fputs("# Згенеровано з міні-C\n\n", out);

    for (int i = 0; i < root->nkids; i++)
        if (root->kids[i]->type == N_FUNC) {
            gen_func(root->kids[i]);
            if (strcmp(root->kids[i]->str, "main") == 0) has_main = 1;
        }

    Scope *global = scope_new(NULL);
    for (int i = 0; i < root->nkids; i++)
        if (root->kids[i]->type != N_FUNC)
            gen_stmt(root->kids[i], 0, global);

    if (has_main) {
        fputs("if __name__ == \"__main__\":\n", out);
        fputs("    print(\"main повернула:\", main())\n", out);
    }
}
