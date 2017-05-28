#include "grammar_c.h"
#include "parser_c.h"

extern const char*
SymbolStr(cf_symbol_t Sym)
{
    if (Sym > C_TerminalMIN && Sym < C_NonterminalMAX) {
        return C_SymbolNames[Sym];
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
    cst_node_function_definition Node = { CST_FunctionDefinition };
    //TODO
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_DeclList, Context, Tokens, Parsed) {
    cst_node_declaration_list Node = { CST_DeclarationList };
    //TODO
    return PushNode(Context, Node);
}

PARSE_FUNC(parse_Declaration, Context, Tokens, Parsed) {
    cst_node_declaration* Node = (cst_node_declaration*)array_at(Parsed, 0); // The declaration bubbles up from the specifiers

    if (Tokens.Length == 3) {
        //TODO
    }
    return Node;
}

PARSE_FUNC(parse_DeclSpecifiers, Context, Tokens, Parsed) {
    cst_node_declaration Node = { CST_Declaration };

    array_for(token, Token, Tokens) {
        if (Token.Type == TypeSpecifier) {
            Node.Type = (cst_node_type_specifier*)array_at(Parsed, TokenIndex);
        }
        else {
            assert(Token.Type == DeclarationQualifiers);
            Node.SpecifierFlags |= (intptr_t)array_at(Parsed, TokenIndex);
        }
    }

    return PushNode(Context, Node);
}

PARSE_FUNC(parse_DeclQualifiers, Context, Tokens, Parsed) {
    intptr_t Result = (intptr_t)array_at(Parsed, 0);
    if (Tokens.Length > 1) Result |= (intptr_t)array_at(Parsed, 1);
    return (void*)Result;
}

PARSE_FUNC(parse_TYPEDEF,  Context, Tokens, Parsed) { return (void*)(intptr_t)DECL_TYPEDEF; }
PARSE_FUNC(parse_EXTERN,   Context, Tokens, Parsed) { return (void*)(intptr_t)DECL_EXTERN; }
PARSE_FUNC(parse_STATIC,   Context, Tokens, Parsed) { return (void*)(intptr_t)DECL_STATIC; }
PARSE_FUNC(parse_AUTO,     Context, Tokens, Parsed) { return (void*)(intptr_t)DECL_AUTO; }
PARSE_FUNC(parse_REGISTER, Context, Tokens, Parsed) { return (void*)(intptr_t)DECL_REGISTER; }
PARSE_FUNC(parse_CONST,    Context, Tokens, Parsed) { return (void*)(intptr_t)DECL_CONST; }
PARSE_FUNC(parse_RESTRICT, Context, Tokens, Parsed) { return (void*)(intptr_t)DECL_RESTRICT; }
PARSE_FUNC(parse_VOLATILE, Context, Tokens, Parsed) { return (void*)(intptr_t)DECL_VOLATILE; }
PARSE_FUNC(parse_INLINE,   Context, Tokens, Parsed) { return (void*)(intptr_t)DECL_INLINE; }

PARSE_FUNC(parse_TypeSpecifier, Context, Tokens, Parsed) {
    cst_node_function_definition Node = { CST_FunctionDefinition };
    //TODO
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
        GrammarRule(parse_ruleX, Constant,  IntegerConstant),
        GrammarRule(parse_ruleX, Constant,  FloatingConstant),
        // x GrammarRule(parse_ruleX, Constant,  EnumerationConstant),
        GrammarRule(parse_ruleX, Constant,  CharacterConstant),

        // x GrammarRule(parse_ruleX, EnumerationConstant,  Identifier),

        /* § A.2.1 Expressions */
        GrammarRule(parse_ruleX, PrimaryExpression,  Identifier),
        GrammarRule(parse_ruleX, PrimaryExpression,  Constant),
        GrammarRule(parse_ruleX, PrimaryExpression,  StringLiteral),
        GrammarRule(parse_ruleX, PrimaryExpression,  LParen, Expression, RParen),

        GrammarRule(parse_ruleX, PostfixExpression,  PrimaryExpression),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, LBracket, Expression, RBracket),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, LParen, RParen),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, LParen, ArgumentExpressionList, RParen),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, Dot, Identifier),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, Arrow, Identifier),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, Increment),
        GrammarRule(parse_ruleX, PostfixExpression,  PostfixExpression, Decrement),
        GrammarRule(parse_ruleX, PostfixExpression,  LParen, TypeName, RParen, LCurly, InitializerList, RCurly),
        GrammarRule(parse_ruleX, PostfixExpression,  LParen, TypeName, RParen, LCurly, InitializerList, RCurly, Comma),

        GrammarRule(parse_ruleX, ArgumentExpressionList,  AssignmentExpression),
        GrammarRule(parse_ruleX, ArgumentExpressionList,  ArgumentExpressionList, AssignmentExpression),

        GrammarRule(parse_ruleX, UnaryExpression,  PostfixExpression),
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

        GrammarRule(parse_ruleX, CastExpression,  UnaryExpression),
        GrammarRule(parse_ruleX, CastExpression,  LParen, TypeName, RParen, CastExpression),

        GrammarRule(parse_ruleX, MultiplicativeExpression,  CastExpression),
        GrammarRule(parse_ruleX, MultiplicativeExpression,  MultiplicativeExpression, Asterisk, CastExpression),
        GrammarRule(parse_ruleX, MultiplicativeExpression,  MultiplicativeExpression, Divide, CastExpression),
        GrammarRule(parse_ruleX, MultiplicativeExpression,  MultiplicativeExpression, Modulus, CastExpression),

        GrammarRule(parse_ruleX, AdditiveExpression,  MultiplicativeExpression),
        GrammarRule(parse_ruleX, AdditiveExpression,  AdditiveExpression, Plus, MultiplicativeExpression),
        GrammarRule(parse_ruleX, AdditiveExpression,  AdditiveExpression, Minus, MultiplicativeExpression),

        GrammarRule(parse_ruleX, ShiftExpression,  AdditiveExpression),
        GrammarRule(parse_ruleX, ShiftExpression,  ShiftExpression, LBitShift, AdditiveExpression),
        GrammarRule(parse_ruleX, ShiftExpression,  ShiftExpression, RBitShift, AdditiveExpression),

        GrammarRule(parse_ruleX, RelationalExpression,  ShiftExpression),
        GrammarRule(parse_ruleX, RelationalExpression,  RelationalExpression, Less, ShiftExpression),
        GrammarRule(parse_ruleX, RelationalExpression,  RelationalExpression, Greater, ShiftExpression),
        GrammarRule(parse_ruleX, RelationalExpression,  RelationalExpression, LessEquals, ShiftExpression),
        GrammarRule(parse_ruleX, RelationalExpression,  RelationalExpression, GreaterEquals, ShiftExpression),

        GrammarRule(parse_ruleX, EqualityExpression,  RelationalExpression),
        GrammarRule(parse_ruleX, EqualityExpression,  EqualityExpression, Equals, RelationalExpression),
        GrammarRule(parse_ruleX, EqualityExpression,  EqualityExpression, LogicNotEquals, RelationalExpression),

        GrammarRule(parse_ruleX, ANDExpression,  EqualityExpression),
        GrammarRule(parse_ruleX, ANDExpression,  ANDExpression, Ampersand, EqualityExpression),

        GrammarRule(parse_ruleX, ExclusiveORExpression,  ANDExpression),
        GrammarRule(parse_ruleX, ExclusiveORExpression,  ExclusiveORExpression, BitXor, ANDExpression),

        GrammarRule(parse_ruleX, InclusiveORExpression, ExclusiveORExpression),
        GrammarRule(parse_ruleX, InclusiveORExpression, InclusiveORExpression, BitOr, ANDExpression),

        GrammarRule(parse_ruleX, LogicalANDExpression,  InclusiveORExpression),
        GrammarRule(parse_ruleX, LogicalANDExpression,  LogicalANDExpression, LogicAnd, InclusiveORExpression),

        GrammarRule(parse_ruleX, LogicalORExpression,  LogicalANDExpression),
        GrammarRule(parse_ruleX, LogicalORExpression,  LogicalORExpression, LogicOr, LogicalANDExpression),

        GrammarRule(parse_ruleX, ConditionalExpression,  LogicalORExpression),
        GrammarRule(parse_ruleX, ConditionalExpression,  LogicalORExpression, QuestionMark, Expression, Colon, ConditionalExpression),

        GrammarRule(parse_ruleX, AssignmentExpression,  ConditionalExpression),
        GrammarRule(parse_ruleX, AssignmentExpression,  UnaryExpression, AssignmentOperator, AssignmentExpression),

        GrammarRule(parse_ruleX, AssignmentOperator,  Assign),
        GrammarRule(parse_ruleX, AssignmentOperator,  TimesEquals),
        GrammarRule(parse_ruleX, AssignmentOperator,  DivideEquals),
        GrammarRule(parse_ruleX, AssignmentOperator,  ModulusEquals),
        GrammarRule(parse_ruleX, AssignmentOperator,  PlusEquals),
        GrammarRule(parse_ruleX, AssignmentOperator,  MinusEquals),
        GrammarRule(parse_ruleX, AssignmentOperator,  LBitShiftEquals),
        GrammarRule(parse_ruleX, AssignmentOperator,  RBitShiftEquals),
        GrammarRule(parse_ruleX, AssignmentOperator,  BitAndEquals),
        GrammarRule(parse_ruleX, AssignmentOperator,  BitXorEquals),
        GrammarRule(parse_ruleX, AssignmentOperator,  BitOrEquals),

        GrammarRule(parse_ruleX, Expression,  AssignmentExpression),
        GrammarRule(parse_ruleX, Expression,  Expression, Comma, AssignmentExpression),

        GrammarRule(parse_ruleX, ConstantExpression,  ConditionalExpression),

        /* § A.2.2 Declarations */
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

        GrammarRule(parse_ruleX, InitDeclaratorList,  InitDeclarator),
        GrammarRule(parse_ruleX, InitDeclaratorList,  InitDeclaratorList, Comma, InitDeclarator),

        GrammarRule(parse_ruleX, InitDeclarator,  Declarator),
        GrammarRule(parse_ruleX, InitDeclarator,  Declarator, Assign, Initializer),

        GrammarRule(parse_TYPEDEF, StorageClassSpecifier,  TYPEDEF),
        GrammarRule(parse_EXTERN, StorageClassSpecifier,  EXTERN),
        GrammarRule(parse_STATIC, StorageClassSpecifier,  STATIC),
        GrammarRule(parse_AUTO, StorageClassSpecifier,  AUTO),
        GrammarRule(parse_REGISTER, StorageClassSpecifier,  REGISTER),

        GrammarRule(parse_TypeSpecifier, TypeSpecifier,  VOID),
        GrammarRule(parse_TypeSpecifier, TypeSpecifier,  _BOOL),
        GrammarRule(parse_TypeSpecifier, TypeSpecifier,  _COMPLEX),
        GrammarRule(parse_TypeSpecifier, TypeSpecifier,  MultiTypeList),
        GrammarRule(parse_TypeSpecifier, TypeSpecifier,  StructOrUnionSpecifier),
        GrammarRule(parse_TypeSpecifier, TypeSpecifier,  EnumSpecifier),
        GrammarRule(parse_TypeSpecifier, TypeSpecifier,  Identifier),

        // Added to the grammar to account for type names like "unsigned long long Foo"
        // being valid, but type names like "foo bar Foo" being probably invalid
        // and causing problems with parsing
        // {
        GrammarRule(parse_ruleX, MultiTypeList,  MultiType),
        GrammarRule(parse_ruleX, MultiTypeList,  MultiTypeList, MultiType),

        GrammarRule(parse_ruleX, MultiType,  CHAR),
        GrammarRule(parse_ruleX, MultiType,  SHORT),
        GrammarRule(parse_ruleX, MultiType,  INT),
        GrammarRule(parse_ruleX, MultiType,  LONG),
        GrammarRule(parse_ruleX, MultiType,  FLOAT),
        GrammarRule(parse_ruleX, MultiType,  DOUBLE),
        GrammarRule(parse_ruleX, MultiType,  SIGNED),
        GrammarRule(parse_ruleX, MultiType,  UNSIGNED),
        // }

        GrammarRule(parse_ruleX, StructOrUnionSpecifier,  StructOrUnion, LCurly, StructDeclarationList, RCurly),
        GrammarRule(parse_ruleX, StructOrUnionSpecifier,  StructOrUnion, Identifier, LBracket, StructDeclarationList, RBracket),
        GrammarRule(parse_ruleX, StructOrUnionSpecifier,  StructOrUnion, Identifier),

        GrammarRule(parse_ruleX, StructOrUnion,  STRUCT),
        GrammarRule(parse_ruleX, StructOrUnion,  UNION),

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
        GrammarRule(parse_ruleX, EnumSpecifier,  ENUM, Identifier),

        GrammarRule(parse_ruleX, EnumeratorList,  Enumerator),
        GrammarRule(parse_ruleX, EnumeratorList,  EnumeratorList, Comma, Enumerator),

        GrammarRule(parse_ruleX, Enumerator,  EnumerationConstant),
        GrammarRule(parse_ruleX, Enumerator,  EnumerationConstant, Equals, ConstantExpression),

        GrammarRule(parse_CONST, TypeQualifier,  CONST),
        GrammarRule(parse_RESTRICT, TypeQualifier,  RESTRICT),
        GrammarRule(parse_VOLATILE, TypeQualifier,  VOLATILE),

        GrammarRule(parse_INLINE, FunctionSpecifier,  INLINE),

        GrammarRule(parse_ruleX, Declarator,  DirectDeclarator),
        GrammarRule(parse_ruleX, Declarator,  Pointer, DirectDeclarator),

        GrammarRule(parse_ruleX, DirectDeclarator,  Identifier),
        GrammarRule(parse_ruleX, DirectDeclarator,  LParen, Declarator, RParen),
        GrammarRule(parse_ruleX, DirectDeclarator,  DirectDeclarator, LBracket, RBracket),
        GrammarRule(parse_ruleX, DirectDeclarator,  DirectDeclarator, LBracket, AssignmentExpression, RBracket),
        GrammarRule(parse_ruleX, DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, RBracket),
        GrammarRule(parse_ruleX, DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, AssignmentExpression, RBracket),
        GrammarRule(parse_ruleX, DirectDeclarator,  DirectDeclarator, LBracket, STATIC, AssignmentExpression, RBracket),
        GrammarRule(parse_ruleX, DirectDeclarator,  DirectDeclarator, LBracket, STATIC, TypeQualifierList, AssignmentExpression, RBracket),
        GrammarRule(parse_ruleX, DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, STATIC, AssignmentExpression, RBracket),
        GrammarRule(parse_ruleX, DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, Asterisk, RBracket),
        GrammarRule(parse_ruleX, DirectDeclarator,  DirectDeclarator, LParen, ParameterTypeList, RParen),
        GrammarRule(parse_ruleX, DirectDeclarator,  DirectDeclarator, LParen, RParen),
        GrammarRule(parse_ruleX, DirectDeclarator,  DirectDeclarator, LParen, IdentifierList, RParen),

        GrammarRule(parse_ruleX, Pointer,  Asterisk),
        GrammarRule(parse_ruleX, Pointer,  Asterisk, TypeQualifierList),
        GrammarRule(parse_ruleX, Pointer,  Asterisk, Pointer),
        GrammarRule(parse_ruleX, Pointer,  Asterisk, TypeQualifierList, Pointer),

        GrammarRule(parse_ruleX, TypeQualifierList,  TypeQualifier),
        GrammarRule(parse_ruleX, TypeQualifierList,  TypeQualifierList, TypeQualifier),

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

        GrammarRule(parse_ruleX, Initializer,  AssignmentExpression),
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
        GrammarRule(parse_ruleX, Statement,  CompoundStatement),
        GrammarRule(parse_ruleX, Statement,  ExpressionStatement),
        GrammarRule(parse_ruleX, Statement,  SelectionStatement),
        GrammarRule(parse_ruleX, Statement,  IterationStatement),
        GrammarRule(parse_ruleX, Statement,  JumpStatement),
         
        GrammarRule(parse_ruleX, LabeledStatement,  Identifier, Colon, Statement),
        GrammarRule(parse_ruleX, LabeledStatement,  CASE, ConstantExpression, Colon, Statement),
        GrammarRule(parse_ruleX, LabeledStatement,  DEFAULT, Colon, Statement),

        GrammarRule(parse_ruleX, CompoundStatement,  LCurly, RCurly),
        GrammarRule(parse_ruleX, CompoundStatement,  LCurly, BlockItemList, RCurly),

        GrammarRule(parse_ruleX, BlockItemList,  BlockItem),
        GrammarRule(parse_ruleX, BlockItemList,  BlockItemList, BlockItem),

        GrammarRule(parse_ruleX, BlockItem,  Declaration),
        GrammarRule(parse_ruleX, BlockItem,  Statement),

        GrammarRule(parse_ruleX, ExpressionStatement,  Semicolon),
        GrammarRule(parse_ruleX, ExpressionStatement,  Expression, Semicolon),

        GrammarRule(parse_ruleX, SelectionStatement,  IF, LParen, Expression, RParen, Statement),
        GrammarRule(parse_ruleX, SelectionStatement,  IF, LParen, Expression, RParen, Statement, ELSE, Statement),
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
        GrammarRule(parse_ruleX, JumpStatement,  CONTINUE, Semicolon),
        GrammarRule(parse_ruleX, JumpStatement,  BREAK, Semicolon),
        GrammarRule(parse_ruleX, JumpStatement,  RETURN, Semicolon),
        GrammarRule(parse_ruleX, JumpStatement,  RETURN, Expression, Semicolon),

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

