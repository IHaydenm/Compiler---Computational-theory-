// Compilar con: gcc -std=c99 -O2 -o Analizador_Lexico lexAnalyzer.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    T_ID, T_KEYWORD, T_INT_LITERAL, T_FLOAT_LITERAL, T_STRING_LITERAL,
    T_PLUS, T_MINUS, T_STAR, T_SLASH, T_PERCENT,
    T_ASSIGN, T_EQ, T_NEQ, T_LT, T_LE, T_GT, T_GE,
    T_AND, T_OR, T_NOT,
    T_SEMICOLON, T_COMMA, T_LPAREN, T_RPAREN, T_LBRACE, T_RBRACE, T_LBRACKET, T_RBRACKET,
    T_EOF, T_UNKNOWN
} TokenType;

const char* tokenTypeName(TokenType t) {
    switch (t) {
        case T_ID: return "ID";
        case T_KEYWORD: return "KEYWORD";
        case T_INT_LITERAL: return "INT";
        case T_FLOAT_LITERAL: return "FLOAT";
        case T_STRING_LITERAL: return "STRING";
        case T_PLUS: return "PLUS";
        case T_MINUS: return "MINUS";
        case T_STAR: return "STAR";
        case T_SLASH: return "SLASH";
        case T_PERCENT: return "PERCENT";
        case T_ASSIGN: return "ASSIGN";
        case T_EQ: return "EQ";
        case T_NEQ: return "NEQ";
        case T_LT: return "LT";
        case T_LE: return "LE";
        case T_GT: return "GT";
        case T_GE: return "GE";
        case T_AND: return "AND";
        case T_OR: return "OR";
        case T_NOT: return "NOT";
        case T_SEMICOLON: return "SEMICOLON";
        case T_COMMA: return "COMMA";
        case T_LPAREN: return "LPAREN";
        case T_RPAREN: return "RPAREN";
        case T_LBRACE: return "LBRACE";
        case T_RBRACE: return "RBRACE";
        case T_LBRACKET: return "LBRACKET";
        case T_RBRACKET: return "RBRACKET";
        case T_EOF: return "EOF";
        default: return "UNKNOWN";
    }
}

const char* keywords[] = {
    "if","else","while","for","return","int","float","char","void","double", "bool", "true", "false", "end", "printf", NULL
};

int isKeyword(const char* token) {
    for (int i = 0; keywords[i]!=NULL; ++i)
        if (strcmp(token, keywords[i]) == 0){ 
            /*Comparing if lexium is a keyword*/
            return 1; //success code
        }
    return 0; //faliure code
}
typedef struct {
    char *buf;
    size_t len;
    size_t pos;
    int line;
    int col;
} Scanner;
Scanner scanner = {NULL,0,0,1,1}; //like in Java, the scanner will be used to read the information inside the file
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

int peek() {
    if (scanner.pos >= scanner.len) return EOF;
    return (unsigned char)scanner.buf[scanner.pos];
}
int getch() {
    int c = peek();
    if (c == EOF) return EOF;
    scanner.pos++;
    if (c == '\n') { scanner.line++; scanner.col = 1; }
    else scanner.col++;
    return c;
}
int match(char expected) {
    if (peek() == expected){
        getch(); 
        return 1; 
    }
    return 0;
}

typedef struct {
    TokenType type;
    char *lexeme;
    int line;
    int col;
} Token;

Token make_token(TokenType type, const char *lexeme_start, size_t len, int line, int col) {
    Token t;
    t.type = type;
    t.lexeme = malloc(len+1);
    if (!t.lexeme) { perror("malloc token"); exit(1); }
    memcpy(t.lexeme, lexeme_start, len);
    t.lexeme[len] = '\0';
    t.line = line;
    t.col = col;
    return t;
}

void free_token(Token *t) {
    free(t->lexeme);
    t->lexeme = NULL;
}

void skip_whitespace_and_comments() {
    int c;
    while ((c = peek()) != EOF) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            getch();
            continue;
        }
        if (c == '/') {
            // could be comment or divide
            if (scanner.pos+1 <= scanner.len && scanner.buf[scanner.pos+1] == '/') {
                //for a comment in line-type
                getch();
                getch();
                /*Taking both '//' of the comment line*/
                while (peek() != EOF && peek() != '\n') getch();
                continue;
            } else if (scanner.pos+1 <= scanner.len && scanner.buf[scanner.pos+1] == '*') {
                //for a comment on block-type
                getch(); 
                getch();
                /*Taking both '/*' of the comment block */ 
                int closed = 0;
                while (peek() != EOF) {
                    if (peek() == '*' && scanner.pos+1 <= scanner.len && scanner.buf[scanner.pos+1] == '/') {
                        getch(); 
                        getch();
                        /*Taking both '* /' of the comment block */ 
                        closed = 1; break;
                    } else getch();
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

Token next_token() {
    skip_whitespace_and_comments();
    int c = peek();
    if (c == EOF) return make_token(T_EOF, "", 0, scanner.line, scanner.col);

    // record start position for lexeme and location
    size_t start_pos = scanner.pos;
    int start_line = scanner.line, start_col = scanner.col;

    //For Ientifiers or keywords
    if (isalpha(c) || c == '_') {
        getch();
        while (isalnum(peek()) || peek() == '_') getch(); //Getting all the chars from the lexium for the ID or KEYWORD
        size_t len = scanner.pos - start_pos;
        char *s = scanner.buf + start_pos;
        /*APPARENTLY THE CODE READS UNTIL THERE IS A \0. Meaning that there will not be a distinction of a true KEYWORD and will be instead recognized as an ID*/
        char temp[len +1];
        memcpy(temp, s, len);//This line makes the program aware of the end of the string at \0 so that it is not consumed into the token
        temp[len] = '\0';
        Token t = make_token( isKeyword(temp) ? T_KEYWORD : T_ID, s, len, start_line, start_col );
        return t;
    }

    //For numbers int or float
    if (isdigit(c)) {
        int seen_dot = 0;
        getch();
        while (isdigit(peek()) || (peek() == '.' && !seen_dot)) {
            if (peek() == '.') {
                seen_dot = 1;
                getch();
                // if next isn't digit, it's an error (e.g., "12.")
                if (!isdigit(peek())) {
                    fprintf(stderr, "Error: número mal formado en linea %d col %d\n", scanner.line, scanner.col);
                    exit(1);
                }
            } else getch();
        }
        size_t len = scanner.pos - start_pos;
        Token t = make_token( seen_dot ? T_FLOAT_LITERAL : T_INT_LITERAL, scanner.buf + start_pos, len, start_line, start_col );
        return t;
    }

    // Cadenas
    if (c == '"') {
        getch(); // consume "
        while (peek() != EOF && peek() != '"') {
            if (peek() == '\\') { // escape
                getch(); // backslash
                if (peek() != EOF) getch(); // escaped char
            } else {
                if (peek() == '\n') {
                    //For this lexer, treat newline inside string as error
                    fprintf(stderr, "Error: cadena no terminada en linea %d col %d\n", start_line, start_col);
                    exit(1);
                }
                getch();
            }
        }
        if (peek() == '"') {
            getch(); // closing "
            size_t len = scanner.pos - start_pos;
            Token t = make_token(T_STRING_LITERAL, scanner.buf + start_pos, len, start_line, start_col);
            return t;
        } else {
            fprintf(stderr, "Error: cadena no terminada antes del EOF (linea %d col %d)\n", start_line, start_col);
            exit(1);
        }
    }

    // Operadores y separadores de múltiples caracteres
    // Two-char lookahead
    if (c == '=') {
        getch();
        if (match('=')) return make_token(T_EQ, "==", 2, start_line, start_col);
        else return make_token(T_ASSIGN, "=", 1, start_line, start_col);
    }
    if (c == '!') {
        getch();
        if (match('=')) return make_token(T_NEQ, "!=", 2, start_line, start_col);
        else return make_token(T_NOT, "!", 1, start_line, start_col);
    }
    if (c == '<') {
        getch();
        if (match('=')) return make_token(T_LE, "<=", 2, start_line, start_col);
        else return make_token(T_LT, "<", 1, start_line, start_col);
    }
    if (c == '>') {
        getch();
        if (match('=')) return make_token(T_GE, ">=", 2, start_line, start_col);
        else return make_token(T_GT, ">", 1, start_line, start_col);
    }
    if (c == '&') {
        getch();
        if (match('&')) return make_token(T_AND, "&&", 2, start_line, start_col);
        else { fprintf(stderr, "Error: '&' aislado en linea %d col %d\n", start_line, start_col); exit(1); }
    }
    if (c == '|') {
        getch();
        if (match('|')) return make_token(T_OR, "||", 2, start_line, start_col);
        else { fprintf(stderr, "Error: '|' aislado en linea %d col %d\n", start_line, start_col); exit(1); }
    }

    getch();
    switch (c) {
        case '+': return make_token(T_PLUS, "+", 1, start_line, start_col);
        case '-': return make_token(T_MINUS, "-", 1, start_line, start_col);
        case '*': return make_token(T_STAR, "*", 1, start_line, start_col);
        case '/': return make_token(T_SLASH, "/", 1, start_line, start_col);
        case '%': return make_token(T_PERCENT, "%", 1, start_line, start_col);
        case ';': return make_token(T_SEMICOLON, ";", 1, start_line, start_col);
        case ',': return make_token(T_COMMA, ",", 1, start_line, start_col);
        case '(': return make_token(T_LPAREN, "(", 1, start_line, start_col);
        case ')': return make_token(T_RPAREN, ")", 1, start_line, start_col);
        case '{': return make_token(T_LBRACE, "{", 1, start_line, start_col);
        case '}': return make_token(T_RBRACE, "}", 1, start_line, start_col);
        case '[': return make_token(T_LBRACKET, "[", 1, start_line, start_col);
        case ']': return make_token(T_RBRACKET, "]", 1, start_line, start_col);
        default:
            {
                // Unknown/illegal char
                char tmp[2] = {(char)c, '\0'};
                fprintf(stderr, "Error lexico: caracter no reconocido '%s' en linea %d col %d\n", tmp, start_line, start_col);
                exit(1);
                return make_token(T_UNKNOWN, tmp, 1, start_line, start_col);
            }
    }
}

// Utility: print token
void print_token(const Token *t) {
    printf("%4d:%3d  %-12s  %s\n", t->line, t->col, tokenTypeName(t->type), t->lexeme);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s archivo_fuente\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s ejemplo.c\n", argv[0]);
        return 1;
    }
    load_file(argv[1]);

    printf("LINE:COL   TOKEN        LEXEMA\n");
    printf("========================================\n");
    while (1) {
        Token t = next_token();
        print_token(&t);
        if (t.type == T_EOF) {
            free_token(&t);
            break;
        }
        free_token(&t);
    }

    free(scanner.buf);
    return 0;
}
