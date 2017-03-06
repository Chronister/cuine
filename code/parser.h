#ifndef PARSER_H
#define PARSER_H

/// Grammar rules

typedef int32_t cf_symbol_t;
#define ARRAY_TYPE cf_symbol_t
#include "array.c"
#define SET_TYPE cf_symbol_t
#include "set.c"
#define ARRAY_TYPE set(cf_symbol_t)
#include "array.c"

typedef struct {
    cf_symbol_t Nonterminal;
    array(cf_symbol_t) Sequence;
    parse_func* Func;
} cf_production;
#define ARRAY_TYPE cf_production
#include "array.c"

typedef struct {
    array(cf_production) Productions;
    cf_symbol_t Root;
    set(cf_symbol_t) FollowTable[NonterminalMAX];
    set(cf_symbol_t) FirstTable[NonterminalMAX];
} cf_grammar;


/// LR Parsing

typedef struct {
    int RuleNum; // Index in grammar.Productions
    int Position;
} lr_item;
#define ARRAY_TYPE lr_item 
#include "array.c"

struct lr_fsa_state;

typedef struct {
    cf_symbol_t Symbol;
    lr_item SourceItem;
    int DestIndex;
} lr_transition;
#define ARRAY_TYPE lr_transition
#include "array.c"

typedef struct lr_fsa_state {
    array(lr_item) Items;
    array(lr_transition) Transitions;
} lr_fsa_state;
#define ARRAY_TYPE lr_fsa_state
#include "array.c"

typedef enum {
    T_NONE = 0,
    T_SHIFT,
    T_REDUCE,
    T_GOTO,
} lr_table_transition_type;

typedef struct {
    lr_table_transition_type Type;
    union {
        int State; // Use if SHIFT or GOTO
        int RuleNum;
    };
} lr_table_transition;

typedef struct {
    // Table size is Rows * Cols * sizeof(lr_transition_table);
    lr_table_transition* Table;
    int Rows;
    int Cols;
} lr_parse_table;

#define cell(T, r, c) (T.Table[((r)* T.Cols + (c))])

void* Parse(tokenizer* Tokenizer);

#endif
