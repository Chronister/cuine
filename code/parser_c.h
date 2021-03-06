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
    X(TranslationUnit),     \
    X(Identifier),          \
    X(BuiltinType),         \
    X(StructuredType),      \
    X(EnumType),            \
    X(ArrayType),           \
    X(FunctionType),        \
    X(Declaration),         \
    X(DeclarationList),     \
    X(VariadicArgument),    \
    X(Block),               \
    X(StringConstant),      \
    X(IntegerConstant),     \
    X(FloatingConstant),    \
    X(CharacterConstant),   \
    X(BinaryOperator),      \
    X(UnaryOperator),       \
    X(Conditional),         \
    X(Jump),                \
    X(Cast),                \
    X(Assignment),          \
    X(Empty),               \
    X(Iteration),           \

#define CST_(x) CST_##x

typedef void* cst_node;
#define ARRAY_TYPE cst_node
#include "array.c"

typedef enum cst_node_type {
    CST_Invalid,
    CST_NODE_LIST(CST_)
    CST_NODE_MAX,
} cst_node_type;

const char* CST_NodeNames[] = {
    "(invalid)"
    CST_NODE_LIST(STR)
};

typedef struct {
    cst_node_type Type;
    int SourceLine;
} cst_node_header;

#define CST_NODE_TYPE(node) (node == NULL ? CST_Invalid : (*(cst_node_type*)(node)))

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
    cst_node_identifier* Name;
    array(cst_node) Declarations;
} cst_node_structured_type;

typedef struct {
    cst_node_header Header;

    cst_node BaseType;
    int Length; // -1 if not specified
} cst_node_array_type;

typedef struct {
    cst_node_header Header;

    array(cst_node) Statements; // CST_Statement or CST_Declaration
} cst_node_block;

typedef struct {
    cst_node_header Header;

    cst_node ReturnType;        // Any valid type-decl node
    cst_node_identifier* Name;  // Name of the function type. 
                                //  NULL if this is part of another declaration,
                                //  for example if the outer declaration is a function 
                                //  that returns a (non-typedef'd) function pointer
    array(cst_node) Arguments;  // CST_Declaration
    cst_node_block* Body;       // NULL if this is a declaration
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
    int BitfieldWidth;                          // Only in structs, ignored otherwise. 0 if not specified.
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
                                                //   CST_ArrayType node for an array type
                                                //   CST_FunctionType node for an function type

    cst_node_identifier* Name;                  // null if the variable name is not yet known
    cst_node Initializer;                       // null if uninitialized
} cst_node_declaration;

typedef struct {
    cst_node_header Header;
    array(cst_node) Declarations;
} cst_node_declaration_list;

typedef struct {
    cst_node_header Header;

    const char* Text;
    int TextLength;
} cst_node_string_constant;

typedef struct {
    cst_node_header Header;

    uint64_t Value;
} cst_node_integer_constant;

typedef struct {
    cst_node_header Header;

    double Value;
} cst_node_float_constant;

typedef struct {
    cst_node_header Header;

    int Value;
} cst_node_char_constant;

typedef struct {
    cst_node_header Header;
    //cst_node_label* Label; // NULL if unlabelled
} cst_node_expression;

typedef struct {
    cst_node_expression Expr;

    cf_symbol_t Operation;
    cst_node Left;
    cst_node Right;
} cst_node_binary_operator;

enum affix_t { PREFIX = 0, POSTFIX };

typedef struct {
    cst_node_expression Expr;

    cf_symbol_t Operation;
    cst_node Operand;
    enum affix_t Affix;
} cst_node_unary_operator;

typedef struct {
    cst_node_expression Expr;

    cst_node TargetType;
    cst_node Operand;
} cst_node_cast;

typedef struct {
    cst_node_expression Expr;

    cf_symbol_t Operator;
    cst_node LValue;
    cst_node RValue;
} cst_node_assignment;

typedef struct {
    cst_node_expression Expr;

    cst_node Condition;
    cst_node TrueBranch;
    cst_node FalseBranch; // Can be NULL
} cst_node_conditional;

typedef struct {
    cst_node_expression Expr;

    cf_symbol_t Type;
    cst_node Expression; // NULL unless Type is RETURN and an expression is being returned
} cst_node_jump;

typedef struct {
    cst_node_expression Expr;

    bool PostCondition; // In C, only set for do { } while() loops.
    // All of the following can be null (loop only terminates if there's a
    // break) or any combo can be set to an expression.
    cst_node Initialization;
    cst_node Condition;
    cst_node Increment;

    cst_node Body;
} cst_node_iteration;

///

typedef struct {
    void* Memory;
    size_t Used;
    size_t Allocated;
} c_context;

#define CF_MAX_SYMBOLS_PER_RULE 20

#define PushNode(Context, X) _PushNode(Context, (cst_node)&(X), sizeof(X))
always_inline cst_node
_PushNode(c_context* Context, cst_node NodePtr, size_t NodeSize)
{
    assert(Context->Used + NodeSize < Context->Allocated);
    cst_node Result = (cst_node)((uint8_t*)Context->Memory + Context->Used);
    Context->Used += NodeSize;
    memcpy(Result, NodePtr, NodeSize);
    return Result;
}


#define PARSE_FUNC(name, Context, Tokens, Parsed) void* name(c_context* Context, array(token) Tokens, array(cst_node) Parsed)
typedef PARSE_FUNC(parse_func, Context, Tokens, Parsed);

#define context c_context

#include "parser.h"

#endif
