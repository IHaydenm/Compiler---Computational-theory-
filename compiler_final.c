#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

typedef enum {
    L_T_ID, L_T_KEYWORD, L_T_INT_LITERAL, L_T_FLOAT_LITERAL, L_T_STRING_LITERAL, L_T_STRUCT_LITERAL,
    L_T_PLUS, L_T_MINUS, L_T_STAR, L_T_SLASH, L_T_PERCENT,
    L_T_ASSIGN, L_T_EQ, L_T_NEQ, L_T_LT, L_T_LE, L_T_GT, L_T_GE,
    L_T_AND, L_T_OR, L_T_NOT,
    L_T_SEMICOLON, L_T_COMMA, L_T_LPAREN, L_T_RPAREN, L_T_LBRACE, L_T_RBRACE, L_T_LBRACKET, L_T_RBRACKET,
    L_T_EOF, L_T_UNKNOWN
} LTokenType;

const char* ltoken_type_name(LTokenType type) {
    switch (type) {
        case L_T_ID: return "IDENTIFIER";
        case L_T_KEYWORD: return "KEYWORD";
        case L_T_INT_LITERAL: return "INT_LITERAL";
        case L_T_FLOAT_LITERAL: return "FLOAT_LITERAL";
        case L_T_STRING_LITERAL: return "STRING_LITERAL";
        case L_T_STRUCT_LITERAL: return "STRUCT LITERAL";
        case L_T_PLUS: return "PLUS (+)";
        case L_T_MINUS: return "MINUS (-)";
        case L_T_STAR: return "MULTIPLY (*)";
        case L_T_SLASH: return "DIVIDE (/)";
        case L_T_PERCENT: return "MODULO (%)";
        case L_T_ASSIGN: return "ASSIGN (=)";
        case L_T_EQ: return "EQUALS (==)";
        case L_T_NEQ: return "NOT_EQUALS (!=)";
        case L_T_LT: return "LESS (<)";
        case L_T_LE: return "LESS_EQUAL (<=)";
        case L_T_GT: return "GREATER (>)";
        case L_T_GE: return "GREATER_EQUAL (>=)";
        case L_T_AND: return "AND (&&)";
        case L_T_OR: return "OR (||)";
        case L_T_NOT: return "NOT (!)";
        case L_T_SEMICOLON: return "SEMICOLON (;)";
        case L_T_COMMA: return "COMMA (,)";
        case L_T_LPAREN: return "LEFT_PAREN (()";
        case L_T_RPAREN: return "RIGHT_PAREN ())";
        case L_T_LBRACE: return "LEFT_BRACE ({)";
        case L_T_RBRACE: return "RIGHT_BRACE (})";
        case L_T_LBRACKET: return "LEFT_BRACKET ([)";
        case L_T_RBRACKET: return "RIGHT_BRACKET (])";
        case L_T_EOF: return "EOF";
        case L_T_UNKNOWN: return "UNKNOWN";
        default: return "UNDEFINED";
    }
}

const char* keywords[] = {
    "if","else","while","for","return","int","float","char","void","double", "bool", "true", "false", "struct", "end", "printf", NULL
};

int isKeyword(const char* token) {
    for (int i = 0; keywords[i]!=NULL; ++i)
        if (strcmp(token, keywords[i]) == 0)
            return 1;
    return 0;
}

typedef struct {
    char *buf;
    size_t len;
    size_t pos;
    int line;
    int col;
} Scanner;
static Scanner scanner = {NULL,0,0,1,1};

void load_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) { perror("fopen"); exit(1); }
    fseek(f, 0, SEEK_END);
    long l = ftell(f);
    fseek(f, 0, SEEK_SET);
    scanner.buf = malloc(l+2);
    if (!scanner.buf) { perror("malloc"); exit(1); }
    size_t r = fread(scanner.buf, 1, l, f);
    scanner.buf[r] = '\0';
    scanner.len = r;
    scanner.pos = 0;
    scanner.line = 1;
    scanner.col = 1;
    fclose(f);
}

int peek_lex() {
    if (scanner.pos >= scanner.len) return EOF;
    return (unsigned char)scanner.buf[scanner.pos];
}
int getch_lex() {
    int c = peek_lex();
    if (c == EOF) return EOF;
    scanner.pos++;
    if (c == '\n') { scanner.line++; scanner.col = 1; }
    else scanner.col++;
    return c;
}
int match_lex(char expected) {
    if (peek_lex() == expected){
        getch_lex();
        return 1;
    }
    return 0;
}

typedef struct {
    LTokenType type;
    char *lexeme;
    int line;
    int col;
} LToken;

LToken make_ltoken(LTokenType type, const char *lexeme_start, size_t len, int line, int col) {
    LToken t;
    t.type = type;
    t.lexeme = malloc(len+1);
    if (!t.lexeme) { perror("malloc token"); exit(1); }
    memcpy(t.lexeme, lexeme_start, len);
    t.lexeme[len] = '\0';
    t.line = line;
    t.col = col;
    return t;
}

void free_ltoken(LToken *t) {
    if (t->lexeme) free(t->lexeme);
    t->lexeme = NULL;
}

void skip_whitespace_and_comments() {
    int c;
    while ((c = peek_lex()) != EOF) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            getch_lex();
            continue;
        }
        if (c == '/') {
            if (scanner.pos+1 <= scanner.len && scanner.buf[scanner.pos+1] == '/') {
                getch_lex(); getch_lex();
                while (peek_lex() != EOF && peek_lex() != '\n') getch_lex();
                continue;
            } else if (scanner.pos+1 <= scanner.len && scanner.buf[scanner.pos+1] == '*') {
                getch_lex(); getch_lex();
                int closed = 0;
                while (peek_lex() != EOF) {
                    if (peek_lex() == '*' && scanner.pos+1 <= scanner.len && scanner.buf[scanner.pos+1] == '/') {
                        getch_lex(); getch_lex();
                        closed = 1; break;
                    } else getch_lex();
                }
                if (!closed) {
                    fprintf(stderr, "Error: comentario de bloque no cerrado (linea %d col %d)\n", scanner.line, scanner.col);
                    exit(1);
                }
                continue;
            }
            break;
        }
        break;
    }
}

LToken lex_next_token() {
    skip_whitespace_and_comments();
    int c = peek_lex();
    if (c == EOF) return make_ltoken(L_T_EOF, "", 0, scanner.line, scanner.col);

    size_t start_pos = scanner.pos;
    int start_line = scanner.line, start_col = scanner.col;

    if (isalpha(c) || c == '_') {
        getch_lex();
        while (isalnum(peek_lex()) || peek_lex() == '_') getch_lex();
        size_t len = scanner.pos - start_pos;
        char temp[len+1];
        memcpy(temp, scanner.buf + start_pos, len);
        temp[len] = '\0';
        if (isKeyword(temp))
            return make_ltoken(L_T_KEYWORD, scanner.buf + start_pos, len, start_line, start_col);
        else
            return make_ltoken(L_T_ID, scanner.buf + start_pos, len, start_line, start_col);
    }

    if (isdigit(c)) {
        int seen_dot = 0;
        getch_lex();
        while (isdigit(peek_lex()) || (peek_lex() == '.' && !seen_dot)) {
            if (peek_lex() == '.') {
                seen_dot = 1;
                getch_lex();
                if (!isdigit(peek_lex())) {
                    fprintf(stderr, "Error: número mal formado en linea %d col %d\n", scanner.line, scanner.col);
                    exit(1);
                }
            } else getch_lex();
        }
        size_t len = scanner.pos - start_pos;
        return make_ltoken(seen_dot ? L_T_FLOAT_LITERAL : L_T_INT_LITERAL, scanner.buf + start_pos, len, start_line, start_col);
    }

    if (c == '"') {
        getch_lex();
        while (peek_lex() != EOF && peek_lex() != '"') {
            if (peek_lex() == '\\') { getch_lex(); if (peek_lex() != EOF) getch_lex(); }
            else {
                if (peek_lex() == '\n') {
                    fprintf(stderr, "Error: cadena no terminada en linea %d col %d\n", start_line, start_col);
                    exit(1);
                }
                getch_lex();
            }
        }
        if (peek_lex() == '"') {
            getch_lex();
            size_t len = scanner.pos - start_pos;
            return make_ltoken(L_T_STRING_LITERAL, scanner.buf + start_pos, len, start_line, start_col);
        } else {
            fprintf(stderr, "Error: cadena no terminada antes del EOF (linea %d col %d)\n", start_line, start_col);
            exit(1);
        }
    }

    /* Two-char operators */
    if (c == '=') {
        getch_lex();
        if (match_lex('=')) return make_ltoken(L_T_EQ, "==", 2, start_line, start_col);
        else return make_ltoken(L_T_ASSIGN, "=", 1, start_line, start_col);
    }
    if (c == '!') {
        getch_lex();
        if (match_lex('=')) return make_ltoken(L_T_NEQ, "!=", 2, start_line, start_col);
        else return make_ltoken(L_T_NOT, "!", 1, start_line, start_col);
    }
    if (c == '<') {
        getch_lex();
        if (match_lex('=')) return make_ltoken(L_T_LE, "<=", 2, start_line, start_col);
        else return make_ltoken(L_T_LT, "<", 1, start_line, start_col);
    }
    if (c == '>') {
        getch_lex();
        if (match_lex('=')) return make_ltoken(L_T_GE, ">=", 2, start_line, start_col);
        else return make_ltoken(L_T_GT, ">", 1, start_line, start_col);
    }
    if (c == '&') {
        getch_lex();
        if (match_lex('&')) return make_ltoken(L_T_AND, "&&", 2, start_line, start_col);
        else { fprintf(stderr, "Error: '&' aislado en linea %d col %d\n", start_line, start_col); exit(1); }
    }
    if (c == '|') {
        getch_lex();
        if (match_lex('|')) return make_ltoken(L_T_OR, "||", 2, start_line, start_col);
        else { fprintf(stderr, "Error: '|' aislado en linea %d col %d\n", start_line, start_col); exit(1); }
    }

    getch_lex();
    switch (c) {
        case '+': return make_ltoken(L_T_PLUS, "+", 1, start_line, start_col);
        case '-': return make_ltoken(L_T_MINUS, "-", 1, start_line, start_col);
        case '*': return make_ltoken(L_T_STAR, "*", 1, start_line, start_col);
        case '/': return make_ltoken(L_T_SLASH, "/", 1, start_line, start_col);
        case '%': return make_ltoken(L_T_PERCENT, "%", 1, start_line, start_col);
        case ';': return make_ltoken(L_T_SEMICOLON, ";", 1, start_line, start_col);
        case ',': return make_ltoken(L_T_COMMA, ",", 1, start_line, start_col);
        case '(': return make_ltoken(L_T_LPAREN, "(", 1, start_line, start_col);
        case ')': return make_ltoken(L_T_RPAREN, ")", 1, start_line, start_col);
        case '{': return make_ltoken(L_T_LBRACE, "{", 1, start_line, start_col);
        case '}': return make_ltoken(L_T_RBRACE, "}", 1, start_line, start_col);
        case '[': return make_ltoken(L_T_LBRACKET, "[", 1, start_line, start_col);
        case ']': return make_ltoken(L_T_RBRACKET, "]", 1, start_line, start_col);
        default: {
            char tmp[2] = {(char)c, '\0'};
            fprintf(stderr, "Error lexico: caracter no reconocido '%s' en linea %d col %d\n", tmp, start_line, start_col);
            exit(1);
            return make_ltoken(L_T_UNKNOWN, tmp, 1, start_line, start_col);
        }
    }
}

#define MAX_LEX 256

typedef enum {
    T_EOF, T_INT, T_FLOAT, T_BOOL, T_VOID, T_DOUBLE,
    T_ID, T_INT_LIT, T_FLOAT_LIT, T_TRUE, T_FALSE,
    T_SEMI, T_EQ, T_PLUS, T_MINUS, T_MUL, T_DIV,
    T_LBRACE, T_RBRACE, T_STRUCT, T_UNKNOWN
} TokenType;

/* Token used by parser / semantic code */
typedef struct {
    TokenType type;
    char *lexeme;
    int line;
} Token;

Token make_token(TokenType type, const char *lexeme, int line)
{
    Token t;
    t.type = type;
    t.lexeme = strdup(lexeme ? lexeme : "");
    t.line = line;
    return t;
}


Token make_parser_token(TokenType type, const char *lex) {
    Token t;
    t.type = type;
    t.lexeme = strdup(lex ? lex : "");
    t.line = 1; /* will overwrite with real line when mapping */
    return t;
}

/* Parser's current token */
static Token cur_tok;

/* Adapter: convert an LToken (from lexer) -> parser Token */
Token ltoken_to_parser_token(const LToken *lt) {
    Token t;
    t.lexeme = strdup(lt->lexeme);
    t.line = lt->line;

    /* Default */
    t.type = T_UNKNOWN;

    switch (lt->type) {
        case L_T_EOF: t.type = T_EOF; break;
        case L_T_INT_LITERAL: t.type = T_INT_LIT; break;
        case L_T_FLOAT_LITERAL: t.type = T_FLOAT_LIT; break;
        case L_T_ID: t.type = T_ID; break;
        case L_T_STRING_LITERAL: t.type = T_UNKNOWN; break;

        /* Operators */
        case L_T_PLUS: t.type = T_PLUS; break;
        case L_T_MINUS: t.type = T_MINUS; break;
        case L_T_STAR: t.type = T_MUL; break;
        case L_T_SLASH: t.type = T_DIV; break;
        case L_T_SEMICOLON: t.type = T_SEMI; break;
        case L_T_LBRACE: t.type = T_LBRACE; break;
        case L_T_RBRACE: t.type = T_RBRACE; break;
        case L_T_ASSIGN: /* single '=' -> used as assignment in parser */ t.type = T_EQ; break;
        case L_T_EQ: /* '==' - not used by parser for assignment; map to T_UNKNOWN */ t.type = T_UNKNOWN; break;

        case L_T_KEYWORD: {
            /* For keywords, inspect lexeme to map to parser token types */
            if (strcmp(lt->lexeme, "int") == 0) t.type = T_INT;
            else if (strcmp(lt->lexeme, "float") == 0) t.type = T_FLOAT;
            else if (strcmp(lt->lexeme, "bool") == 0) t.type = T_BOOL;
            else if (strcmp(lt->lexeme, "void") == 0) t.type = T_VOID;
            else if (strcmp(lt->lexeme, "double") == 0) t.type = T_DOUBLE;
            else if (strcmp(lt->lexeme, "true") == 0) t.type = T_TRUE;
            else if (strcmp(lt->lexeme, "false") == 0) t.type = T_FALSE;
            else if (strcmp(lt->lexeme, "struct") == 0) t.type = T_STRUCT;
            else t.type = T_UNKNOWN;
            break;
        }

        default:
            t.type = T_UNKNOWN;
            break;
    }
    return t;
}

/* We'll implement a wrapper function that uses lexer to fetch the next LToken,
   converts it to parser Token, and returns it. We'll keep a small internal buffer
   because parser expects to free cur_tok.lexeme when advancing. */

Token get_next_parser_token() {
    LToken lt = lex_next_token();
    Token t = ltoken_to_parser_token(&lt);
    free_ltoken(&lt); /* we strdup'ed lexeme into parser token, so free lexer's copy */
    return t;
}

/* Helper to free parser token (lexeme) */
void free_parser_token(Token *t) {
    if (t->lexeme) free(t->lexeme);
    t->lexeme = NULL;
}

/* ---------- AST and Parser (largely unchanged) ---------- */

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

/* Parser state helpers */
void advance(); /* forward */
Token next_token_from_lexer(); /* forward */

/* convenience functions */
int is_eof_parser_token(TokenType t) { return t == T_EOF; }

/* parser helper functions */
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

Stmt* parse_struct_decl() {
    int ln = cur_tok.line;
    expect(T_STRUCT, "struct");
    if (cur_tok.type != T_ID) {
        fprintf(stderr, "Parse error (line %d): expected struct name\n", ln);
        exit(1);
    }
    char *name = strdup(cur_tok.lexeme);
    advance();
    expect(T_LBRACE, "{");

    // parse fields (reusing parse_decl() logic but inside struct)
    while (cur_tok.type == T_INT || cur_tok.type == T_FLOAT || cur_tok.type == T_BOOL) {
        parse_decl(); // or store fields in a list
    }

    expect(T_RBRACE, "}");
    expect(T_SEMI, "semicolon");

    printf("Parsed struct declaration: %s\n", name);
    free(name);
    return NULL; // for now, not added to AST
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
        char op = (cur_tok.type == T_MUL) ? '*' : '/';
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
        char op = (cur_tok.type == T_PLUS) ? '+' : '-';
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

Stmt* parse_block(); /* forward */

Stmt* parse_stmt() {
    if (cur_tok.type == T_INT || cur_tok.type == T_FLOAT || cur_tok.type == T_BOOL) return parse_decl();
    if (cur_tok.type == T_ID) return parse_assign();
    if (cur_tok.type == T_LBRACE) return parse_block();
    if(cur_tok.type == T_STRUCT) return parse_struct_decl();
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
    for (int i = c->table.n - 1; i >= 0; --i) {
        if (c->table.arr[i].scope_level == c->scope_level) {
            free(c->table.arr[i].name);
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
                sem_check_expr(c, s->u.assign.expr);
                return;
            }
            VarType rt = sem_check_expr(c, s->u.assign.expr);
            if (rt == TYPE_ERROR) return;
            if (dst->type == rt) return;
            if (dst->type == TYPE_FLOAT && rt == TYPE_INT) {
                return;
            }
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

/* ---------- Token stream glue (advance / next token) ---------- */

/* advance() frees current token lexeme and fetches next token from lexer-adapter */
void advance() {
    free_parser_token(&cur_tok);
    cur_tok = get_next_parser_token();
}

void run_lexer_output(const char *filename) {
    printf("=== LEXICAL ANALYZER OUTPUT ===\n");
    load_file(filename);

    LToken t;
    do {
        t = lex_next_token();
        printf("Line %-3d  Type %-15s  Lexeme: '%s'\n", t.line, ltoken_type_name(t.type), t.lexeme);
        free_ltoken(&t);
    } while (t.type != L_T_EOF);

    printf("=== END OF LEXICAL ANALYZER OUTPUT ===\n\n");
}


int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo_entrada.txt>\n", argv[0]);
        return 1;
    }

    run_lexer_output(argv[1]);
    load_file(argv[1]); 
    cur_tok = get_next_parser_token();

    Program *prog = parse_program();

    int errors = sem_check_program(prog);

    if (errors == 0)
        printf("Analisis semantico completado: sin errores.\n");
    else
        printf("Analisis semántico completado: %d error(es) encontrados.\n", errors);

    /* free */
    free_program(prog);
    free_parser_token(&cur_tok);
    free(scanner.buf);

    return errors ? 1 : 0;
}