#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

typedef enum {
    L_T_ID, L_T_KEYWORD, L_T_INT_LITERAL, L_T_FLOAT_LITERAL, L_T_STRING_LITERAL, L_T_STRUCT_LITERAL,
    L_T_PLUS, L_T_MINUS, L_T_PLUSPLUS, L_T_MINUSMINUS, L_T_STAR, L_T_SLASH, L_T_PERCENT,
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
        case L_T_PLUSPLUS: return "PLUSPLUS (++)";
        case L_T_MINUSMINUS: return "MINUSMINUS (--)";
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
    "if","elif","else","while","for","return","int","float","char","void","double", "bool", "true", "false", "struct", "end", "printf", NULL
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
        case '+': {
            if (match_lex('+')) return make_ltoken(L_T_PLUSPLUS, "++", 2, start_line, start_col);
            return make_ltoken(L_T_PLUS, "+", 1, start_line, start_col);
        }
        case '-': {
            if (match_lex('-')) return make_ltoken(L_T_MINUSMINUS, "--", 2, start_line, start_col);
            return make_ltoken(L_T_MINUS, "-", 1, start_line, start_col);
        }
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
    T_EOF, T_IF, T_ELIF, T_ELSE, T_WHILE, T_FOR, T_INT, T_FLOAT, T_BOOL, T_VOID, T_DOUBLE,
    T_ID, T_INT_LIT, T_FLOAT_LIT, T_TRUE, T_FALSE, T_AND, T_OR, T_NOT,
    T_SEMI, T_EQ, T_PLUS, T_MINUS, T_MUL, T_DIV, T_KEYWORD, T_PLUSPLUS, T_MINUSMINUS,
    T_GT, T_LT, T_GE, T_LE, T_EQEQ, T_NEQ,
    T_LBRACE, T_RBRACE, T_LPAREN, T_RPAREN, T_STRUCT, T_UNKNOWN
} TokenType;
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
    t.line = 1;
    return t;
}

static Token cur_tok;
Token ltoken_to_parser_token(const LToken *lt) {
    Token t;
    t.lexeme = strdup(lt->lexeme);
    t.line = lt->line;
    t.type = T_UNKNOWN;
    switch (lt->type) {
        case L_T_EOF: t.type = T_EOF; 
            break;
        case L_T_INT_LITERAL: t.type = T_INT_LIT;
            break;
        case L_T_FLOAT_LITERAL: t.type = T_FLOAT_LIT; 
            break;
        case L_T_ID: t.type = T_ID; 
            break;
        case L_T_STRING_LITERAL: t.type = T_UNKNOWN;
            break;
        case L_T_LPAREN: t.type = T_LPAREN; 
            break;
        case L_T_RPAREN: t.type = T_RPAREN;
            break;
        case L_T_PLUS: t.type = T_PLUS; 
            break;
        case L_T_MINUS: t.type = T_MINUS; 
            break;
        case L_T_PLUSPLUS: t.type = T_PLUSPLUS;
            break;
        case L_T_MINUSMINUS: t.type = T_MINUSMINUS;
            break;
        case L_T_STAR: t.type = T_MUL; 
            break;
        case L_T_SLASH: t.type = T_DIV; 
            break;
        case L_T_SEMICOLON: t.type = T_SEMI; 
            break;
        case L_T_LBRACE: t.type = T_LBRACE; 
            break;
        case L_T_RBRACE: t.type = T_RBRACE; 
            break;
        case L_T_ASSIGN: t.type = T_EQ; 
            break;
        case L_T_AND: t.type = T_AND;
            break;
        case L_T_OR: t.type = T_OR;
            break;
        case L_T_NOT: t.type = T_NOT;
            break;
        case L_T_EQ: t.type = T_EQEQ; 
            break;
        case L_T_NEQ: t.type = T_NEQ;
            break;
        case L_T_GT: t.type = T_GT; 
            break;               
        case L_T_LT: t.type = T_LT; 
            break;               
        case L_T_GE: t.type = T_GE; 
            break;            
        case L_T_LE: t.type = T_LE; 
            break; 
    
        case L_T_KEYWORD: {
            if (strcmp(lt->lexeme, "int") == 0) t.type = T_INT;
            else if(strcmp(lt->lexeme, "if") == 0) t.type = T_IF;
            else if(strcmp(lt->lexeme, "elif") == 0) t.type = T_ELIF; //considered ELIF because it is easier to work
            else if(strcmp(lt->lexeme, "else") == 0) t.type = T_ELSE;
            else if (strcmp(lt->lexeme, "float") == 0) t.type = T_FLOAT;
            else if (strcmp(lt->lexeme, "bool") == 0) t.type = T_BOOL;
            else if (strcmp(lt->lexeme, "void") == 0) t.type = T_VOID;
            else if (strcmp(lt->lexeme, "double") == 0) t.type = T_DOUBLE;
            else if (strcmp(lt->lexeme, "true") == 0) t.type = T_TRUE;
            else if (strcmp(lt->lexeme, "false") == 0) t.type = T_FALSE;
            else if (strcmp(lt->lexeme, "struct") == 0) t.type = T_STRUCT;
            else if (strcmp(lt->lexeme, "while") == 0) t.type = T_WHILE;
            else if (strcmp(lt->lexeme, "for") == 0) t.type = T_FOR;
            else t.type = T_UNKNOWN;
            break;
        }

        default:
            t.type = T_UNKNOWN;
            break;
    }
    return t;
}
Token get_next_parser_token() {
    LToken lt = lex_next_token();
    Token t = ltoken_to_parser_token(&lt);
    free_ltoken(&lt);
    return t;
}
void free_parser_token(Token *t) {
    if (t->lexeme) free(t->lexeme);
    t->lexeme = NULL;
}
typedef enum { TYPE_INT, TYPE_FLOAT, TYPE_BOOL, TYPE_ERROR } VarType;
const char* type_name(VarType t){
    switch(t){ case TYPE_INT: return "int"; case TYPE_FLOAT: return "float"; case TYPE_BOOL: return "bool"; default: return "error"; }
}
typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    OP_AND, OP_OR, OP_NOT,
    OP_INC, OP_DEC,
    OP_GT, OP_LT, OP_GE, OP_LE, OP_EQ,
    OP_UNKNOWN
} OpType;
typedef enum { EXPR_ID, EXPR_INT_LIT, EXPR_FLOAT_LIT, EXPR_BOOL_LIT, EXPR_BINOP, EXPR_UNOP } ExprKind;
typedef struct Expr {
    ExprKind kind;
    VarType inferred_type;
    int line;
    union {
        char *id; // EXPR_ID
        int ival; // EXPR_INT_LIT
        double fval; // EXPR_FLOAT_LIT
        int bval; // EXPR_BOOL_LIT
        struct { OpType op; struct Expr *left, *right; } bin;
        struct { OpType op; struct Expr *operand; } un;
    } u;
} Expr;

/* Statements: decl or assignment or block or expr */
typedef enum { STMT_DECL, STMT_ASSIGN, STMT_BLOCK, STMT_EXPR, STMT_WHILE, STMT_FOR, STMT_IF } StmtKind;
typedef struct Stmt {
    StmtKind kind;
    int line;
    union {
        struct { VarType vtype; char *id; Expr *init; 
                Expr *c; struct Stmt *then_branch; struct Stmt *else_branch;} decl;
        struct { char *id; Expr *expr; } assign;
        struct { struct Stmt **stmts; int n; } block;
        struct { Expr *expr; } expr_stmt;
        struct { Expr *cond; struct Stmt *body; } while_stmt;
        struct { struct Stmt *init; Expr *cond; struct Stmt *update; struct Stmt *body; } for_stmt;
        struct { Expr *cond; struct Stmt *then_branch; struct Stmt *else_branch; } if_stmt;
    } u;
} Stmt;

typedef struct {
    Stmt **stmts;
    int n;
} Program;

void advance();
Token next_token_from_lexer(); 
/*====================FORWARDS=====================*/
int is_eof_parser_token(TokenType t) { return t == T_EOF; }

int accept(TokenType t) { if (cur_tok.type == t) { advance(); return 1; } return 0; }
int expect(TokenType t, const char *errMsg) {
    if (cur_tok.type == t) { advance(); return 1; }
    fprintf(stderr, "Parse error (line %d): expected %s but found '%s'\n", cur_tok.line, errMsg, cur_tok.lexeme);
    exit(1);
}
/*===================FORWARD===================*/
Expr* parse_expr();
/*===================FORWARDS===================*/
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
    Expr *init = NULL;
    if (cur_tok.type == T_EQ) {
        advance();
        init = parse_expr();
    }
    expect(T_SEMI, "semicolon");
    Stmt *s = malloc(sizeof(Stmt)); s->kind = STMT_DECL; s->line = ln;
    s->u.decl.vtype = vt; s->u.decl.id = id; s->u.decl.init = init;
    s->u.decl.c = NULL; s->u.decl.then_branch = NULL; s->u.decl.else_branch = NULL;
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
Expr* make_binop_expr(OpType op, Expr *l, Expr *r, int ln) {
    Expr *e = malloc(sizeof(Expr));
    e->kind = EXPR_BINOP;
    e->line = ln;
    e->inferred_type = TYPE_ERROR;
    e->u.bin.op = op;
    e->u.bin.left = l;
    e->u.bin.right = r;
    return e;
}
Expr* make_unary_expr(OpType op, Expr *operand, int ln) {
    Expr *e = malloc(sizeof(Expr));
    e->kind = EXPR_UNOP;
    e->line = ln;
    e->inferred_type = TYPE_ERROR;
    e->u.un.op = op;
    e->u.un.operand = operand;
    return e;
}   
/*========================FORWARDS=======================*/
Expr* make_binop_expr();
Expr* parse_primary();
Expr* parse_unary();
Expr* parse_muldiv();
Expr* parse_add();
Expr* parse_rel_expr();
Expr* parse_and_expr();
Expr* parse_or_expr();
Expr* parse_expr();
/*========================FORWARDS=======================*/
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
        /* handle postfix ++/-- */
        if (cur_tok.type == T_PLUSPLUS || cur_tok.type == T_MINUSMINUS) {
            int is_inc = (cur_tok.type == T_PLUSPLUS);
            advance();
            return make_unary_expr(is_inc ? OP_INC : OP_DEC, e, ln);
        }
        return e;
    }
    if (accept(T_LPAREN)) {
        Expr *e = parse_expr();
        expect(T_RPAREN, "right parenthesis");
        return e;
    }
    fprintf(stderr,"Parse error (line %d): unexpected token '%s' in expression\n", cur_tok.line, cur_tok.lexeme);
    exit(1);
}
Expr* parse_unary() {
    if (cur_tok.type == T_NOT) {
        int ln = cur_tok.line;
        advance();
        Expr *operand = parse_unary();
        return make_unary_expr(OP_NOT, operand, ln);
    }
    if (cur_tok.type == T_PLUSPLUS || cur_tok.type == T_MINUSMINUS) {
        int ln = cur_tok.line;
        int is_inc = (cur_tok.type == T_PLUSPLUS);
        advance();
        Expr *operand = parse_unary();
        return make_unary_expr(is_inc ? OP_INC : OP_DEC, operand, ln);
    }
    return parse_primary();
}

Expr* parse_muldiv() {
    Expr *left = parse_unary();
    while (cur_tok.type == T_MUL || cur_tok.type == T_DIV) {
        OpType op = (cur_tok.type == T_MUL) ? OP_MUL : OP_DIV;
        int ln = cur_tok.line;
        advance();
        Expr *right = parse_unary();
        left = make_binop_expr(op, left, right, ln);
    }
    return left;
}
Expr* parse_add() {
    Expr *left = parse_muldiv();
    while (cur_tok.type == T_PLUS || cur_tok.type == T_MINUS) {
        OpType op = (cur_tok.type == T_PLUS) ? OP_ADD : OP_SUB;
        int ln = cur_tok.line;
        advance();
        Expr *right = parse_muldiv();
        left = make_binop_expr(op, left, right, ln);
    }
    return left;
}
Expr* parse_rel_expr() {
    Expr *left = parse_add();
    while (cur_tok.type == T_GT || cur_tok.type == T_LT || cur_tok.type == T_GE || cur_tok.type == T_LE || cur_tok.type == T_EQEQ || cur_tok.type == T_NEQ) {
        OpType op;
        int ln = cur_tok.line;
        switch (cur_tok.type) {
            case T_GT: op = OP_GT; break;
            case T_LT: op = OP_LT; break;
            case T_GE: op = OP_GE; break;
            case T_LE: op = OP_LE; break;
            case T_EQEQ: op = OP_EQ; break;
            case T_NEQ: op = OP_EQ; break;
            default: op = OP_UNKNOWN; break;
        }
        advance();
        Expr *right = parse_add();
        left = make_binop_expr(op, left, right, ln);
    }
    return left;
}
Expr* parse_and_expr() {
    Expr *left = parse_rel_expr();
    while (cur_tok.type == T_AND) {
        int ln = cur_tok.line;
        advance();
        Expr *right = parse_rel_expr();
        left = make_binop_expr(OP_AND, left, right, ln);
    }
    return left;
}
Expr* parse_or_expr() {
    Expr *left = parse_and_expr();
    while (cur_tok.type == T_OR) {
        int ln = cur_tok.line;
        advance();
        Expr *right = parse_and_expr();
        left = make_binop_expr(OP_OR, left, right, ln);
    }
    return left;
}
Expr* parse_expr() {
    return parse_or_expr();
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

Stmt* parse_block();
Stmt* parse_cond_decl();
Stmt* parse_expr_stmt();

/* peek next parser token type by using lexer but restoring scanner state */
TokenType peek_next_parser_token_type() {
    size_t saved_pos = scanner.pos;
    int saved_line = scanner.line;
    int saved_col = scanner.col;
    LToken lt = lex_next_token();
    Token t = ltoken_to_parser_token(&lt);
    TokenType ret = t.type;
    free_parser_token(&t);
    free_ltoken(&lt);
    scanner.pos = saved_pos;
    scanner.line = saved_line;
    scanner.col = saved_col;
    return ret;
}
// Peek parser token at offset (0 = next token from lexer)
Token peek_parser_token(int offset) {
    size_t saved_pos = scanner.pos;
    int saved_line = scanner.line;
    int saved_col = scanner.col;
    Token ret = make_parser_token(T_UNKNOWN, "");
    for (int k = 0; k <= offset; ++k) {
        LToken lt = lex_next_token();
        free_parser_token(&ret);
        ret = ltoken_to_parser_token(&lt);
        free_ltoken(&lt);
    }
    scanner.pos = saved_pos;
    scanner.line = saved_line;
    scanner.col = saved_col;
    return ret; // caller must free
}

Stmt* parse_while();
Stmt* parse_for();
Stmt* parse_stmt() {
    if (cur_tok.type == T_INT || cur_tok.type == T_FLOAT || cur_tok.type == T_BOOL) {
        Token t1 = peek_parser_token(1); 
        Token t2 = peek_parser_token(2); 
        Token t3 = peek_parser_token(3); 
        Token t4 = peek_parser_token(4); 

        int is_func = 0;
        if (t1.type == T_ID && t2.type == T_LPAREN && t3.type == T_RPAREN && t4.type == T_LBRACE) {
            is_func = 1;
        }

        free_parser_token(&t1); free_parser_token(&t2); free_parser_token(&t3); free_parser_token(&t4);
        if (is_func) {
            advance(); 
            advance(); 
            expect(T_LPAREN, "(");
            expect(T_RPAREN, ")");
            Stmt *body = parse_block();
            return body;
        }
        return parse_decl();
    }
    if (cur_tok.type == T_ID) {
        TokenType next = peek_next_parser_token_type();
        if (next == T_EQ) return parse_assign();
        else return parse_expr_stmt();
    }
    if (cur_tok.type == T_LBRACE) return parse_block();
    if(cur_tok.type == T_STRUCT) return parse_struct_decl();
    if(cur_tok.type == T_WHILE) return parse_while();
    if(cur_tok.type == T_FOR) return parse_for();
    if(cur_tok.type == T_IF || cur_tok.type == T_ELIF || cur_tok.type == T_ELSE) return parse_cond_decl();
    fprintf(stderr,"Parse error (line %d): unexpected token '%s' at statement start\n", cur_tok.line, cur_tok.lexeme);
    exit(1);
}

Stmt* parse_expr_stmt() {
    int ln = cur_tok.line;
    Expr *e = parse_expr();
    expect(T_SEMI, "semicolon");
    Stmt *s = malloc(sizeof(Stmt));
    s->kind = STMT_EXPR;
    s->line = ln;
    s->u.expr_stmt.expr = e;
    return s;
}

Stmt* parse_while() {
    int ln = cur_tok.line;
    expect(T_WHILE, "while");
    expect(T_LPAREN, "(");
    Expr *cond = parse_expr();
    expect(T_RPAREN, ")");
    Stmt *body = parse_stmt();
    Stmt *s = malloc(sizeof(Stmt));
    s->kind = STMT_WHILE;
    s->line = ln;
    s->u.while_stmt.cond = cond;
    s->u.while_stmt.body = body;
    return s;
}

Stmt* parse_for() {
    int ln = cur_tok.line;
    expect(T_FOR, "for");
    expect(T_LPAREN, "(");

    Stmt *init = NULL;
    if (cur_tok.type == T_SEMI) {
    } else if (cur_tok.type == T_INT || cur_tok.type == T_FLOAT || cur_tok.type == T_BOOL) {
        VarType vt = parse_type_token(cur_tok.type);
        int ln_decl = cur_tok.line;
        advance();
        if (cur_tok.type != T_ID){
            fprintf(stderr,"Parse error (line %d): expected identifier after type in for-init\n", ln_decl); exit(1); 
        }
        char *id = strdup(cur_tok.lexeme);
        advance(); 
        Expr *initexpr = NULL;
        if (cur_tok.type == T_EQ) {
            advance();
            initexpr = parse_expr();
        }
        Stmt *d = malloc(sizeof(Stmt)); d->kind = STMT_DECL; d->line = ln_decl;
        d->u.decl.vtype = vt; d->u.decl.id = id; d->u.decl.init = initexpr;
        d->u.decl.c = NULL; d->u.decl.then_branch = NULL; d->u.decl.else_branch = NULL;
        init = d;
    } else {
        if (cur_tok.type == T_ID) {
            TokenType next = peek_next_parser_token_type();
            if (next == T_EQ) {
                int lnupd = cur_tok.line;
                char *id = strdup(cur_tok.lexeme);
                advance();
                expect(T_EQ, "equals");
                Expr *e = parse_expr();
                Stmt *as = malloc(sizeof(Stmt)); as->kind = STMT_ASSIGN; as->line = lnupd;
                as->u.assign.id = id; as->u.assign.expr = e;
                init = as;
            } else {
                Expr *e = parse_expr();
                Stmt *es = malloc(sizeof(Stmt)); es->kind = STMT_EXPR; es->line = cur_tok.line;
                es->u.expr_stmt.expr = e;
                init = es;
            }
        } else {
            Expr *e = parse_expr();
            Stmt *es = malloc(sizeof(Stmt)); es->kind = STMT_EXPR; es->line = cur_tok.line;
            es->u.expr_stmt.expr = e;
            init = es;
        }
    }
    expect(T_SEMI, ";");
    Expr *cond = NULL;
    if (cur_tok.type != T_SEMI) {
        cond = parse_expr();
    }
    expect(T_SEMI, ";"); 
    Stmt *update = NULL;
    if (cur_tok.type != T_RPAREN) {
        if (cur_tok.type == T_ID) {
            TokenType next = peek_next_parser_token_type();
            if (next == T_EQ) {
                int lnupd = cur_tok.line;
                char *id = strdup(cur_tok.lexeme);
                advance();
                expect(T_EQ, "equals");
                Expr *e = parse_expr();
                Stmt *s = malloc(sizeof(Stmt)); s->kind = STMT_ASSIGN; s->line = lnupd;
                s->u.assign.id = id; s->u.assign.expr = e;
                update = s;
            } else {
                Expr *e = parse_expr();
                Stmt *s = malloc(sizeof(Stmt)); s->kind = STMT_EXPR; s->line = cur_tok.line;
                s->u.expr_stmt.expr = e;
                update = s;
            }
        } else {
            Expr *e = parse_expr();
            Stmt *s = malloc(sizeof(Stmt)); s->kind = STMT_EXPR; s->line = cur_tok.line;
            s->u.expr_stmt.expr = e;
            update = s;
        }
    }

    expect(T_RPAREN, ")");
    Stmt *body = parse_stmt();
    Stmt *s = malloc(sizeof(Stmt));
    s->kind = STMT_FOR;
    s->line = ln;
    s->u.for_stmt.init = init;
    s->u.for_stmt.cond = cond;
    s->u.for_stmt.update = update;
    s->u.for_stmt.body = body;
    return s;
}

Stmt* parse_cond_decl() {
    int ln = cur_tok.line;
    expect(T_IF, "if");
    expect(T_LPAREN, "(");
    Expr *cond = parse_expr();
    expect(T_RPAREN, ")");
    Stmt *then_branch = parse_stmt();
    Stmt *else_branch = NULL;
    while (cur_tok.type == T_ELIF) {
        int elif_ln = cur_tok.line;
        advance();
        expect(T_LPAREN, "(");
        Expr *elif_cond = parse_expr();
        expect(T_RPAREN, ")");
        Stmt *elif_branch = parse_stmt();
        Stmt *new_if = malloc(sizeof(Stmt));
        new_if->kind = STMT_IF;
        new_if->line = elif_ln;
        new_if->u.if_stmt.cond = elif_cond;
        new_if->u.if_stmt.then_branch = elif_branch;
        new_if->u.if_stmt.else_branch = NULL;

        if (else_branch == NULL) {
            else_branch = new_if;
        } else {
           Stmt *last = else_branch;
            while (last->u.if_stmt.else_branch != NULL)
                last = last->u.if_stmt.else_branch;
            last->u.if_stmt.else_branch = new_if;
        }
    }

    if (cur_tok.type == T_ELSE) {
        advance(); 
        Stmt *else_stmt = parse_stmt();
        else_branch = else_stmt;
    }

    Stmt *s = malloc(sizeof(Stmt));
    s->kind = STMT_IF;
    s->line = ln;
    s->u.if_stmt.cond = cond;
    s->u.if_stmt.then_branch = then_branch;
    s->u.if_stmt.else_branch = else_branch;
    return s;
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
        if ((cur_tok.type == T_INT || cur_tok.type == T_FLOAT || cur_tok.type == T_KEYWORD) ) {
            Token rt = cur_tok;
            advance();
            if (cur_tok.type == T_ID) {
                char *fname = strdup(cur_tok.lexeme);
                advance();
                if (cur_tok.type == T_LPAREN) {
                    advance();
                    if (cur_tok.type == T_RPAREN) {
                        advance();
                        Stmt *body = parse_block();
                        p->stmts = realloc(p->stmts, sizeof(Stmt*)*(p->n+1));
                        p->stmts[p->n++] = body;
                        continue;
                    }
                }
                free(fname);
            }
        }

        Stmt *s = parse_stmt();
        p->stmts = realloc(p->stmts, sizeof(Stmt*)*(p->n+1));
        p->stmts[p->n++] = s;
    }
    return p;
}
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
int is_numeric(VarType t) { return t == TYPE_INT || t == TYPE_FLOAT; }

VarType unify_numeric(VarType a, VarType b) {
    if (!is_numeric(a) || !is_numeric(b)) return TYPE_ERROR;
    if (a == TYPE_FLOAT || b == TYPE_FLOAT) return TYPE_FLOAT;
    return TYPE_INT;
}


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
    OpType op = e->u.bin.op;

    switch (op) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
            if (!is_numeric(lt) || !is_numeric(rt)) {
                sem_error(c, e->line,
                    "operator requires numeric operands (found %s and %s)",
                    type_name(lt), type_name(rt));
                return e->inferred_type = TYPE_ERROR;
            }
            return e->inferred_type = unify_numeric(lt, rt);
        case OP_GT:
        case OP_LT:
        case OP_GE:
        case OP_LE:
            if (!is_numeric(lt) || !is_numeric(rt)) {
                sem_error(c, e->line,
                    "relational operator requires numeric operands (found %s and %s)",
                    type_name(lt), type_name(rt));
                return e->inferred_type = TYPE_ERROR;
            }
            return e->inferred_type = TYPE_BOOL;
        case OP_EQ:
            /* equality: allow numeric==numeric or bool==bool (returns bool) */
            if (lt == rt && (is_numeric(lt) || lt == TYPE_BOOL)) {
                return e->inferred_type = TYPE_BOOL;
            }
            sem_error(c, e->line,
                "equality operator requires operands of same type (found %s and %s)",
                type_name(lt), type_name(rt));
            return e->inferred_type = TYPE_ERROR;
        case OP_AND:
        case OP_OR:
            if (lt != TYPE_BOOL || rt != TYPE_BOOL) {
                sem_error(c, e->line,
                    "logical operator requires boolean operands (found %s and %s)",
                    type_name(lt), type_name(rt));
                return e->inferred_type = TYPE_ERROR;
            }
            return e->inferred_type = TYPE_BOOL;

        default:
            sem_error(c, e->line, "unknown operator");
            return e->inferred_type = TYPE_ERROR;
            }
        }
    return TYPE_ERROR;
    }
}
void sem_check_stmt(SemCtx *c, Stmt *s) {
    if (!s) return;
    switch (s->kind) {
        case STMT_DECL: {
            if (s->u.decl.id) {
                if (symtable_lookup_in_scope(&c->table, s->u.decl.id, c->scope_level)) {
                    sem_error(c, s->line, "redeclaration of '%s' in the same scope", s->u.decl.id);
                } else {
                    symtable_add(&c->table, s->u.decl.id, s->u.decl.vtype, c->scope_level);
                }
                if (s->u.decl.init) {
                    VarType rt = sem_check_expr(c, s->u.decl.init);
                    if (rt == TYPE_ERROR) return;
                    if (s->u.decl.vtype == rt) return;
                    if (s->u.decl.vtype == TYPE_FLOAT && rt == TYPE_INT) {
                        return;
                    }
                    sem_error(c, s->line, "cannot initialize %s with %s for variable '%s'", type_name(rt), type_name(s->u.decl.vtype), s->u.decl.id);
                }
            } else {
                /* this branch is used by the overloaded IF/ELIF creation */
                /* check conditional node */
                VarType condt = sem_check_expr(c, s->u.decl.c);
                if (condt != TYPE_BOOL) {
                    sem_error(c, s->line, "condition does not evaluate to bool");
                }
                sem_check_stmt(c, s->u.decl.then_branch);
                if (s->u.decl.else_branch) sem_check_stmt(c, s->u.decl.else_branch);
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
        case STMT_EXPR: {
            sem_check_expr(c, s->u.expr_stmt.expr);
            return;
        }
        case STMT_WHILE: {
            VarType condt = sem_check_expr(c, s->u.while_stmt.cond);
            if (condt != TYPE_BOOL) sem_error(c, s->line, "condition does not evaluate to bool");
            sem_check_stmt(c, s->u.while_stmt.body);
            return;
        }
        case STMT_IF: {
            VarType condt = sem_check_expr(c, s->u.if_stmt.cond);
            if (condt != TYPE_BOOL) sem_error(c, s->line, "condition does not evaluate to bool");
            sem_check_stmt(c, s->u.if_stmt.then_branch);
            if (s->u.if_stmt.else_branch) sem_check_stmt(c, s->u.if_stmt.else_branch);
            return;
        }
        case STMT_FOR: {
            sem_enter_scope(c);
            if (s->u.for_stmt.init) sem_check_stmt(c, s->u.for_stmt.init);
            if (s->u.for_stmt.cond) {
                VarType condt = sem_check_expr(c, s->u.for_stmt.cond);
                if (condt != TYPE_BOOL) sem_error(c, s->line, "condition does not evaluate to bool");
            }
            if (s->u.for_stmt.update) sem_check_stmt(c, s->u.for_stmt.update);
            sem_check_stmt(c, s->u.for_stmt.body);
            sem_leave_scope(c);
            return;
        }
    }
}

int sem_check_program(Program *p) {
    SemCtx ctx; symtable_init(&ctx.table); ctx.scope_level = 0; ctx.errors = 0;
    for (int i=0;i<p->n;i++) sem_check_stmt(&ctx, p->stmts[i]);
    int errs = ctx.errors;
    symtable_free(&ctx.table);
    return errs;
}
void free_expr(Expr *e) {
    if (!e) return;
    if (e->kind == EXPR_ID) free(e->u.id);
    if (e->kind == EXPR_BINOP) { free_expr(e->u.bin.left); free_expr(e->u.bin.right); }
    if (e->kind == EXPR_UNOP) { free_expr(e->u.un.operand); }
    free(e);
}

void free_stmt(Stmt *s) {
    if (!s) return;
    if (s->kind == STMT_DECL) {
        if (s->u.decl.id) free(s->u.decl.id);
        if (s->u.decl.init) free_expr(s->u.decl.init);
        if (s->u.decl.c) free_expr(s->u.decl.c);
        if (s->u.decl.then_branch) free_stmt(s->u.decl.then_branch);
        if (s->u.decl.else_branch) free_stmt(s->u.decl.else_branch);
    }
    if (s->kind == STMT_ASSIGN) { free(s->u.assign.id); free_expr(s->u.assign.expr); }
    if (s->kind == STMT_BLOCK) {
        for (int i=0;i<s->u.block.n;i++) free_stmt(s->u.block.stmts[i]);
        free(s->u.block.stmts);
    }
    if (s->kind == STMT_EXPR) {
        free_expr(s->u.expr_stmt.expr);
    }
    if (s->kind == STMT_WHILE) {
        free_expr(s->u.while_stmt.cond);
        free_stmt(s->u.while_stmt.body);
    }
    if (s->kind == STMT_IF) {
        free_expr(s->u.if_stmt.cond);
        free_stmt(s->u.if_stmt.then_branch);
        if (s->u.if_stmt.else_branch) free_stmt(s->u.if_stmt.else_branch);
    }
    if (s->kind == STMT_FOR) {
        if (s->u.for_stmt.init) free_stmt(s->u.for_stmt.init);
        if (s->u.for_stmt.cond) free_expr(s->u.for_stmt.cond);
        if (s->u.for_stmt.update) free_stmt(s->u.for_stmt.update);
        if (s->u.for_stmt.body) free_stmt(s->u.for_stmt.body);
    }
    free(s);
}

void free_program(Program *p) {
    for (int i=0;i<p->n;i++) free_stmt(p->stmts[i]);
    free(p->stmts);
    free(p);
}
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
        printf("Semantical analysis completed: without errors.\n");
    else
        printf("Semantical analysis completed: %d error(s) found.\n", errors);

    /*This will free memory*/
    free_program(prog);
    free_parser_token(&cur_tok);
    free(scanner.buf);

    return errors ? 1 : 0;
}
