#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define MAX_LEX 256

typedef enum {
    T_EOF, T_INT, T_FLOAT, T_BOOL, T_VOID, T_DOUBLE,
    T_ID, T_INT_LIT, T_FLOAT_LIT, T_TRUE, T_FALSE,
    T_SEMI, T_EQ, T_PLUS, T_MINUS, T_MUL, T_DIV,
    T_LBRACE, T_RBRACE,
    T_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;
    int line;
} Token;


static int line_no = 1;
static int pos = 0;
static char *src = NULL;

void next_char() { pos++; }
char cur_char() { return src[pos]; }
int is_eof() { return src[pos] == '\0'; }
void skip_ws() {
    while (!is_eof()) {
        char c = cur_char();
        if (c == '\n') { line_no++; pos++; continue; }
        if (isspace((unsigned char)c)) { pos++; continue; }
        if (c == '/' && src[pos+1] == '/') { // comment until EOL
            while (!is_eof() && cur_char() != '\n') pos++;
            continue;
        }
        break;
    }
}

Token make_token(TokenType type, const char *lex) {
    Token t;
    t.type = type;
    t.lexeme = strdup(lex ? lex : "");
    t.line = line_no;
    return t;
}

int starts_ident(char c) { return isalpha((unsigned char)c) || c == '_'; }
int is_ident_char(char c) { return isalnum((unsigned char)c) || c == '_'; }

Token next_token() {
    skip_ws();
    if (is_eof()) return make_token(T_EOF, "");
    char c = cur_char();

    // single char tokens
    if (c == ';') { next_char(); return make_token(T_SEMI, ";"); }
    if (c == '=') { next_char(); return make_token(T_EQ, "="); }
    if (c == '+') { next_char(); return make_token(T_PLUS, "+"); }
    if (c == '-') { next_char(); return make_token(T_MINUS, "-"); }
    if (c == '*') { next_char(); return make_token(T_MUL, "*"); }
    if (c == '/') { next_char(); return make_token(T_DIV, "/"); }
    if (c == '{') { next_char(); return make_token(T_LBRACE, "{"); }
    if (c == '}') { next_char(); return make_token(T_RBRACE, "}"); }

    if (starts_ident(c)) {
        char buf[MAX_LEX]; int i = 0;
        while (!is_eof() && is_ident_char(cur_char())) {
            if (i < MAX_LEX-1) buf[i++] = cur_char();
            next_char();
        }
        buf[i] = '\0';
        if (strcmp(buf, "int") == 0) return make_token(T_INT, buf);
        if (strcmp(buf, "float") == 0) return make_token(T_FLOAT, buf);
        if (strcmp(buf, "bool") == 0) return make_token(T_BOOL, buf);
        if (strcmp(buf, "true") == 0) return make_token(T_TRUE, buf);
        if (strcmp(buf, "false") == 0) return make_token(T_FALSE, buf);
        if (strcmp(buf, "void") == 0) return make_token(T_VOID, buf);
        if (strcmp(buf, "double") == 0) return make_token(T_DOUBLE, buf);
        return make_token(T_ID, buf);
    }
    if (isdigit((unsigned char)c)) {
        char buf[MAX_LEX]; int i = 0;
        while (!is_eof() && isdigit((unsigned char)cur_char())) {
            if (i < MAX_LEX-1) buf[i++] = cur_char();
            next_char();
        }
        int is_float = 0;
        if (!is_eof() && cur_char() == '.') {
            is_float = 1;
            buf[i++] = '.';
            next_char();
            while (!is_eof() && isdigit((unsigned char)cur_char())) {
                if (i < MAX_LEX-1) buf[i++] = cur_char();
                next_char();
            }
        }
        buf[i] = '\0';
        return is_float ? make_token(T_FLOAT_LIT, buf) : make_token(T_INT_LIT, buf);
    }

    char tmp[2] = {c, '\0'}; next_char();
    return make_token(T_UNKNOWN, tmp);
}

/* ---------- AST ---------- */

typedef enum { TYPE_INT, TYPE_FLOAT, TYPE_BOOL, TYPE_ERROR } VarType;

const char* type_name(VarType t){
    switch(t){ case TYPE_INT: return "int"; case TYPE_FLOAT: return "float"; case TYPE_BOOL: return "bool"; default: return "error"; }
}

typedef enum { EXPR_ID, EXPR_INT_LIT, EXPR_FLOAT_LIT, EXPR_BOOL_LIT, EXPR_BINOP } ExprKind;
typedef struct Expr {
    ExprKind kind;
    VarType inferred_type; // filled by semantic analyzer
    int line;
    union {
        char *id; // EXPR_ID
        int ival; // EXPR_INT_LIT
        double fval; // EXPR_FLOAT_LIT
        int bval; // EXPR_BOOL_LIT
        struct { char op; struct Expr *left, *right; } bin; // EXPR_BINOP
    } u;
} Expr;

/* Statements: decl or assignment or block */
typedef enum { STMT_DECL, STMT_ASSIGN, STMT_BLOCK } StmtKind;
typedef struct Stmt {
    StmtKind kind;
    int line;
    union {
        struct { VarType vtype; char *id; } decl;
        struct { char *id; Expr *expr; } assign;
        struct { struct Stmt **stmts; int n; } block;
    } u;
} Stmt;

typedef struct {
    Stmt **stmts;
    int n;
} Program;

/* ---------- Simple parser (recursive descent) ---------- */

Token cur_tok;
void advance() { free(cur_tok.lexeme); cur_tok = next_token(); }

int accept(TokenType t) { if (cur_tok.type == t) { advance(); return 1; } return 0; }
int expect(TokenType t, const char *errMsg) {
    if (cur_tok.type == t) { advance(); return 1; }
    fprintf(stderr, "Parse error (line %d): expected %s but found '%s'\n", cur_tok.line, errMsg, cur_tok.lexeme);
    exit(1);
}

/* forward */
Expr* parse_expr();

VarType parse_type_token(TokenType t) {
    if (t == T_INT) return TYPE_INT;
    if (t == T_FLOAT) return TYPE_FLOAT;
    if (t == T_BOOL) return TYPE_BOOL;
    return TYPE_ERROR;
}

Stmt* parse_decl() {
    VarType vt = parse_type_token(cur_tok.type);
    int ln = cur_tok.line;
    advance(); // consume type
    if (cur_tok.type != T_ID) { fprintf(stderr,"Parse error (line %d): expected identifier after type\n", ln); exit(1); }
    char *id = strdup(cur_tok.lexeme);
    advance();
    expect(T_SEMI, "semicolon");
    Stmt *s = malloc(sizeof(Stmt)); s->kind = STMT_DECL; s->line = ln;
    s->u.decl.vtype = vt; s->u.decl.id = id;
    return s;
}

Expr* make_int_expr(int v, int ln) {
    Expr *e = malloc(sizeof(Expr)); e->kind = EXPR_INT_LIT; e->line = ln; e->inferred_type = TYPE_INT; e->u.ival = v; return e;
}
Expr* make_float_expr(double v, int ln) {
    Expr *e = malloc(sizeof(Expr)); e->kind = EXPR_FLOAT_LIT; e->line = ln; e->inferred_type = TYPE_FLOAT; e->u.fval = v; return e;
}
Expr* make_bool_expr(int b, int ln) {
    Expr *e = malloc(sizeof(Expr)); e->kind = EXPR_BOOL_LIT; e->line = ln; e->inferred_type = TYPE_BOOL; e->u.bval = b; return e;
}
Expr* make_id_expr(const char *id, int ln) {
    Expr *e = malloc(sizeof(Expr)); e->kind = EXPR_ID; e->line = ln; e->inferred_type = TYPE_ERROR; e->u.id = strdup(id); return e;
}
Expr* make_binop(char op, Expr *l, Expr *r, int ln) {
    Expr *e = malloc(sizeof(Expr)); e->kind = EXPR_BINOP; e->line = ln; e->inferred_type = TYPE_ERROR; e->u.bin.op = op; e->u.bin.left = l; e->u.bin.right = r; return e;
}

/* Parsing expressions with precedence: +-, then */
Expr* parse_primary() {
    if (cur_tok.type == T_INT_LIT) {
        int v = atoi(cur_tok.lexeme);
        int ln = cur_tok.line;
        advance(); return make_int_expr(v, ln);
    }
    if (cur_tok.type == T_FLOAT_LIT) {
        double v = atof(cur_tok.lexeme);
        int ln = cur_tok.line;
        advance(); return make_float_expr(v, ln);
    }
    if (cur_tok.type == T_TRUE) { int ln = cur_tok.line; advance(); return make_bool_expr(1, ln); }
    if (cur_tok.type == T_FALSE) { int ln = cur_tok.line; advance(); return make_bool_expr(0, ln); }
    if (cur_tok.type == T_ID) {
        int ln = cur_tok.line;
        Expr *e = make_id_expr(cur_tok.lexeme, ln);
        advance();
        return e;
    }
    if (accept(T_LBRACE)) { // allow brace expr? not in grammar, put back
        fprintf(stderr,"Parse error (line %d): unexpected '{' in expression\n", cur_tok.line); exit(1);
    }
    fprintf(stderr,"Parse error (line %d): unexpected token '%s' in expression\n", cur_tok.line, cur_tok.lexeme);
    exit(1);
}

Expr* parse_muldiv() {
    Expr *left = parse_primary();
    while (cur_tok.type == T_MUL || cur_tok.type == T_DIV) {
        char op = cur_tok.lexeme[0];
        int ln = cur_tok.line;
        advance();
        Expr *right = parse_primary();
        left = make_binop(op, left, right, ln);
    }
    return left;
}

Expr* parse_expr() {
    Expr *left = parse_muldiv();
    while (cur_tok.type == T_PLUS || cur_tok.type == T_MINUS) {
        char op = cur_tok.lexeme[0];
        int ln = cur_tok.line;
        advance();
        Expr *right = parse_muldiv();
        left = make_binop(op, left, right, ln);
    }
    return left;
}

Stmt* parse_assign() {
    int ln = cur_tok.line;
    char *id = strdup(cur_tok.lexeme);
    advance();
    expect(T_EQ, "equals");
    Expr *e = parse_expr();
    expect(T_SEMI, "semicolon");
    Stmt *s = malloc(sizeof(Stmt)); s->kind = STMT_ASSIGN; s->line = ln;
    s->u.assign.id = id; s->u.assign.expr = e;
    return s;
}

Stmt* parse_block(); // forward

Stmt* parse_stmt() {
    if (cur_tok.type == T_INT || cur_tok.type == T_FLOAT || cur_tok.type == T_BOOL) return parse_decl();
    if (cur_tok.type == T_ID) return parse_assign();
    if (cur_tok.type == T_LBRACE) return parse_block();
    fprintf(stderr,"Parse error (line %d): unexpected token '%s' at statement start\n", cur_tok.line, cur_tok.lexeme);
    exit(1);
}

Stmt* parse_block() {
    int ln = cur_tok.line;
    expect(T_LBRACE, "{");
    Stmt **arr = NULL; int n = 0;
    while (cur_tok.type != T_RBRACE && cur_tok.type != T_EOF) {
        Stmt *s = parse_stmt();
        arr = realloc(arr, sizeof(Stmt*)*(n+1)); arr[n++] = s;
    }
    expect(T_RBRACE, "}");
    Stmt *b = malloc(sizeof(Stmt)); b->kind = STMT_BLOCK; b->line = ln;
    b->u.block.stmts = arr; b->u.block.n = n;
    return b;
}

Program* parse_program() {
    Program *p = malloc(sizeof(Program)); p->stmts = NULL; p->n = 0;
    while (cur_tok.type != T_EOF) {
        Stmt *s = parse_stmt();
        p->stmts = realloc(p->stmts, sizeof(Stmt*)*(p->n+1));
        p->stmts[p->n++] = s;
    }
    return p;
}

/* ---------- Symbol table & semantic analysis ---------- */

typedef struct Sym {
    char *name;
    VarType type;
    int scope_level;
} Sym;

typedef struct {
    Sym *arr;
    int n;
    int capacity;
} SymTable;

void symtable_init(SymTable *t) { t->arr = NULL; t->n = 0; t->capacity = 0; }
void symtable_free(SymTable *t) {
    for (int i=0;i<t->n;i++) free(t->arr[i].name);
    free(t->arr);
}

void symtable_add(SymTable *t, const char *name, VarType ty, int scope) {
    if (t->n == t->capacity) { t->capacity = t->capacity? t->capacity*2: 8; t->arr = realloc(t->arr, sizeof(Sym)*t->capacity); }
    t->arr[t->n].name = strdup(name);
    t->arr[t->n].type = ty;
    t->arr[t->n].scope_level = scope;
    t->n++;
}

Sym* symtable_lookup_in_scope(SymTable *t, const char *name, int scope) {
    for (int i = t->n-1; i >= 0; --i) {
        if (t->arr[i].scope_level != scope) continue;
        if (strcmp(t->arr[i].name, name) == 0) return &t->arr[i];
    }
    return NULL;
}
Sym* symtable_lookup(SymTable *t, const char *name) {
    for (int i = t->n-1; i >= 0; --i) if (strcmp(t->arr[i].name, name) == 0) return &t->arr[i];
    return NULL;
}

/* Semantic analyzer state */
typedef struct {
    SymTable table;
    int scope_level;
    int errors;
} SemCtx;

void sem_enter_scope(SemCtx *c) { c->scope_level++; }
void sem_leave_scope(SemCtx *c) {
    // remove symbols that belong to this scope
    for (int i = c->table.n - 1; i >= 0; --i) {
        if (c->table.arr[i].scope_level == c->scope_level) {
            free(c->table.arr[i].name);
            // compact array
            c->table.arr[i] = c->table.arr[c->table.n-1];
            c->table.n--;
        }
    }
    c->scope_level--;
}

void sem_error(SemCtx *c, int line, const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "Semantic error (line %d): ", line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    c->errors++;
}

/* Type checking helpers */
int is_numeric(VarType t) { return t == TYPE_INT || t == TYPE_FLOAT; }

VarType unify_numeric(VarType a, VarType b) {
    if (!is_numeric(a) || !is_numeric(b)) return TYPE_ERROR;
    if (a == TYPE_FLOAT || b == TYPE_FLOAT) return TYPE_FLOAT;
    return TYPE_INT;
}

/* Walk expression and infer/check types */
VarType sem_check_expr(SemCtx *c, Expr *e) {
    if (!e) return TYPE_ERROR;
    switch (e->kind) {
        case EXPR_INT_LIT: e->inferred_type = TYPE_INT; return TYPE_INT;
        case EXPR_FLOAT_LIT: e->inferred_type = TYPE_FLOAT; return TYPE_FLOAT;
        case EXPR_BOOL_LIT: e->inferred_type = TYPE_BOOL; return TYPE_BOOL;
        case EXPR_ID: {
            Sym *s = symtable_lookup(&c->table, e->u.id);
            if (!s) {
                sem_error(c, e->line, "use of undeclared variable '%s'", e->u.id);
                e->inferred_type = TYPE_ERROR;
                return TYPE_ERROR;
            }
            e->inferred_type = s->type;
            return s->type;
        }
        case EXPR_BINOP: {
            VarType lt = sem_check_expr(c, e->u.bin.left);
            VarType rt = sem_check_expr(c, e->u.bin.right);
            char op = e->u.bin.op;
            if (op == '+' || op == '-' || op == '*' || op == '/') {
                if (!is_numeric(lt) || !is_numeric(rt)) {
                    sem_error(c, e->line, "operator '%c' requires numeric operands (found %s and %s)", op, type_name(lt), type_name(rt));
                    e->inferred_type = TYPE_ERROR; return TYPE_ERROR;
                }
                VarType uni = unify_numeric(lt, rt);
                e->inferred_type = uni; return uni;
            }
            sem_error(c, e->line, "unknown binary operator '%c'", op);
            e->inferred_type = TYPE_ERROR; return TYPE_ERROR;
        }
    }
    return TYPE_ERROR;
}

/* Check a statement */
void sem_check_stmt(SemCtx *c, Stmt *s) {
    if (!s) return;
    switch (s->kind) {
        case STMT_DECL: {
            // check redeclaration in same scope
            if (symtable_lookup_in_scope(&c->table, s->u.decl.id, c->scope_level)) {
                sem_error(c, s->line, "redeclaration of '%s' in the same scope", s->u.decl.id);
            } else {
                symtable_add(&c->table, s->u.decl.id, s->u.decl.vtype, c->scope_level);
            }
            return;
        }
        case STMT_ASSIGN: {
            Sym *dst = symtable_lookup(&c->table, s->u.assign.id);
            if (!dst) {
                sem_error(c, s->line, "assignment to undeclared variable '%s'", s->u.assign.id);
                // still check expr to report other errors
                sem_check_expr(c, s->u.assign.expr);
                return;
            }
            VarType rt = sem_check_expr(c, s->u.assign.expr);
            // allow implicit int -> float, but not float -> int
            if (rt == TYPE_ERROR) return;
            if (dst->type == rt) return;
            if (dst->type == TYPE_FLOAT && rt == TYPE_INT) {
                // implicit promotion OK
                return;
            }
            // disallow other conversions
            sem_error(c, s->line, "cannot assign %s to %s variable '%s'", type_name(rt), type_name(dst->type), s->u.assign.id);
            return;
        }
        case STMT_BLOCK: {
            sem_enter_scope(c);
            for (int i=0;i<s->u.block.n;i++) sem_check_stmt(c, s->u.block.stmts[i]);
            sem_leave_scope(c);
            return;
        }
    }
}

/* Program semantic check */
int sem_check_program(Program *p) {
    SemCtx ctx; symtable_init(&ctx.table); ctx.scope_level = 0; ctx.errors = 0;
    // global scope level 0
    for (int i=0;i<p->n;i++) sem_check_stmt(&ctx, p->stmts[i]);
    int errs = ctx.errors;
    symtable_free(&ctx.table);
    return errs;
}

/* ---------- Utilities to free AST ---------- */

void free_expr(Expr *e) {
    if (!e) return;
    if (e->kind == EXPR_ID) free(e->u.id);
    if (e->kind == EXPR_BINOP) { free_expr(e->u.bin.left); free_expr(e->u.bin.right); }
    free(e);
}

void free_stmt(Stmt *s) {
    if (!s) return;
    if (s->kind == STMT_DECL) free(s->u.decl.id);
    if (s->kind == STMT_ASSIGN) { free(s->u.assign.id); free_expr(s->u.assign.expr); }
    if (s->kind == STMT_BLOCK) {
        for (int i=0;i<s->u.block.n;i++) free_stmt(s->u.block.stmts[i]);
        free(s->u.block.stmts);
    }
    free(s);
}

void free_program(Program *p) {
    for (int i=0;i<p->n;i++) free_stmt(p->stmts[i]);
    free(p->stmts);
    free(p);
}

/* ---------- Main + example ---------- */

/* semantico.c - versión modificada para leer desde archivo .txt
   Compilar:
     gcc -std=c99 semantico.c -o semantico
   Ejecutar:
     ./semantico input.txt
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>  // <--- ¡faltaba para va_list!

/* ---------- (resto de tu código igual que antes hasta el main) ---------- */
/* ... pega todo tu código anterior aquí sin cambios ... */


/* ---------- Main modificado ---------- */

int main(int argc, char **argv) {
    FILE *f = NULL;

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo_entrada.txt>\n", argv[0]);
        return 1;
    }

    // Abrir archivo
    f = fopen(argv[1], "r");
    if (!f) {
        perror("No se pudo abrir el archivo");
        return 1;
    }

    // Obtener tamaño del archivo
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Leer todo el contenido del archivo en memoria
    src = malloc(fsize + 2);
    if (!src) { fprintf(stderr, "Error: sin memoria.\n"); fclose(f); return 1; }

    size_t r = fread(src, 1, fsize, f);
    src[r] = '\0';
    fclose(f);

    // Inicializar análisis
    pos = 0;
    line_no = 1;
    cur_tok = next_token();

    Program *prog = parse_program();
    int errors = sem_check_program(prog);

    if (errors == 0)
        printf("Análisis semántico completado: sin errores.\n");
    else
        printf("Análisis semántico completado: %d error(es) encontrados.\n", errors);

    // Liberar memoria
    free_program(prog);
    free(cur_tok.lexeme);
    free(src);
    return errors ? 1 : 0;
}