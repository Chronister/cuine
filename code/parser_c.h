#ifndef PARSER_C_H
#define PARSER_C_H

#include "grammar_c.h"

#define TerminalMIN C_TerminalMIN
#define TerminalMAX C_TerminalMAX
#define NonterminalMIN C_NonterminalMIN
#define NonterminalMAX C_NonterminalMAX

#define CST_NODE_(x) CST_##x

typedef void* cst_node;
#define ARRAY_TYPE cst_node
#include "array.c"

typedef enum {
    C_TERMINAL_LIST(CST_NODE_)
    C_NONTERMINAL_LIST(CST_NODE_)
    CST_NODE_MAX,
} cst_node_type;

typedef struct {
    cst_node_type Type;
    int SourceLine;
} cst_node_header;

typedef struct {
    cst_node_header Header;
    array(cst_node) ExternDecls;
} cst_node_translation_unit;

typedef struct {
    cst_node_header Header;

} cst_node_type_specifier;

typedef enum {
    DECL_TYPEDEF  = 1 << 0,
    DECL_EXTERN   = 1 << 1,
    DECL_STATIC   = 1 << 2,
    DECL_AUTO     = 1 << 3,
    DECL_REGISTER = 1 << 4,
    DECL_CONST    = 1 << 5,
    DECL_RESTRICT = 1 << 6,
    DECL_VOLATILE = 1 << 7,
    DECL_INLINE   = 1 << 8,
} cst_declaration_flag;
typedef intptr_t cst_declaration_flags;

typedef struct {
    cst_node_header Header;

    cst_node_type_specifier* Type;
    cst_declaration_flags SpecifierFlags;
} cst_node_declaration;

typedef struct {
    cst_node_header Header;
    array(cst_node) Declarations;
} cst_node_declaration_list;

typedef struct {
    cst_node_header Header;
    
} cst_node_function_definition;

typedef struct {
    void* Memory;
    size_t Used;
    size_t Allocated;
} c_context;

#define CF_MAX_SYMBOLS_PER_RULE 20

#define PushNode(Context, X) _PushNode(Context, (cst_node*)&(X), sizeof(X))
inline cst_node* 
_PushNode(c_context* Context, cst_node* NodePtr, size_t NodeSize)
{
    assert(Context->Used + NodeSize < Context->Allocated);
    cst_node* Result = (cst_node*)((uint8_t*)Context->Memory + Context->Used);
    Context->Used += NodeSize;
    memcpy(Result, NodePtr, NodeSize);
    return Result;
}


#define PARSE_FUNC(name, Context, Tokens, Parsed) void* name(c_context* Context, array(token) Tokens, array(cst_node) Parsed)
typedef PARSE_FUNC(parse_func, Context, Tokens, Parsed);

#define context c_context

#include "parser.h"

#endif
