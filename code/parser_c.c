#include "grammar_c.h"
#include "parser_c.h"

extern const char*
SymbolStr(cf_symbol_t Sym) {
    if (Sym > C_TerminalMIN && Sym < C_NonterminalMAX) {
        return C_SymbolNames[Sym];
    }
    return "???";
}

extern const char*
NodeNameStr(cst_node_type NodeType) {
    if (NodeType >= 0 && NodeType < CST_NODE_MAX) {
        return CST_NodeNames[NodeType];
    }
    return "???";
}

static token
Process (token Token) { return Token; }

// TODO: include source file / column information in tokens

PARSE_FUNC(parse_SourceFile, Context, Tokens, Parsed) {
    return array_at(Parsed, 0);
}

PARSE_FUNC(parse_TranslationUnit, Context, Tokens, Parsed) {
    if (array_at(Tokens, 0).Type == ExternalDeclaration) {
        cst_node_translation_unit Node = { CST_TranslationUnit };
        Node.Header.SourceLine = array_at(Tokens, 0).LineStart;
        Node.ExternDecls = array_init(cst_node, 1, array_at(Parsed, 0));
        return PushNode(Context, Node);
    } else {
        assert(array_at(Tokens, 0).Type == TranslationUnit);
        assert(array_at(Tokens, 1).Type == ExternalDeclaration);
        cst_node_translation_unit* TU = (cst_node_translation_unit*)array_at(Parsed, 0);
        array_push(cst_node, &TU->ExternDecls, array_at(Parsed, 1));
        return array_at(Parsed, 0);
    }
}

PARSE_FUNC(parse_ExternDecl, Context, Tokens, Parsed) {
    return array_at(Parsed, 0);
}

PARSE_FUNC(parse_FunctionDefn, Context, Tokens, Parsed) {
    cst_node_function_type Node = { CST_FunctionType };
    //TODO
    cst_node_block* Block = NULL;
    if (Tokens.Length == 3) {
        Block = (cst_node_block*)array_at(Parsed, 2);
    }
    else if (Tokens.Length == 4) {
        Block = (cst_node_block*)array_at(Parsed, 3);
    }
    else_invalid;

    Node.Body = Block;
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_DeclList, Context, Tokens, Parsed) {
    cst_node_declaration_list Node = { CST_DeclarationList };
    //TODO
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_InitDeclarator, Context, Tokens, Parsed) {
    // Base declaration bubbles up from the Declarator
    cst_node_declaration* Node = (cst_node_declaration*)array_at(Parsed, 0);
    Node->Initializer = array_at(Parsed, 2);
    return Node;
}

PARSE_FUNC(parse_InitDeclaratorList, Context, Tokens, Parsed) {
    if (Tokens.Length == 1) {
        cst_node_declaration_list List = { CST_DeclarationList };
        List.Declarations = array_init(cst_node, 1, array_at(Parsed, 0));
        return PushNode(Context, List);
    }
    else if (Tokens.Length == 3) {
        cst_node_declaration_list* List = array_at(Parsed, 0);
        array_push(cst_node, &List->Declarations, array_at(Parsed, 2));
        return List;
    }
    else_invalid;
    return NULL;
}

PARSE_FUNC(parse_Declaration, Context, Tokens, Parsed) {
    if (Tokens.Length == 3) {
        // Base declaration list bubbles up from the init declarator list, and we
        // apply the decl specifiers to all of those
        cst_node_declaration* DeclSpecifiers = array_at(Parsed, 0); 
        cst_node_declaration_list* List = array_at(Parsed, 1); 

        array_for(cst_node, Node, List->Declarations) {
            cst_node_declaration* Decl = (cst_node_declaration*)Node;
            Decl->SpecifierFlags = DeclSpecifiers->SpecifierFlags;
            Decl->BaseType = DeclSpecifiers->BaseType;
        }
        // free(DeclSpecifiers);
        return List;
    } else if (Tokens.Length == 2) {
        assert(!"that rule you were confused by was hit");
        return NULL;
    }
    else_invalid;
    return NULL;
}

PARSE_FUNC(parse_DeclSpecifiers, Context, Tokens, Parsed) {
    cst_node_declaration Node = { CST_Declaration };

    array_for(token, Token, Tokens) {
        if (Token.Type == TypeSpecifier) {
            Node.BaseType = array_at(Parsed, TokenIndex);
        }
        else {
            assert(Token.Type == DeclarationQualifiers);
            Node.SpecifierFlags |= (intptr_t)array_at(Parsed, TokenIndex);
        }
    }

    return PushNode(Context, Node);
}

PARSE_FUNC(parse_DeclQualifiers, Context, Tokens, Parsed) {
    intptr_t Result = (cst_declaration_flag)array_at(Parsed, 0);
    if (Tokens.Length > 1) Result |= (cst_declaration_flag)array_at(Parsed, 1);
    return (void*)Result;
}

// Passes through the token type's corresponding enum flag as the data "pointer"
PARSE_FUNC(parse_TYPEDEF,  Context, Tokens, Parsed) { return (void*)(cst_declaration_flag)DECL_TYPEDEF; }
PARSE_FUNC(parse_EXTERN,   Context, Tokens, Parsed) { return (void*)(cst_declaration_flag)DECL_EXTERN; }
PARSE_FUNC(parse_STATIC,   Context, Tokens, Parsed) { return (void*)(cst_declaration_flag)DECL_STATIC; }
PARSE_FUNC(parse_AUTO,     Context, Tokens, Parsed) { return (void*)(cst_declaration_flag)DECL_AUTO; }
PARSE_FUNC(parse_REGISTER, Context, Tokens, Parsed) { return (void*)(cst_declaration_flag)DECL_REGISTER; }
PARSE_FUNC(parse_CONST,    Context, Tokens, Parsed) { return (void*)(cst_declaration_flag)DECL_CONST; }
PARSE_FUNC(parse_RESTRICT, Context, Tokens, Parsed) { return (void*)(cst_declaration_flag)DECL_RESTRICT; }
PARSE_FUNC(parse_VOLATILE, Context, Tokens, Parsed) { return (void*)(cst_declaration_flag)DECL_VOLATILE; }
PARSE_FUNC(parse_INLINE,   Context, Tokens, Parsed) { return (void*)(cst_declaration_flag)DECL_INLINE; }

PARSE_FUNC(parse_CHAR,     Context, Tokens, Parsed) { return (void*)(c_builtin_type_flags)TYPE_CHAR; }
PARSE_FUNC(parse_SHORT,    Context, Tokens, Parsed) { return (void*)(c_builtin_type_flags)TYPE_SHORT; }
PARSE_FUNC(parse_INT,      Context, Tokens, Parsed) { return (void*)(c_builtin_type_flags)TYPE_INT; }
PARSE_FUNC(parse_LONG,     Context, Tokens, Parsed) { return (void*)(c_builtin_type_flags)TYPE_LONG; }
PARSE_FUNC(parse_SIGNED,   Context, Tokens, Parsed) { return (void*)(c_builtin_type_flags)TYPE_SIGNED; }
PARSE_FUNC(parse_UNSIGNED, Context, Tokens, Parsed) { return (void*)(c_builtin_type_flags)TYPE_UNSIGNED; }
PARSE_FUNC(parse_FLOAT,    Context, Tokens, Parsed) { return (void*)(c_builtin_type_flags)TYPE_FLOAT; }
PARSE_FUNC(parse_DOUBLE,   Context, Tokens, Parsed) { return (void*)(c_builtin_type_flags)TYPE_DOUBLE; }
PARSE_FUNC(parse_COMPLEX,  Context, Tokens, Parsed) { return (void*)(c_builtin_type_flags)TYPE_COMPLEX; }

// Passes through the token type as the data "pointer"
PARSE_FUNC(parse_PassthroughTokenType,  Context, Tokens, Parsed) { return (void*)(uintptr_t)array_at(Tokens, 0).Type; }
PARSE_FUNC(parse_Passthrough,  Context, Tokens, Parsed) { return array_at(Parsed, 0); }
PARSE_FUNC(parse_PassthroughSecond,  Context, Tokens, Parsed) { return array_at(Parsed, 1); }

PARSE_FUNC(parse_Identifier, Context, Tokens, Parsed) { 
    cst_node_identifier Iden = { CST_Identifier };
    Iden.Text = array_at(Tokens, 0).Text;
    Iden.TextLength = array_at(Tokens, 0).TextLength;
    return PushNode(Context, Iden);
}

PARSE_FUNC(parse_TypeSpecifier, Context, Tokens, Parsed) {
    switch(array_at(Tokens, 0).Type) {
        case VOID:
        {
            cst_node_builtin_type T = { CST_BuiltinType };
            T.Type = TYPE_VOID;
            return PushNode(Context, T);
        }
            
        case _BOOL:
        {
            cst_node_builtin_type T = { CST_BuiltinType };
            T.Type = TYPE_BOOL;
            return PushNode(Context, T);
        }

        case MultiTypeList:
        {
            cst_node_builtin_type T = { CST_BuiltinType };
            T.Type = (c_builtin_type_flags)array_at(Parsed, 0);
            return PushNode(Context, T);
        }

        case StructOrUnionSpecifier:
        case EnumSpecifier:
            return array_at(Parsed, 0);

        default:
            assert(!"Unexpected child of type specifier!");
            return NULL;
    }
}

PARSE_FUNC(parse_Declarator, Context, Tokens, Parsed) {
    cst_node_declaration Decl = { CST_Declaration };
    cst_node_declaration* Result;
    cst_node Direct;
    if (Tokens.Length == 2) {
        // Combine the pointer declaration with the rest of the declaration
        Result = (cst_node_declaration*)array_at(Parsed, 0);
        Direct = array_at(Parsed, 1);
    } else {
        Result = (cst_node_declaration*)PushNode(Context, Decl);
        Direct = array_at(Parsed, 0);
    }

    switch (*(cst_node_type*)Direct) {
        case CST_Identifier:
            Result->Name = (cst_node_identifier*)Direct;
            break;
        case CST_FunctionType:
        case CST_ArrayType:
            //TODO
            return Direct;
        default:
            assert(!"TODO");
            break;
    }
    return Result;
}

PARSE_FUNC(parse_Pointer, Context, Tokens, Parsed) {
    if (Tokens.Length == 1) {
        cst_node_declaration PointerNode = { CST_Declaration };
        PointerNode.PointerLevel = array_init(cst_declaration_flags, 1, 0);
        return PushNode(Context, PointerNode);
    } 
    else if (Tokens.Length == 2 && array_at(Tokens, 1).Type == TypeQualifierList) {
        cst_node_declaration PointerNode = { CST_Declaration };
        PointerNode.PointerLevel = array_init(cst_declaration_flags, 1, 
                                              (cst_declaration_flags)array_at(Parsed, 1));
        return PushNode(Context, PointerNode);
    } 
    else if (Tokens.Length == 2 && array_at(Tokens, 1).Type == Pointer) {
        cst_node_declaration* PointerNode = (cst_node_declaration*)array_at(Parsed, 1);
        array_push(cst_declaration_flags, &PointerNode->PointerLevel, 0);
        return PointerNode;
    } else {
        cst_node_declaration* PointerNode = (cst_node_declaration*)array_at(Parsed, 2);
        array_push(cst_declaration_flags, 
                     &PointerNode->PointerLevel, 
                     (cst_declaration_flags)array_at(Parsed, 1));
        return PointerNode;
    }
}

PARSE_FUNC(parse_TypeQualifierList, Context, Tokens, Parsed) {
    if(array_at(Tokens, 0).Type == TypeQualifier) {
        return array_at(Parsed, 0);
    } else {
        return (void*)((cst_declaration_flags)array_at(Parsed, 0) | 
                       (cst_declaration_flags)array_at(Parsed, 1));
    }
}

PARSE_FUNC(parse_MultiTypeList, Context, Tokens, Parsed) {
    if(array_at(Tokens, 0).Type == MultiType) {
        return array_at(Parsed, 0);
    } else {
        return (void*)((c_builtin_type_flags)array_at(Parsed, 0) | 
                       (c_builtin_type_flags)array_at(Parsed, 1));
    }
}

PARSE_FUNC(parse_StructuredType, Context, Tokens, Parsed) 
{
    cst_node_structured_type Node = { CST_StructuredType };
    Node.IsUnion = ((uintptr_t)array_at(Parsed, 0) == UNION);
    //TODO
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_ArrayDecl, Context, Tokens, Parsed) {
    cst_node_array_type Node = { CST_ArrayType };
    //TODO
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_FunctionDecl, Context, Tokens, Parsed) {
    cst_node_array_type Node = { CST_FunctionType };
    //TODO
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_CompoundStmt, Context, Tokens, Parsed) {
    if (Tokens.Length == 2) {
        cst_node_block Block = { CST_Block };
        return PushNode(Context, Block);
    }
    else if (Tokens.Length == 3) {
        return array_at(Parsed, 1);
    }
    else_invalid;
    return NULL;
}

PARSE_FUNC(parse_BlockItemList, Context, Tokens, Parsed) {
    if (Tokens.Length == 1) {
        cst_node_block Block = { CST_Block };
        Block.Statements = array_init(cst_node, 1, array_at(Parsed, 0));
        return PushNode(Context, Block);
    } else if (Tokens.Length == 2) {
        cst_node_block* Block = (cst_node_block*)array_at(Parsed, 0);
        array_push(cst_node, &Block->Statements, array_at(Parsed, 1));
        return Block;
    }
    else_invalid;
    return NULL;
}

PARSE_FUNC(parse_StringLiteral, Context, Tokens, Parsed) {
    cst_node_string_constant Node = { CST_StringConstant };
    Node.Text = array_at(Tokens, 0).Text;
    Node.TextLength = array_at(Tokens, 0).TextLength;
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_IntegerConstant, Context, Tokens, Parsed) {
    // TODO integer parsing
    cst_node_string_constant Node = { CST_StringConstant };
    Node.Text = array_at(Tokens, 0).Text;
    Node.TextLength = array_at(Tokens, 0).TextLength;
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_FloatingConstant, Context, Tokens, Parsed) {
    // TODO float parsing
    cst_node_string_constant Node = { CST_StringConstant };
    Node.Text = array_at(Tokens, 0).Text;
    Node.TextLength = array_at(Tokens, 0).TextLength;
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_CharacterConstant, Context, Tokens, Parsed) {
    // TODO char parsing
    cst_node_string_constant Node = { CST_StringConstant };
    Node.Text = array_at(Tokens, 0).Text;
    Node.TextLength = array_at(Tokens, 0).TextLength;
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_BinaryExpression, Context, Tokens, Parsed) {
    cst_node_binary_operator Node = { CST_BinaryOperator };
    Node.Operation = array_at(Tokens, 1).Type;
    Node.Left = array_at(Parsed, 0);
    Node.Right = array_at(Parsed, 2);
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_If, Context, Tokens, Parsed) {
    cst_node_conditional Node = { CST_Conditional };
    Node.Condition = array_at(Parsed, 2);
    Node.TrueBranch = array_at(Parsed, 4);

    if (Tokens.Length > 5) Node.FalseBranch = array_at(Parsed, 6);
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_JumpStmt, Context, Tokens, Parsed) {
    cst_node_jump Node = { CST_Jump };
    Node.Type = array_at(Tokens, 0).Type;
    if (Tokens.Length == 3) Node.Expression = array_at(Parsed, 1);
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_AssignmentExpr, Context, Tokens, Parsed) {
    cst_node_assignment Node = { CST_Assignment };

    Node.Operator = (uintptr_t)array_at(Parsed, 1);
    Node.LValue = array_at(Parsed, 0);
    Node.RValue = array_at(Parsed, 2);

    return PushNode(Context, Node);
}

PARSE_FUNC(parse_ruleX, Context, Tokens, Parsed) {
    //array_for(token, Token, Tokens) {
    //    printf("\tTokens[%d]: %s\n", TokenIndex, SymbolStr(Token.Type));
    //}
    return NULL;
}


cf_grammar GenerateGrammar()
{
    cf_production Rules[] = {
        // Augment grammar with an extra rule
        GrammarRule(parse_SourceFile, SourceFile,  TranslationUnit, EndOfFile),

        /* § A.1.5 Constants*/
        GrammarRule(parse_IntegerConstant, Constant,  IntegerConstant),
        GrammarRule(parse_FloatingConstant, Constant,  FloatingConstant),
        GrammarRule(parse_CharacterConstant, Constant,  CharacterConstant),

        /* § A.2.1 Expressions */
        GrammarRule(parse_Identifier, PrimaryExpression,  Identifier),
        GrammarRule(parse_StringLiteral, PrimaryExpression,  StringLiteral),
        GrammarRule(parse_Passthrough, PrimaryExpression,  Constant),
        GrammarRule(parse_PassthroughSecond, PrimaryExpression,  LParen, Expression, RParen),

        GrammarRule(parse_Passthrough, PostfixExpression,  PrimaryExpression),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, LBracket, Expression, RBracket),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, LParen, RParen),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, LParen, ArgumentExpressionList, RParen),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, Dot, Identifier),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, Arrow, Identifier),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, Increment),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, Decrement),
        GrammarRule(parse_ruleX, PostfixExpression,  LParen, TypeName, RParen, LCurly, InitializerList, RCurly),
        GrammarRule(parse_ruleX, PostfixExpression,  LParen, TypeName, RParen, LCurly, InitializerList, RCurly, Comma),

        GrammarRule(parse_Passthrough, ArgumentExpressionList,  AssignmentExpression),
        GrammarRule(parse_ruleX, ArgumentExpressionList,  ArgumentExpressionList, AssignmentExpression),

        GrammarRule(parse_Passthrough, UnaryExpression,  PostfixExpression),
        GrammarRule(parse_ruleX, UnaryExpression,  Increment, UnaryExpression),
        GrammarRule(parse_ruleX, UnaryExpression,  Decrement, UnaryExpression),
        GrammarRule(parse_ruleX, UnaryExpression,  UnaryOperator, CastExpression),
        GrammarRule(parse_ruleX, UnaryExpression,  SIZEOF, UnaryExpression),
        GrammarRule(parse_ruleX, UnaryExpression,  SIZEOF, LParen, TypeName, RParen),
        
        GrammarRule(parse_ruleX, UnaryOperator,  Ampersand),
        GrammarRule(parse_ruleX, UnaryOperator,  Asterisk),
        GrammarRule(parse_ruleX, UnaryOperator,  Plus),
        GrammarRule(parse_ruleX, UnaryOperator,  Minus),
        GrammarRule(parse_ruleX, UnaryOperator,  BitNot),
        GrammarRule(parse_ruleX, UnaryOperator,  LogicNot),

        GrammarRule(parse_Passthrough, CastExpression,  UnaryExpression),
        GrammarRule(parse_ruleX, CastExpression,  LParen, TypeName, RParen, CastExpression),

        GrammarRule(parse_Passthrough, MultiplicativeExpression,  CastExpression),
        GrammarRule(parse_BinaryExpression, MultiplicativeExpression,  MultiplicativeExpression, Asterisk, CastExpression),
        GrammarRule(parse_BinaryExpression, MultiplicativeExpression,  MultiplicativeExpression, Divide, CastExpression),
        GrammarRule(parse_BinaryExpression, MultiplicativeExpression,  MultiplicativeExpression, Modulus, CastExpression),

        GrammarRule(parse_Passthrough, AdditiveExpression,  MultiplicativeExpression),
        GrammarRule(parse_BinaryExpression, AdditiveExpression,  AdditiveExpression, Plus, MultiplicativeExpression),
        GrammarRule(parse_BinaryExpression, AdditiveExpression,  AdditiveExpression, Minus, MultiplicativeExpression),

        GrammarRule(parse_Passthrough, ShiftExpression,  AdditiveExpression),
        GrammarRule(parse_BinaryExpression, ShiftExpression,  ShiftExpression, LBitShift, AdditiveExpression),
        GrammarRule(parse_BinaryExpression, ShiftExpression,  ShiftExpression, RBitShift, AdditiveExpression),

        GrammarRule(parse_Passthrough, RelationalExpression,  ShiftExpression),
        GrammarRule(parse_BinaryExpression, RelationalExpression,  RelationalExpression, Less, ShiftExpression),
        GrammarRule(parse_BinaryExpression, RelationalExpression,  RelationalExpression, Greater, ShiftExpression),
        GrammarRule(parse_BinaryExpression, RelationalExpression,  RelationalExpression, LessEquals, ShiftExpression),
        GrammarRule(parse_BinaryExpression, RelationalExpression,  RelationalExpression, GreaterEquals, ShiftExpression),

        GrammarRule(parse_Passthrough, EqualityExpression,  RelationalExpression),
        GrammarRule(parse_BinaryExpression, EqualityExpression,  EqualityExpression, Equals, RelationalExpression),
        GrammarRule(parse_BinaryExpression, EqualityExpression,  EqualityExpression, LogicNotEquals, RelationalExpression),

        GrammarRule(parse_Passthrough, ANDExpression,  EqualityExpression),
        GrammarRule(parse_BinaryExpression, ANDExpression,  ANDExpression, Ampersand, EqualityExpression),

        GrammarRule(parse_Passthrough, ExclusiveORExpression,  ANDExpression),
        GrammarRule(parse_BinaryExpression, ExclusiveORExpression,  ExclusiveORExpression, BitXor, ANDExpression),

        GrammarRule(parse_Passthrough, InclusiveORExpression, ExclusiveORExpression),
        GrammarRule(parse_BinaryExpression, InclusiveORExpression, InclusiveORExpression, BitOr, ANDExpression),

        GrammarRule(parse_Passthrough, LogicalANDExpression,  InclusiveORExpression),
        GrammarRule(parse_BinaryExpression, LogicalANDExpression,  LogicalANDExpression, LogicAnd, InclusiveORExpression),

        GrammarRule(parse_Passthrough, LogicalORExpression,  LogicalANDExpression),
        GrammarRule(parse_BinaryExpression, LogicalORExpression,  LogicalORExpression, LogicOr, LogicalANDExpression),

        GrammarRule(parse_Passthrough, ConditionalExpression,  LogicalORExpression),
        GrammarRule(parse_ruleX, ConditionalExpression,  LogicalORExpression, QuestionMark, Expression, Colon, ConditionalExpression),

        GrammarRule(parse_Passthrough, AssignmentExpression,  ConditionalExpression),
        GrammarRule(parse_AssignmentExpr, AssignmentExpression,  UnaryExpression, AssignmentOperator, AssignmentExpression),

        GrammarRule(parse_PassthroughTokenType, AssignmentOperator,  Assign),
        GrammarRule(parse_PassthroughTokenType, AssignmentOperator,  TimesEquals),
        GrammarRule(parse_PassthroughTokenType, AssignmentOperator,  DivideEquals),
        GrammarRule(parse_PassthroughTokenType, AssignmentOperator,  ModulusEquals),
        GrammarRule(parse_PassthroughTokenType, AssignmentOperator,  PlusEquals),
        GrammarRule(parse_PassthroughTokenType, AssignmentOperator,  MinusEquals),
        GrammarRule(parse_PassthroughTokenType, AssignmentOperator,  LBitShiftEquals),
        GrammarRule(parse_PassthroughTokenType, AssignmentOperator,  RBitShiftEquals),
        GrammarRule(parse_PassthroughTokenType, AssignmentOperator,  BitAndEquals),
        GrammarRule(parse_PassthroughTokenType, AssignmentOperator,  BitXorEquals),
        GrammarRule(parse_PassthroughTokenType, AssignmentOperator,  BitOrEquals),

        GrammarRule(parse_Passthrough, Expression,  AssignmentExpression),
        GrammarRule(parse_ruleX, Expression,  Expression, Comma, AssignmentExpression),

        GrammarRule(parse_Passthrough, ConstantExpression,  ConditionalExpression),

        /* § A.2.2 Declarations */
        // TODO why is this ↓ rule here
        GrammarRule(parse_Declaration, Declaration,  DeclarationSpecifiers, Semicolon),
        GrammarRule(parse_Declaration, Declaration,  DeclarationSpecifiers, InitDeclaratorList, Semicolon),

        GrammarRule(parse_DeclSpecifiers, DeclarationSpecifiers,  TypeSpecifier),
        GrammarRule(parse_DeclSpecifiers, DeclarationSpecifiers,  DeclarationQualifiers, TypeSpecifier),
        GrammarRule(parse_DeclSpecifiers, DeclarationSpecifiers,  TypeSpecifier, DeclarationQualifiers),
        GrammarRule(parse_DeclSpecifiers, DeclarationSpecifiers,  DeclarationQualifiers, TypeSpecifier, DeclarationQualifiers),

        GrammarRule(parse_DeclQualifiers, DeclarationQualifiers,  StorageClassSpecifier),
        GrammarRule(parse_DeclQualifiers, DeclarationQualifiers,  StorageClassSpecifier, DeclarationQualifiers),
        GrammarRule(parse_DeclQualifiers, DeclarationQualifiers,  TypeQualifier),
        GrammarRule(parse_DeclQualifiers, DeclarationQualifiers,  TypeQualifier, DeclarationQualifiers),
        GrammarRule(parse_DeclQualifiers, DeclarationQualifiers,  FunctionSpecifier),
        GrammarRule(parse_DeclQualifiers, DeclarationQualifiers,  FunctionSpecifier, DeclarationQualifiers),

        GrammarRule(parse_InitDeclaratorList, InitDeclaratorList,  InitDeclarator),
        GrammarRule(parse_InitDeclaratorList, InitDeclaratorList,  InitDeclaratorList, Comma, InitDeclarator),

        GrammarRule(parse_Passthrough,    InitDeclarator,  Declarator),
        GrammarRule(parse_InitDeclarator, InitDeclarator,  Declarator, Assign, Initializer),

        GrammarRule(parse_TYPEDEF, StorageClassSpecifier,  TYPEDEF),
        GrammarRule(parse_EXTERN, StorageClassSpecifier,  EXTERN),
        GrammarRule(parse_STATIC, StorageClassSpecifier,  STATIC),
        GrammarRule(parse_AUTO, StorageClassSpecifier,  AUTO),
        GrammarRule(parse_REGISTER, StorageClassSpecifier,  REGISTER),

        GrammarRule(parse_TypeSpecifier, TypeSpecifier,  VOID),
        GrammarRule(parse_TypeSpecifier, TypeSpecifier,  _BOOL),
        GrammarRule(parse_TypeSpecifier, TypeSpecifier,  MultiTypeList),
        GrammarRule(parse_TypeSpecifier, TypeSpecifier,  StructOrUnionSpecifier),
        GrammarRule(parse_TypeSpecifier, TypeSpecifier,  EnumSpecifier),
        GrammarRule(parse_Identifier,    TypeSpecifier,  Identifier),

        // Added to the grammar to account for type names like "unsigned long long Foo"
        // being valid, but type names like "foo bar Foo" being probably invalid
        // and causing problems with parsing
        // {
        GrammarRule(parse_MultiTypeList, MultiTypeList,  MultiType),
        GrammarRule(parse_MultiTypeList, MultiTypeList,  MultiTypeList, MultiType),

        GrammarRule(parse_CHAR,     MultiType,  CHAR),
        GrammarRule(parse_SHORT,    MultiType,  SHORT),
        GrammarRule(parse_INT,      MultiType,  INT),
        GrammarRule(parse_LONG,     MultiType,  LONG),
        GrammarRule(parse_SIGNED,   MultiType,  SIGNED),
        GrammarRule(parse_UNSIGNED, MultiType,  UNSIGNED),
        GrammarRule(parse_FLOAT,    MultiType,  FLOAT),
        GrammarRule(parse_DOUBLE,   MultiType,  DOUBLE),
        GrammarRule(parse_COMPLEX,  MultiType,  _COMPLEX),
        // }

        GrammarRule(parse_StructuredType, StructOrUnionSpecifier,  StructOrUnion, LCurly, StructDeclarationList, RCurly),
        GrammarRule(parse_StructuredType, StructOrUnionSpecifier,  StructOrUnion, Identifier, LBracket, StructDeclarationList, RBracket),
        GrammarRule(parse_StructuredType, StructOrUnionSpecifier,  StructOrUnion, Identifier),

        GrammarRule(parse_PassthroughTokenType, StructOrUnion,  STRUCT),
        GrammarRule(parse_PassthroughTokenType, StructOrUnion,  UNION),

        GrammarRule(parse_ruleX, StructDeclarationList,  StructDeclaration),
        GrammarRule(parse_ruleX, StructDeclarationList,  StructDeclarationList, StructDeclaration),

        GrammarRule(parse_ruleX, StructDeclaration,  SpecifierQualifierList, StructDeclaratorList),

        GrammarRule(parse_ruleX, SpecifierQualifierList,  TypeSpecifier),
        GrammarRule(parse_ruleX, SpecifierQualifierList,  TypeSpecifier, SpecifierQualifierList),
        GrammarRule(parse_ruleX, SpecifierQualifierList,  TypeQualifier),
        GrammarRule(parse_ruleX, SpecifierQualifierList,  TypeQualifier, SpecifierQualifierList),

        GrammarRule(parse_ruleX, StructDeclaratorList,  StructDeclarator),
        GrammarRule(parse_ruleX, StructDeclaratorList,  StructDeclaratorList, Comma, StructDeclarator),

        GrammarRule(parse_ruleX, StructDeclarator,  Declarator),
        GrammarRule(parse_ruleX, StructDeclarator,  Colon, ConstantExpression),
        GrammarRule(parse_ruleX, StructDeclarator,  Declarator, Colon, ConstantExpression),

        GrammarRule(parse_ruleX, EnumSpecifier,  ENUM, LCurly, EnumeratorList, RCurly),
        GrammarRule(parse_ruleX, EnumSpecifier,  ENUM, Identifier, LCurly, EnumeratorList, RCurly),
        GrammarRule(parse_ruleX, EnumSpecifier,  ENUM, LCurly, EnumeratorList, Comma, RCurly),
        GrammarRule(parse_ruleX, EnumSpecifier,  ENUM, Identifier, LCurly, EnumeratorList, Comma, RCurly),
        GrammarRule(parse_Identifier, EnumSpecifier,  ENUM, Identifier),

        GrammarRule(parse_ruleX, EnumeratorList,  Enumerator),
        GrammarRule(parse_ruleX, EnumeratorList,  EnumeratorList, Comma, Enumerator),

        GrammarRule(parse_ruleX, Enumerator,  EnumerationConstant),
        GrammarRule(parse_ruleX, Enumerator,  EnumerationConstant, Equals, ConstantExpression),

        GrammarRule(parse_CONST, TypeQualifier,  CONST),
        GrammarRule(parse_RESTRICT, TypeQualifier,  RESTRICT),
        GrammarRule(parse_VOLATILE, TypeQualifier,  VOLATILE),

        GrammarRule(parse_INLINE, FunctionSpecifier,  INLINE),

        GrammarRule(parse_Declarator, Declarator,  DirectDeclarator),
        GrammarRule(parse_Declarator, Declarator,  Pointer, DirectDeclarator),

        GrammarRule(parse_Identifier, DirectDeclarator,  Identifier),
        GrammarRule(parse_ruleX, DirectDeclarator,  LParen, Declarator, RParen),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, AssignmentExpression, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, AssignmentExpression, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, STATIC, AssignmentExpression, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, STATIC, TypeQualifierList, AssignmentExpression, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, STATIC, AssignmentExpression, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, Asterisk, RBracket),
        GrammarRule(parse_FunctionDecl, DirectDeclarator,  DirectDeclarator, LParen, ParameterTypeList, RParen),
        GrammarRule(parse_FunctionDecl, DirectDeclarator,  DirectDeclarator, LParen, RParen),
        GrammarRule(parse_FunctionDecl, DirectDeclarator,  DirectDeclarator, LParen, IdentifierList, RParen),

        GrammarRule(parse_Pointer, Pointer,  Asterisk),
        GrammarRule(parse_Pointer, Pointer,  Asterisk, TypeQualifierList),
        GrammarRule(parse_Pointer, Pointer,  Asterisk, Pointer),
        GrammarRule(parse_Pointer, Pointer,  Asterisk, TypeQualifierList, Pointer),

        GrammarRule(parse_TypeQualifierList, TypeQualifierList,  TypeQualifier),
        GrammarRule(parse_TypeQualifierList, TypeQualifierList,  TypeQualifierList, TypeQualifier),

        GrammarRule(parse_ruleX, ParameterTypeList,  ParameterList),
        GrammarRule(parse_ruleX, ParameterTypeList,  ParameterList, Comma, Ellipses),

        GrammarRule(parse_ruleX, ParameterList,  ParameterDeclaration),
        GrammarRule(parse_ruleX, ParameterList,  ParameterList, Comma, ParameterDeclaration),

        GrammarRule(parse_ruleX, ParameterDeclaration,  DeclarationSpecifiers, Declarator),
        GrammarRule(parse_ruleX, ParameterDeclaration,  DeclarationSpecifiers),
        GrammarRule(parse_ruleX, ParameterDeclaration,  DeclarationSpecifiers, AbstractDeclarator),

        GrammarRule(parse_ruleX, IdentifierList,  Identifier),
        GrammarRule(parse_ruleX, IdentifierList,  IdentifierList, Comma, Identifier),

        GrammarRule(parse_ruleX, TypeName,  SpecifierQualifierList),
        GrammarRule(parse_ruleX, TypeName,  SpecifierQualifierList, AbstractDeclarator),

        GrammarRule(parse_ruleX, AbstractDeclarator,  Pointer),
        GrammarRule(parse_ruleX, AbstractDeclarator,  DirectAbstractDeclarator),
        GrammarRule(parse_ruleX, AbstractDeclarator,  Pointer, DirectAbstractDeclarator),

        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  LParen, AbstractDeclarator, RParen),

        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  LBracket, RBracket),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, RBracket),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  LBracket, TypeQualifierList, RBracket),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  LBracket, AssignmentExpression, RBracket),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, TypeQualifierList, RBracket),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, AssignmentExpression, RBracket),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  LBracket, TypeQualifierList, AssignmentExpression, RBracket),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, TypeQualifierList, AssignmentExpression, RBracket),

        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  LBracket, STATIC, AssignmentExpression, RBracket),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, STATIC, AssignmentExpression, RBracket),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  LBracket, STATIC, TypeQualifierList, AssignmentExpression, RBracket),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, STATIC, TypeQualifierList, AssignmentExpression, RBracket),

        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  LBracket, TypeQualifierList, STATIC, AssignmentExpression, RBracket),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, TypeQualifierList, STATIC, AssignmentExpression, RBracket),

        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  LBracket, Asterisk, RBracket),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, Asterisk, RBracket),

        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  LParen, RParen),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  DirectAbstractDeclarator, LParen, RParen),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  LParen, ParameterTypeList, RParen),
        GrammarRule(parse_ruleX, DirectAbstractDeclarator,  DirectAbstractDeclarator, LParen, ParameterTypeList, RParen),

        // x GrammarRule(parse_ruleX, TypedefName,  Identifier),

        GrammarRule(parse_Passthrough, Initializer,  AssignmentExpression),
        GrammarRule(parse_ruleX, Initializer,  LCurly, InitializerList, RCurly),
        GrammarRule(parse_ruleX, Initializer,  LCurly, InitializerList, Comma, RCurly),

        GrammarRule(parse_ruleX, InitializerList,  Initializer),
        GrammarRule(parse_ruleX, InitializerList,  Designation, Initializer),
        GrammarRule(parse_ruleX, InitializerList,  InitializerList, Comma, Initializer),
        GrammarRule(parse_ruleX, InitializerList,  InitializerList, Comma, Designation, Initializer),

        GrammarRule(parse_ruleX, Designation,  DesignatorList, Equals),

        GrammarRule(parse_ruleX, DesignatorList,  Designator),
        GrammarRule(parse_ruleX, DesignatorList,  DesignatorList, Designator),

        GrammarRule(parse_ruleX, Designator,  LBracket, ConstantExpression, RBracket),
        GrammarRule(parse_ruleX, Designator,  Dot, Identifier),
 
        /* § A.2.3 Statements */
        GrammarRule(parse_ruleX, Statement,  LabeledStatement),
        GrammarRule(parse_Passthrough, Statement,  CompoundStatement),
        GrammarRule(parse_Passthrough, Statement,  ExpressionStatement),
        GrammarRule(parse_Passthrough, Statement,  SelectionStatement),
        GrammarRule(parse_ruleX, Statement,  IterationStatement),
        GrammarRule(parse_Passthrough, Statement,  JumpStatement),
         
        GrammarRule(parse_ruleX, LabeledStatement,  Identifier, Colon, Statement),
        GrammarRule(parse_ruleX, LabeledStatement,  CASE, ConstantExpression, Colon, Statement),
        GrammarRule(parse_ruleX, LabeledStatement,  DEFAULT, Colon, Statement),

        GrammarRule(parse_CompoundStmt, CompoundStatement,  LCurly, RCurly),
        GrammarRule(parse_CompoundStmt, CompoundStatement,  LCurly, BlockItemList, RCurly),

        GrammarRule(parse_BlockItemList, BlockItemList,  BlockItem),
        GrammarRule(parse_BlockItemList, BlockItemList,  BlockItemList, BlockItem),

        GrammarRule(parse_Passthrough, BlockItem,  Declaration),
        GrammarRule(parse_Passthrough, BlockItem,  Statement),

        GrammarRule(parse_ruleX, ExpressionStatement,  Semicolon),
        GrammarRule(parse_Passthrough, ExpressionStatement,  Expression, Semicolon),

        GrammarRule(parse_If, SelectionStatement,  IF, LParen, Expression, RParen, Statement),
        GrammarRule(parse_If, SelectionStatement,  IF, LParen, Expression, RParen, Statement, ELSE, Statement),
        GrammarRule(parse_ruleX, SelectionStatement,  SWITCH, LParen, Expression, RParen, Statement),

        GrammarRule(parse_ruleX, IterationStatement,  WHILE, LParen, Expression, RParen, Statement),
        GrammarRule(parse_ruleX, IterationStatement,  DO, Statement, WHILE, LParen, Expression, RParen, Semicolon),

        GrammarRule(parse_ruleX, IterationStatement,  FOR, Semicolon, Semicolon, RParen, Statement),
        GrammarRule(parse_ruleX, IterationStatement,  FOR, Expression, Semicolon, Semicolon, RParen, Statement),
        GrammarRule(parse_ruleX, IterationStatement,  FOR, Semicolon, Expression, Semicolon, RParen, Statement),
        GrammarRule(parse_ruleX, IterationStatement,  FOR, Expression, Semicolon, Expression, Semicolon, RParen, Statement),
        GrammarRule(parse_ruleX, IterationStatement,  FOR, Semicolon, Semicolon, Expression, RParen, Statement),
        GrammarRule(parse_ruleX, IterationStatement,  FOR, Expression, Semicolon, Semicolon, Expression, RParen, Statement),
        GrammarRule(parse_ruleX, IterationStatement,  FOR, Semicolon, Expression, Semicolon, Expression, RParen, Statement),
        GrammarRule(parse_ruleX, IterationStatement,  FOR, Expression, Semicolon, Expression, Semicolon, Expression, RParen, Statement),

        GrammarRule(parse_ruleX, JumpStatement,  GOTO, Identifier, Semicolon),
        GrammarRule(parse_JumpStmt, JumpStatement,  CONTINUE, Semicolon),
        GrammarRule(parse_JumpStmt, JumpStatement,  BREAK, Semicolon),
        GrammarRule(parse_JumpStmt, JumpStatement,  RETURN, Semicolon),
        GrammarRule(parse_JumpStmt, JumpStatement,  RETURN, Expression, Semicolon),

        /* § A.2.4 External definitions */
        GrammarRule(parse_TranslationUnit, TranslationUnit,  ExternalDeclaration),
        GrammarRule(parse_TranslationUnit, TranslationUnit,  TranslationUnit, ExternalDeclaration),

        GrammarRule(parse_ExternDecl, ExternalDeclaration,  FunctionDefinition),
        GrammarRule(parse_ExternDecl, ExternalDeclaration,  Declaration),

        GrammarRule(parse_FunctionDefn, FunctionDefinition,  DeclarationSpecifiers, Declarator, CompoundStatement),
        GrammarRule(parse_FunctionDefn, FunctionDefinition,  DeclarationSpecifiers, Declarator, DeclarationList, CompoundStatement),

        GrammarRule(parse_DeclList, DeclarationList,  Declaration),
        GrammarRule(parse_DeclList, DeclarationList,  DeclarationList, Declaration),
    };


    array(cf_production) Productions = array_new(cf_production, countof(Rules));
    Productions.Length = countof(Rules);

    array_for(cf_production, Rule, Productions) {
        *RuleRef = Rules[(RuleRef - Productions.Data)];
    }

    cf_grammar Grammar = { Productions };
    Grammar.Root = SourceFile;
    GenerateFollowTable(&Grammar);

    return Grammar;
}

