#ifndef PARSER_C_H
#define PARSER_C_H

#include "macro.h"
#include "types.h"
#include "grammar_c.h"

#define TerminalMIN C_TerminalMIN
#define TerminalMAX C_TerminalMAX
#define NonterminalMIN C_NonterminalMIN
#define NonterminalMAX C_NonterminalMAX

#define CST_NODE_LIST(X)    \
    X(Identifier),          \
    X(BuiltinType),         \
    X(StructuredType),      \
    X(EnumType),            \
    X(ArrayType),           \
    X(FunctionType),        \
    X(Declaration),         \
    X(DeclarationList),     \
    X(TranslationUnit),

#define CST_(x) CST_##x

typedef void* cst_node;
#define ARRAY_TYPE cst_node
#include "array.c"

typedef enum cst_node_type {
    CST_NODE_LIST(CST_)
    CST_NODE_MAX,
} cst_node_type;

const char* CST_NodeNames[] = {
    CST_NODE_LIST(STR)
};

typedef struct {
    cst_node_type Type;
    int SourceLine;
} cst_node_header;

typedef struct {
    cst_node_header Header;
    const char* Text;
    int TextLength;
} cst_node_identifier;

typedef struct {
    cst_node_header Header;
    array(cst_node) ExternDecls;
} cst_node_translation_unit;

typedef enum {
    TYPE_VOID       = FLAG(0),
    TYPE_BOOL       = FLAG(1),
    TYPE_CHAR       = FLAG(2),
    TYPE_SHORT      = FLAG(3),
    TYPE_INT        = FLAG(4),
    TYPE_LONG       = FLAG(5),
    TYPE_LONGLONG   = FLAG(6),
    TYPE_SIGNED     = FLAG(7),
    TYPE_UNSIGNED   = FLAG(8),
    TYPE_FLOAT      = FLAG(9),
    TYPE_DOUBLE     = FLAG(10),
    TYPE_COMPLEX    = FLAG(11),
} c_builtin_type_flag;
typedef uintptr_t c_builtin_type_flags;

typedef struct {
    cst_node_header Header;
    c_builtin_type_flags Type;
} cst_node_builtin_type;

typedef struct {
    cst_node_header Header;

    bool IsUnion;
    // TODO
} cst_node_structured_type;

typedef struct {
    cst_node_header Header;

    cst_node BaseType;
    int Length; // -1 if not specified
} cst_node_array_type;

typedef struct {
    cst_node_header Header;
    // TODO
} cst_node_function_type;

typedef enum {
    DECL_TYPEDEF  = FLAG(0),
    DECL_EXTERN   = FLAG(1),
    DECL_STATIC   = FLAG(2),
    DECL_AUTO     = FLAG(3),
    DECL_REGISTER = FLAG(4),
    DECL_CONST    = FLAG(5),
    DECL_RESTRICT = FLAG(6),
    DECL_VOLATILE = FLAG(7),
    DECL_INLINE   = FLAG(8),
} cst_declaration_flag;
typedef uintptr_t cst_declaration_flags;
#define ARRAY_TYPE cst_declaration_flags
#include "array.c"

// These are also used for partial values. During parsing, any
// field(s) may be left null to indicate values are not yet known
typedef struct {
    cst_node_header Header;

    cst_declaration_flags SpecifierFlags;       // 0 if no flags are known or specified
    array(cst_declaration_flags) PointerLevel;  // array whose length is the pointer level,
                                                // each entry of which indicates the qualifiers
                                                // of the type pointed to at that level
                                                //   e.g:
                                                //     int * const * volatile * Foo
                                                //   would result in the array:
                                                //     [0, DECL_VOLATILE, DECL_CONST]
                                                //   because the immediate type of the variable is
                                                //   an unqualified pointer, and it's pointing to a
                                                //   volatile pointer, and that points to a const
                                                //   pointer, which points to the base type.
                                                //   Likewise, A non-pointer type would have an array 
                                                //   length of 0.

    cst_node BaseType;                          // null if the base type is not yet known.
                                                // otherwise, may be either a:
                                                //   CST_BuiltinType node for a base type
                                                //   CST_Identifier node for a named type
                                                //   CST_StructuredType node for a structured type
                                                //   CST_EnumSpecifier node for an enum type
                                                //   CST_ArrayType node for an enum type
                                                //   CST_FunctionType node for an enum type

    cst_node_identifier* Name;                  // null if the variable name is not yet known
    cst_node Initializer;                       // null if uninitialized
} cst_node_declaration;

typedef struct {
    cst_node_header Header;
    array(cst_node) Declarations;
} cst_node_declaration_list;

typedef struct {
    void* Memory;
    size_t Used;
    size_t Allocated;
} c_context;

#define CF_MAX_SYMBOLS_PER_RULE 20

#define PushNode(Context, X) _PushNode(Context, (cst_node*)&(X), sizeof(X))
always_inline cst_node* 
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
