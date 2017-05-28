#ifndef SCANNER_H
#define SCANNER_H
#include "grammar_c.h"

#define TOKEN_(x) TOKEN_##x

typedef enum {
    //C_TERMINAL_LIST(TOKEN_)
    TOKEN_L_IDENTIFIER,
    TOKEN_L_LPAREN,
    TOKEN_L_RPAREN,
    TOKEN_L_COMMA,
    TOKEN_L_EOF,
    TOKEN_MAX,
} token_type;

typedef struct {
    token_type Type;

    const char* Text;
    int TextLength;

    int LineStart;
    int LineEnd;
} token;

#define ARRAY_TYPE token
#include "array.c"

typedef struct {
    const char* Start;
    const char* LineStart;
    const char* At;

    int Line;

    const char* Filename;
} tokenizer;

extern const char* TokenType_Str(token_type Type);
extern token GetToken(tokenizer* Tzer);

#endif
