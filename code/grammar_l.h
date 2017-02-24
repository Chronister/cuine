#ifndef GRAMMAR_L_H
#define GRAMMAR_L_H

/// L-specific symbols

#define L_TERMINAL_LIST(X)  \
    X(EOF),                 \
    X(Identifier),          \
    X(LParen),              \
    X(RParen),              \
    X(Comma),               

#define L_NONTERMINAL_LIST(X)       \
    /* Test symbols) */             \
    X(S),                           \
    X(Sp),                          \
    X(L),               

#define L_(x) L_##x
#define STR(x) #x

typedef enum {
    L_TerminalMIN = -1,
    L_TERMINAL_LIST(L_)
    L_TerminalMAX,
} l_terminal_symbol;

typedef enum {
    L_NonterminalMIN = L_TerminalMAX - 1,
    L_NONTERMINAL_LIST(L_)
    L_NonterminalMAX,
} l_nonterminal_symbol;

const char* L_SymbolNames[] = {
    L_TERMINAL_LIST(STR)
    L_NONTERMINAL_LIST(STR)
};

#endif
