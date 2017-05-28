#ifndef PARSER_L_H
#define PARSER_L_H
#include <stdint.h>
#include <assert.h>
#include "scanner.h"

#include "grammar_l.h"
#include "grammar_c.h"

#define TerminalMIN L_TerminalMIN
#define TerminalMAX L_TerminalMAX
#define NonterminalMIN L_NonterminalMIN
#define NonterminalMAX L_NonterminalMAX

// Simple test grammar to dry run everything

typedef enum {
    LST_Identifier,
    LST_List,
} lst_node_type; 

typedef struct {
    lst_node_type Type;
    int SourceLine;
} lst_node_header;

typedef lst_node_header lst_node;
#define ARRAY_TYPE lst_node
#include "array.c"

typedef struct lst_node_list {
    lst_node_header Header;
    
    struct lst_node_list* Next;
    struct lst_node_list* First;
    lst_node* Data;
} lst_node_list; 

typedef struct {
    lst_node_header Header;

    const char* Value;
    int ValueLength;
} lst_node_identifier;

typedef struct {
    void* Memory;
    size_t Used;
    size_t Allocated;
} l_context;

#define CF_MAX_SYMBOLS_PER_RULE 3

#define PushNode(Context, X) _PushNode(Context, (lst_node*)&(X), sizeof(X))
inline lst_node* 
_PushNode(l_context* Context, lst_node* NodePtr, size_t NodeSize)
{
    assert(Context->Used + NodeSize < Context->Allocated);
    lst_node* Result = (lst_node*)((uint8_t*)Context->Memory + Context->Used);
    Context->Used += NodeSize;
    memcpy(Result, NodePtr, NodeSize);
    return Result;
}

#define PARSE_FUNC(name, Context, Tokens, Parsed) lst_node* name(l_context* Context, array(token) Tokens, array(lst_node) Parsed)
typedef PARSE_FUNC(parse_func, Context, Tokens, Parsed);

#define context l_context

#endif
