#ifndef PARSER_C_H
#define PARSER_C_H


#define CST_NODE_(x) CST_NODE_##x

/*
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
    cst_node_declaration Decl;
    cst_node_declaration_list* Next;
} cst_node_declaration_list;

typedef struct {
    cst_node_header Header;
    cst_node_declaration_specifiers Specifiers;
    cst_node_declarator Declarator;
    cst_node_declaration_list* Declarator;
} cst_node_function_defn;

typedef struct {
    cst_node_header Header;
    cst_node_external_decl* Next;
    
    union {
        cst_node_type Type;
        cst_node_function_defn Func;
        cst_node_declaration Decl;
    } Child;
} cst_node_external_decl;

typedef struct {
    cst_node_header Header;
    cst_node_external_decl ExternalDeclList;
} cst_node_translation_unit;

*/

#endif
