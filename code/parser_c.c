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

static void
util_PropagateFunctionReturnType(cst_node FunctionType, cst_node ReturnType)
{
    assert(CST_NODE_TYPE(FunctionType) == CST_FunctionType);
    cst_node_function_type* HigherFunc = cast(cst_node_function_type*, FunctionType);
    while (HigherFunc->ReturnType != NULL && 
           CST_NODE_TYPE(HigherFunc->ReturnType) == CST_FunctionType) 
    {
        HigherFunc = HigherFunc->ReturnType;
    }
    assert(HigherFunc->ReturnType == NULL);
    HigherFunc->ReturnType = ReturnType;
}

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


PARSE_FUNC(parse_FunctionDefn, Context, Tokens, Parsed) {
    cst_node_declaration* Base = cast(cst_node_declaration*, array_at(Parsed, 1));
    assert(CST_NODE_TYPE(Base->BaseType) == CST_FunctionType);

    cst_node_function_type* Func = cast(cst_node_function_type*, Base->BaseType);
    cst_node_declaration* DeclSpecifiers = array_at(Parsed, 0); 

    Base->SpecifierFlags = DeclSpecifiers->SpecifierFlags;

    if (Tokens.Length == 3) {
        Func->Body = cast(cst_node_block*, array_at(Parsed, 2));

        cst_node_function_type* NextFunc = Func;
        while (NextFunc->ReturnType != NULL && 
               CST_NODE_TYPE(NextFunc->ReturnType) == CST_FunctionType) 
        {
            NextFunc = NextFunc->ReturnType;
        }
        assert(NextFunc->ReturnType == NULL);
        NextFunc->ReturnType = DeclSpecifiers->BaseType;
    }
    //else if (Tokens.Length == 4) { }
    else_invalid;

    return Base;
}

PARSE_FUNC(parse_DeclList, Context, Tokens, Parsed) {
    if (Tokens.Length == 1) {
        cst_node_declaration_list Node = { CST_DeclarationList };
        Node.Declarations = array_init(cst_node, 1, array_at(Parsed, 0));
        return PushNode(Context, Node);
    }
    else if (Tokens.Length == 2) {
        cst_node_declaration_list* Node = cast(cst_node_declaration_list*, array_at(Parsed, 0)); 
        array_push(cst_node, &Node->Declarations, array_at(Parsed, 1));
        return Node;
    }
    else if (Tokens.Length == 3) {
        cst_node_declaration_list* Node = cast(cst_node_declaration_list*, array_at(Parsed, 0)); 
        array_push(cst_node, &Node->Declarations, array_at(Parsed, 2));
        return Node;
    }
    else_invalid;

    return NULL;
}

PARSE_FUNC(parse_InitDeclarator, Context, Tokens, Parsed) {
    // Base declaration bubbles up from the Declarator
    cst_node_declaration* Node = (cst_node_declaration*)array_at(Parsed, 0);
    Node->Initializer = array_at(Parsed, 2);
    return Node;
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

            if (Decl->BaseType == NULL) {
                Decl->BaseType = DeclSpecifiers->BaseType;
            }
            else if (CST_NODE_TYPE(Decl->BaseType) == CST_FunctionType) {
                util_PropagateFunctionReturnType(Decl->BaseType, DeclSpecifiers->BaseType);
            }
            else if (CST_NODE_TYPE(Decl->BaseType) == CST_ArrayType) {
                assert(!"TODO");
            }
            else_invalid;
        }
        // free(DeclSpecifiers);
        return List;
    } else if (Tokens.Length == 2) {
        // Just return whatever the DeclarationSpecifiers came up with (usually a struct/union/enum)
        return array_at(Parsed, 0);
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
    if (Tokens.Length == 2) {
        // Combine the pointer declaration with the rest of the declaration
        cst_node_declaration* Pointer = cast(cst_node_declaration*, array_at(Parsed, 0));
        cst_node_declaration* Decl = cast(cst_node_declaration*, array_at(Parsed, 1));
        Decl->PointerLevel = Pointer->PointerLevel;
        // free(Pointer);
        return Decl;
    } else if (Tokens.Length == 1) {
        cst_node_declaration* Decl = cast(cst_node_declaration*, array_at(Parsed, 0));
        return Decl;
    }
    else_invalid;
    return NULL;
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
    Node.IsUnion = (cast(uintptr_t, array_at(Parsed, 0)) == UNION);
    if (array_at(Tokens,1).Type == Identifier) {
        Node.Name = cast(cst_node_identifier*, array_at(Parsed, 1));
    }

    cst_node_declaration_list* DeclList = NULL;

    if (Tokens.Length == 4) {
        DeclList = cast(cst_node_declaration_list*, array_at(Parsed, 2));
    }
    else if (Tokens.Length == 5) {
        DeclList = cast(cst_node_declaration_list*, array_at(Parsed, 3));
    }
    else if (Tokens.Length == 2);
    else_invalid;

    if (DeclList != NULL) Node.Declarations = DeclList->Declarations;
    //free(DeclList);

    return PushNode(Context, Node);
}

PARSE_FUNC(parse_StructDecl, Context, Tokens, Parsed) {
    cst_node_declaration_list* Decls = cast(cst_node_declaration_list*, array_at(Parsed, 1));
    cst_node_declaration* Type = cast(cst_node_declaration*, array_at(Parsed, 0));

    array_for(cst_node, Node, Decls->Declarations) {
        cst_node_declaration* Decl = cast(cst_node_declaration*, Node);
        switch(CST_NODE_TYPE(Decl->BaseType)) {
            case CST_Invalid:
                Decl->BaseType = Type;
                break;
            case CST_FunctionType:
                util_PropagateFunctionReturnType(Decl->BaseType, Type);
                break;
            case CST_ArrayType:
                assert(!"TODO");
                break;
        }
        Decl->SpecifierFlags |= Type->SpecifierFlags;
    }
    // free(Type);
    return Decls;
}

PARSE_FUNC(parse_SpecifierQualifierList, Context, Tokens, Parsed) {
    cst_node_declaration* Type = NULL;
    cst_declaration_flags SpecifierFlags = 0;
    array_for(token, Token, Tokens) {
        switch(Token.Type) {
            case TypeSpecifier:
                Type = array_at(Parsed, TokenIndex);
                break;
            case SpecifierQualifierList:
                SpecifierFlags |= (cst_declaration_flags)array_at(Parsed, TokenIndex);
                break;
        }
    }

    assert(Type != NULL);
    Type->SpecifierFlags |= SpecifierFlags;

    return Type;
}

PARSE_FUNC(parse_StructDeclarator, Context, Tokens, Parsed) {
    cst_node_declaration* Decl = NULL;
    cst_node WidthExpression = NULL;
    if (Tokens.Length == 2) {
        cst_node_declaration Node = { CST_Declaration };
        Decl = PushNode(Context, Node);
        WidthExpression = array_at(Parsed, 1);
    }
    else if (Tokens.Length == 3) {
        Decl = array_at(Parsed, 0);
        WidthExpression = array_at(Parsed, 2);
    }
    else_invalid;

    // TODO compile time constant evaluation!
    Decl->BitfieldWidth = -1; //EvaluateConstantExpr(WidthExpression);
    return Decl;
}

PARSE_FUNC(parse_IdentifierDecl, Context, Tokens, Parsed) {
    cst_node_declaration Decl = { CST_Declaration };
    cst_node_identifier Iden = { CST_Identifier };
    Iden.Text = array_at(Tokens, 0).Text;
    Iden.TextLength = array_at(Tokens, 0).TextLength;
    Decl.Name = PushNode(Context, Iden);
    return PushNode(Context, Decl);
}

PARSE_FUNC(parse_ArrayDecl, Context, Tokens, Parsed) {
    cst_node_array_type Node = { CST_ArrayType };
    //TODO
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_FunctionDecl, Context, Tokens, Parsed) {
    cst_node_function_type Node = { CST_FunctionType };
    if (array_at(Tokens, 2).Type == ParameterTypeList) {
        cst_node_declaration_list* List = cast(cst_node_declaration_list*, array_at(Parsed, 2));

        Node.Arguments = List->Declarations;
        //free(array_at(Parsed, 1));
    }

    cst_node_declaration* Decl = cast(cst_node_declaration*, array_at(Parsed, 0));
    switch(CST_NODE_TYPE(Decl->BaseType)) {
        case CST_Invalid: 
            // Base case. This is a direct function declaration.
            Node.Name = Decl->Name;
            Decl->BaseType = PushNode(Context, Node);

            return Decl;

        case CST_FunctionType:
        {
            // Recursive case. The innermost-so-far DirectDeclarator returns a
            // function pointer matching the type of this declarator (this
            // declarator could be incomplete -- no return type -- so we will
            // need to bubble that through later once we've seen the whole thing
            // in FunctionDefinition)

            cst_node_identifier Name = { CST_Identifier };
            Name.Text = "Anonymous function";
            Name.TextLength = strlen("Anonymous function");
            Node.Name = cast(cst_node_identifier*, PushNode(Context, Name));

            //util_PropagateFunctionReturnType(Decl->BaseType, PushNode(Context, Node));

            cst_node_function_type* HigherFunc = cast(cst_node_function_type*, Decl->BaseType);
            while (HigherFunc->ReturnType != NULL && 
                   CST_NODE_TYPE(HigherFunc->ReturnType) == CST_FunctionType)
            {
                HigherFunc = HigherFunc->ReturnType;
            }
            // TODO temp
            assert(HigherFunc->ReturnType == NULL);
            HigherFunc->ReturnType = PushNode(Context, Node);

            return Decl;
        } break;

        //case CST_ArrayType:
        //{
        //} break;
        
        default_invalid;
    }
    return NULL;
}

PARSE_FUNC(parse_ParamDecl, Context, Tokens, Parsed) {
    cst_node_declaration* Decl = cast(cst_node_declaration*, array_at(Parsed, 1));
    cst_node_declaration* DeclSpecifiers = cast(cst_node_declaration*, array_at(Parsed, 0));
    // Combine the decl specifiers with the base decl
    Decl->SpecifierFlags = DeclSpecifiers->SpecifierFlags;
    Decl->BaseType = DeclSpecifiers->BaseType;
    // free(DeclSpecifiers);
    return Decl;
}

PARSE_FUNC(parse_VarArgParams, Context, Tokens, Parsed) {
    cst_node_declaration_list* List = cast(cst_node_declaration_list*, array_at(Parsed, 0));
    array_push(cst_node, &List->Declarations, cast(cst_node, CST_VariadicArgument));
    return List;
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

PARSE_FUNC(parse_EmptyExpr, Context, Tokens, Parsed) {
    cst_node_expression Expr = { CST_Empty };
    return PushNode(Context, Expr);
}

PARSE_FUNC(parse_BinaryExpression, Context, Tokens, Parsed) {
    cst_node_binary_operator Node = { CST_BinaryOperator };
    Node.Operation = array_at(Tokens, 1).Type;
    Node.Left = array_at(Parsed, 0);
    Node.Right = array_at(Parsed, 2);
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_UnaryPrefixExpression, Context, Tokens, Parsed) {
    cst_node_unary_operator Node = { CST_UnaryOperator };
    Node.Operation = array_at(Tokens, 0).Type;
    Node.Operand = array_at(Parsed, 1);
    Node.Affix = PREFIX;
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_UnaryPostfixExpression, Context, Tokens, Parsed) {
    cst_node_unary_operator Node = { CST_UnaryOperator };
    Node.Operation = array_at(Tokens, 1).Type;
    Node.Operand = array_at(Parsed, 0);
    Node.Affix = POSTFIX;
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_UnaryPrefixOperator, Context, Tokens, Parsed) {
    cst_node_unary_operator Node = { CST_UnaryOperator };
    Node.Operation = (uintptr_t)array_at(Parsed, 0);
    Node.Operand = array_at(Parsed, 1);
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_Sizeof, Context, Tokens, Parsed) {
    //TODO do we want to turn this into a compiler-intrinsic "constexpr" function?
    cst_node_unary_operator Node = { CST_UnaryOperator };
    if (Tokens.Length == 2) {
        Node.Operation = array_at(Tokens, 0).Type;
        Node.Operand = array_at(Parsed, 1);
    } else if (Tokens.Length == 4) {
        Node.Operation = array_at(Tokens, 0).Type;
        Node.Operand = array_at(Parsed, 2);
    }
    else_invalid;
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_If, Context, Tokens, Parsed) {
    cst_node_conditional Node = { CST_Conditional };
    Node.Condition = array_at(Parsed, 2);
    Node.TrueBranch = array_at(Parsed, 4);

    if (Tokens.Length > 5) Node.FalseBranch = array_at(Parsed, 6);
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_TernaryExpression, Context, Tokens, Parsed) {
    cst_node_conditional Node = { CST_Conditional };
    Node.Condition = array_at(Parsed, 0);
    Node.TrueBranch = array_at(Parsed, 2);
    Node.FalseBranch = array_at(Parsed, 4);

    return PushNode(Context, Node);
}

PARSE_FUNC(parse_JumpStmt, Context, Tokens, Parsed) {
    cst_node_jump Node = { CST_Jump };
    Node.Type = array_at(Tokens, 0).Type;
    if (Tokens.Length == 3) Node.Expression = array_at(Parsed, 1);
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_Cast, Context, Tokens, Parsed) {
    cst_node_cast Node = { CST_Cast };
    Node.TargetType = array_at(Parsed, 0);
    Node.Operand = array_at(Parsed, 1);
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_AssignmentExpr, Context, Tokens, Parsed) {
    cst_node_assignment Node = { CST_Assignment };

    Node.Operator = (uintptr_t)array_at(Parsed, 1);
    Node.LValue = array_at(Parsed, 0);
    Node.RValue = array_at(Parsed, 2);

    return PushNode(Context, Node);
}

PARSE_FUNC(parse_WhileStmt, Context, Tokens, Parsed) {
    cst_node_iteration Iter = { CST_Iteration };
    Iter.Condition = array_at(Parsed, 2);
    Iter.Body = array_at(Parsed, 4);
    return PushNode(Context, Iter);
}

PARSE_FUNC(parse_DoStmt, Context, Tokens, Parsed) {
    cst_node_iteration Iter = { CST_Iteration };
    Iter.PostCondition = true;
    Iter.Condition = array_at(Parsed, 4);
    Iter.Body = array_at(Parsed, 1);
    return PushNode(Context, Iter);
}

PARSE_FUNC(parse_ForStmt, Context, Tokens, Parsed) {
    cst_node_iteration Iter = { CST_Iteration };

    int Section = 0;
    array_for(token, Token, Tokens) {
        switch(Token.Type) {
            case FOR: break;
            case Semicolon: Section++; break;
            case Expression:
                if      (Section == 0) Iter.Initialization = array_at(Parsed, TokenIndex);
                else if (Section == 1) Iter.Condition      = array_at(Parsed, TokenIndex);
                else if (Section == 2) Iter.Increment      = array_at(Parsed, TokenIndex);
                else_invalid;
                break;
            case Statement: Iter.Body = array_at(Parsed, TokenIndex); break;
            default_invalid;
        }
    }
    return PushNode(Context, Iter);
}

PARSE_FUNC(parse_TODO, Context, Tokens, Parsed) {
    array_for(token, Token, Tokens) {
        printf("\tTokens[%d]: %s\n", TokenIndex, SymbolStr(Token.Type));
    }
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
        GrammarRule(parse_TODO, PostfixExpression,  PostfixExpression, LBracket, Expression, RBracket),
        GrammarRule(parse_TODO, PostfixExpression,  PostfixExpression, LParen, RParen),
        GrammarRule(parse_TODO, PostfixExpression,  PostfixExpression, LParen, ArgumentExpressionList, RParen),
        GrammarRule(parse_BinaryExpression, PostfixExpression,  PostfixExpression, Dot, Identifier),
        GrammarRule(parse_BinaryExpression, PostfixExpression,  PostfixExpression, Arrow, Identifier),
        GrammarRule(parse_UnaryPostfixExpression, PostfixExpression,  PostfixExpression, Increment),
        GrammarRule(parse_UnaryPostfixExpression, PostfixExpression,  PostfixExpression, Decrement),
        GrammarRule(parse_TODO, PostfixExpression,  LParen, TypeName, RParen, LCurly, InitializerList, RCurly),
        GrammarRule(parse_TODO, PostfixExpression,  LParen, TypeName, RParen, LCurly, InitializerList, RCurly, Comma),

        GrammarRule(parse_Passthrough, ArgumentExpressionList,  AssignmentExpression),
        GrammarRule(parse_TODO, ArgumentExpressionList,  ArgumentExpressionList, AssignmentExpression),

        GrammarRule(parse_Passthrough, UnaryExpression,  PostfixExpression),
        GrammarRule(parse_UnaryPrefixExpression, UnaryExpression,  Increment, UnaryExpression),
        GrammarRule(parse_UnaryPrefixExpression, UnaryExpression,  Decrement, UnaryExpression),
        GrammarRule(parse_UnaryPrefixOperator, UnaryExpression,  UnaryOperator, CastExpression),
        GrammarRule(parse_Sizeof, UnaryExpression,  SIZEOF, UnaryExpression),
        /* Change: surely this should be UnaryExpression, not TypeName? */
        GrammarRule(parse_Sizeof, UnaryExpression,  SIZEOF, LParen, UnaryExpression, RParen),
        
        GrammarRule(parse_PassthroughTokenType, UnaryOperator,  Ampersand),
        GrammarRule(parse_PassthroughTokenType, UnaryOperator,  Asterisk),
        GrammarRule(parse_PassthroughTokenType, UnaryOperator,  Plus),
        GrammarRule(parse_PassthroughTokenType, UnaryOperator,  Minus),
        GrammarRule(parse_PassthroughTokenType, UnaryOperator,  BitNot),
        GrammarRule(parse_PassthroughTokenType, UnaryOperator,  LogicNot),

        GrammarRule(parse_Passthrough, CastExpression,  UnaryExpression),
        GrammarRule(parse_Cast, CastExpression,  LParen, TypeName, RParen, CastExpression),

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
        GrammarRule(parse_TernaryExpression, ConditionalExpression,  LogicalORExpression, QuestionMark, Expression, Colon, ConditionalExpression),

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
        GrammarRule(parse_TODO, Expression,  Expression, Comma, AssignmentExpression),

        GrammarRule(parse_Passthrough, ConstantExpression,  ConditionalExpression),

        /* § A.2.2 Declarations */
        // NOTE(chronister, 2 aug 2017): the first rule here is to handle e.g. struct declarations
        // without a name, since struct foo { int bar; } is technically a DeclarationSpecifiers
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

        GrammarRule(parse_DeclList, InitDeclaratorList,  InitDeclarator),
        GrammarRule(parse_DeclList, InitDeclaratorList,  InitDeclaratorList, Comma, InitDeclarator),

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
        GrammarRule(parse_StructuredType, StructOrUnionSpecifier,  StructOrUnion, Identifier, LCurly, StructDeclarationList, RCurly),
        GrammarRule(parse_StructuredType, StructOrUnionSpecifier,  StructOrUnion, Identifier),

        GrammarRule(parse_PassthroughTokenType, StructOrUnion,  STRUCT),
        GrammarRule(parse_PassthroughTokenType, StructOrUnion,  UNION),

        GrammarRule(parse_DeclList, StructDeclarationList,  StructDeclaration),
        GrammarRule(parse_DeclList, StructDeclarationList,  StructDeclarationList, StructDeclaration),

        // NOTE(chronister, 2 aug 2017): StructDeclaration and SpecQualList have been modified to prevent
        // multiple TypeSpecifiers from appearing in a row. Should be semantically identical to the spec.
        // TODO: find out why this was permitted in the first place (it allows declarations like:
        //      struct foo {
        //          int double long bar;
        //      }
        // which seem completely nonsensical).
        // {
        GrammarRule(parse_Passthrough, StructDeclaration,  StructDeclaratorList, Semicolon),
        GrammarRule(parse_StructDecl, StructDeclaration,  SpecifierQualifierList, StructDeclaratorList, Semicolon),

        GrammarRule(parse_SpecifierQualifierList, SpecifierQualifierList,  TypeSpecifier),
        GrammarRule(parse_SpecifierQualifierList, SpecifierQualifierList,  TypeSpecifier, TypeQualifierList),
        GrammarRule(parse_SpecifierQualifierList, SpecifierQualifierList,  TypeQualifierList, TypeSpecifier),
        GrammarRule(parse_SpecifierQualifierList, SpecifierQualifierList,  TypeQualifierList, TypeSpecifier, TypeQualifierList),
        // }

        GrammarRule(parse_DeclList, StructDeclaratorList,  StructDeclarator),
        GrammarRule(parse_DeclList, StructDeclaratorList,  StructDeclaratorList, Comma, StructDeclarator),

        GrammarRule(parse_Passthrough, StructDeclarator,  Declarator),
        GrammarRule(parse_StructDeclarator, StructDeclarator,  Colon, ConstantExpression),
        GrammarRule(parse_StructDeclarator, StructDeclarator,  Declarator, Colon, ConstantExpression),

        GrammarRule(parse_TODO, EnumSpecifier,  ENUM, LCurly, EnumeratorList, RCurly),
        GrammarRule(parse_TODO, EnumSpecifier,  ENUM, Identifier, LCurly, EnumeratorList, RCurly),
        GrammarRule(parse_TODO, EnumSpecifier,  ENUM, LCurly, EnumeratorList, Comma, RCurly),
        GrammarRule(parse_TODO, EnumSpecifier,  ENUM, Identifier, LCurly, EnumeratorList, Comma, RCurly),
        GrammarRule(parse_Identifier, EnumSpecifier,  ENUM, Identifier),

        GrammarRule(parse_TODO, EnumeratorList,  Enumerator),
        GrammarRule(parse_TODO, EnumeratorList,  EnumeratorList, Comma, Enumerator),

        GrammarRule(parse_TODO, Enumerator,  EnumerationConstant),
        GrammarRule(parse_TODO, Enumerator,  EnumerationConstant, Equals, ConstantExpression),

        GrammarRule(parse_CONST, TypeQualifier,  CONST),
        GrammarRule(parse_RESTRICT, TypeQualifier,  RESTRICT),
        GrammarRule(parse_VOLATILE, TypeQualifier,  VOLATILE),

        GrammarRule(parse_INLINE, FunctionSpecifier,  INLINE),

        GrammarRule(parse_Declarator, Declarator,  DirectDeclarator),
        GrammarRule(parse_Declarator, Declarator,  Pointer, DirectDeclarator),

        GrammarRule(parse_IdentifierDecl, DirectDeclarator,  Identifier),
        GrammarRule(parse_PassthroughSecond, DirectDeclarator,  LParen, Declarator, RParen),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, AssignmentExpression, RBracket),

        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, AssignmentExpression, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, STATIC, AssignmentExpression, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, STATIC, TypeQualifierList, AssignmentExpression, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, STATIC, AssignmentExpression, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, Asterisk, RBracket),
        GrammarRule(parse_ArrayDecl, DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, Asterisk, RBracket),
        GrammarRule(parse_FunctionDecl, DirectDeclarator,  DirectDeclarator, LParen, RParen),
        GrammarRule(parse_FunctionDecl, DirectDeclarator,  DirectDeclarator, LParen, ParameterTypeList, RParen),
        // TODO(chronister, jul 16 17) this rule is necessary for old style
        // function declarations to be interpreted correctly (with identifiers
        // as variable names and not types), but I believe it's ambiguous with a
        // ParameterTypeList of unlabeled types (parsed by the above rule)
        //GrammarRule(parse_FunctionDecl, DirectDeclarator,  DirectDeclarator, LParen, IdentifierList, RParen),

        GrammarRule(parse_Pointer, Pointer,  Asterisk),
        GrammarRule(parse_Pointer, Pointer,  Asterisk, TypeQualifierList),
        GrammarRule(parse_Pointer, Pointer,  Asterisk, Pointer),
        GrammarRule(parse_Pointer, Pointer,  Asterisk, TypeQualifierList, Pointer),

        GrammarRule(parse_TypeQualifierList, TypeQualifierList,  TypeQualifier),
        GrammarRule(parse_TypeQualifierList, TypeQualifierList,  TypeQualifierList, TypeQualifier),

        GrammarRule(parse_Passthrough, ParameterTypeList,  ParameterList),
        GrammarRule(parse_VarArgParams, ParameterTypeList,  ParameterList, Comma, Ellipses),

        GrammarRule(parse_DeclList, ParameterList,  ParameterDeclaration),
        GrammarRule(parse_DeclList, ParameterList,  ParameterList, Comma, ParameterDeclaration),

        GrammarRule(parse_Passthrough, ParameterDeclaration,  DeclarationSpecifiers),
        GrammarRule(parse_ParamDecl, ParameterDeclaration,  DeclarationSpecifiers, Declarator),
        GrammarRule(parse_TODO, ParameterDeclaration,  DeclarationSpecifiers, AbstractDeclarator),

        // NOTE(chronister, jul 16 17): this appears to only be used in parsing
        // DirectDeclarators for function decls, and the rule that uses it seems
        // to be unnecessary (see NOTE on that rule above)
        //GrammarRule(parse_IdentifierList, IdentifierList,  Identifier),
        //GrammarRule(parse_IdentifierList, IdentifierList,  IdentifierList, Comma, Identifier),

        GrammarRule(parse_TODO, TypeName,  SpecifierQualifierList),
        GrammarRule(parse_TODO, TypeName,  SpecifierQualifierList, AbstractDeclarator),

        GrammarRule(parse_TODO, AbstractDeclarator,  Pointer),
        GrammarRule(parse_TODO, AbstractDeclarator,  DirectAbstractDeclarator),
        GrammarRule(parse_TODO, AbstractDeclarator,  Pointer, DirectAbstractDeclarator),

        GrammarRule(parse_TODO, DirectAbstractDeclarator,  LParen, AbstractDeclarator, RParen),

        GrammarRule(parse_TODO, DirectAbstractDeclarator,  LBracket, RBracket),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, RBracket),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  LBracket, TypeQualifierList, RBracket),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  LBracket, AssignmentExpression, RBracket),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, TypeQualifierList, RBracket),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, AssignmentExpression, RBracket),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  LBracket, TypeQualifierList, AssignmentExpression, RBracket),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, TypeQualifierList, AssignmentExpression, RBracket),

        GrammarRule(parse_TODO, DirectAbstractDeclarator,  LBracket, STATIC, AssignmentExpression, RBracket),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, STATIC, AssignmentExpression, RBracket),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  LBracket, STATIC, TypeQualifierList, AssignmentExpression, RBracket),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, STATIC, TypeQualifierList, AssignmentExpression, RBracket),

        GrammarRule(parse_TODO, DirectAbstractDeclarator,  LBracket, TypeQualifierList, STATIC, AssignmentExpression, RBracket),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, TypeQualifierList, STATIC, AssignmentExpression, RBracket),

        GrammarRule(parse_TODO, DirectAbstractDeclarator,  LBracket, Asterisk, RBracket),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, Asterisk, RBracket),

        GrammarRule(parse_TODO, DirectAbstractDeclarator,  LParen, RParen),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  DirectAbstractDeclarator, LParen, RParen),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  LParen, ParameterTypeList, RParen),
        GrammarRule(parse_TODO, DirectAbstractDeclarator,  DirectAbstractDeclarator, LParen, ParameterTypeList, RParen),

        // x GrammarRule(parse_TODO, TypedefName,  Identifier),

        GrammarRule(parse_Passthrough, Initializer,  AssignmentExpression),
        GrammarRule(parse_TODO, Initializer,  LCurly, InitializerList, RCurly),
        GrammarRule(parse_TODO, Initializer,  LCurly, InitializerList, Comma, RCurly),

        GrammarRule(parse_TODO, InitializerList,  Initializer),
        GrammarRule(parse_TODO, InitializerList,  Designation, Initializer),
        GrammarRule(parse_TODO, InitializerList,  InitializerList, Comma, Initializer),
        GrammarRule(parse_TODO, InitializerList,  InitializerList, Comma, Designation, Initializer),

        GrammarRule(parse_TODO, Designation,  DesignatorList, Equals),

        GrammarRule(parse_TODO, DesignatorList,  Designator),
        GrammarRule(parse_TODO, DesignatorList,  DesignatorList, Designator),

        GrammarRule(parse_TODO, Designator,  LBracket, ConstantExpression, RBracket),
        GrammarRule(parse_TODO, Designator,  Dot, Identifier),
 
        /* § A.2.3 Statements */
        GrammarRule(parse_TODO, Statement,  LabeledStatement),
        GrammarRule(parse_Passthrough, Statement,  CompoundStatement),
        GrammarRule(parse_Passthrough, Statement,  ExpressionStatement),
        GrammarRule(parse_Passthrough, Statement,  SelectionStatement),
        GrammarRule(parse_Passthrough, Statement,  IterationStatement),
        GrammarRule(parse_Passthrough, Statement,  JumpStatement),
         
        GrammarRule(parse_TODO, LabeledStatement,  Identifier, Colon, Statement),
        GrammarRule(parse_TODO, LabeledStatement,  CASE, ConstantExpression, Colon, Statement),
        GrammarRule(parse_TODO, LabeledStatement,  DEFAULT, Colon, Statement),

        GrammarRule(parse_CompoundStmt, CompoundStatement,  LCurly, RCurly),
        GrammarRule(parse_CompoundStmt, CompoundStatement,  LCurly, BlockItemList, RCurly),

        GrammarRule(parse_BlockItemList, BlockItemList,  BlockItem),
        GrammarRule(parse_BlockItemList, BlockItemList,  BlockItemList, BlockItem),

        GrammarRule(parse_Passthrough, BlockItem,  Declaration),
        GrammarRule(parse_Passthrough, BlockItem,  Statement),

        GrammarRule(parse_EmptyExpr, ExpressionStatement,  Semicolon),
        GrammarRule(parse_Passthrough, ExpressionStatement,  Expression, Semicolon),

        GrammarRule(parse_If, SelectionStatement,  IF, LParen, Expression, RParen, Statement),
        GrammarRule(parse_If, SelectionStatement,  IF, LParen, Expression, RParen, Statement, ELSE, Statement),
        GrammarRule(parse_TODO, SelectionStatement,  SWITCH, LParen, Expression, RParen, Statement),

        GrammarRule(parse_WhileStmt, IterationStatement,  WHILE, LParen, Expression, RParen, Statement),
        GrammarRule(parse_DoStmt, IterationStatement,  DO, Statement, WHILE, LParen, Expression, RParen, Semicolon),

        GrammarRule(parse_ForStmt, IterationStatement,  FOR, Semicolon, Semicolon, RParen, Statement),
        GrammarRule(parse_ForStmt, IterationStatement,  FOR, Expression, Semicolon, Semicolon, RParen, Statement),
        GrammarRule(parse_ForStmt, IterationStatement,  FOR, Semicolon, Expression, Semicolon, RParen, Statement),
        GrammarRule(parse_ForStmt, IterationStatement,  FOR, Expression, Semicolon, Expression, Semicolon, RParen, Statement),
        GrammarRule(parse_ForStmt, IterationStatement,  FOR, Semicolon, Semicolon, Expression, RParen, Statement),
        GrammarRule(parse_ForStmt, IterationStatement,  FOR, Expression, Semicolon, Semicolon, Expression, RParen, Statement),
        GrammarRule(parse_ForStmt, IterationStatement,  FOR, Semicolon, Expression, Semicolon, Expression, RParen, Statement),
        GrammarRule(parse_ForStmt, IterationStatement,  FOR, Expression, Semicolon, Expression, Semicolon, Expression, RParen, Statement),

        GrammarRule(parse_TODO, JumpStatement,  GOTO, Identifier, Semicolon),
        GrammarRule(parse_JumpStmt, JumpStatement,  CONTINUE, Semicolon),
        GrammarRule(parse_JumpStmt, JumpStatement,  BREAK, Semicolon),
        GrammarRule(parse_JumpStmt, JumpStatement,  RETURN, Semicolon),
        GrammarRule(parse_JumpStmt, JumpStatement,  RETURN, Expression, Semicolon),

        /* § A.2.4 External definitions */
        GrammarRule(parse_TranslationUnit, TranslationUnit,  ExternalDeclaration),
        GrammarRule(parse_TranslationUnit, TranslationUnit,  TranslationUnit, ExternalDeclaration),

        GrammarRule(parse_Passthrough, ExternalDeclaration,  FunctionDefinition),
        GrammarRule(parse_Passthrough, ExternalDeclaration,  Declaration),

        GrammarRule(parse_FunctionDefn, FunctionDefinition,  DeclarationSpecifiers, Declarator, CompoundStatement),
        // NOTE(chronister, jul 7 17): omitting the old-style definitions as
        // they are very rare in modern code
        //GrammarRule(parse_FunctionDefn, FunctionDefinition,  DeclarationSpecifiers, Declarator, DeclarationList, CompoundStatement),

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

