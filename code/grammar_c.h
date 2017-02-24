#ifndef GRAMMAR_C_H
#define GRAMMAR_C_H

/// C-specific symbols

#define C_TERMINAL_LIST(X)  \
    X(EndOfFile),           \
    X(CharLiteral),         \
    X(IntLiteral),          \
    X(FloatLiteral),        \
    X(StringLiteral),       \
                            \
    X(Identifier),          \
    X(Keyword),             \
                            \
    X(LParen),              \
    X(RParen),              \
    X(LBracket),            \
    X(RBracket),            \
    X(LCurly),              \
    X(RCurly),              \
                            \
    X(Comma),               \
    X(Dot),                 \
    X(Ellipses),            \
    X(Arrow),               \
    X(Semicolon),           \
    X(Colon),               \
    X(QuestionMark),        \
                            \
    X(Plus),                \
    X(PlusEquals),          \
    X(Increment),           \
    X(Minus),               \
    X(MinusEquals),         \
    X(Decrement),           \
    X(Asterisk),            \
    X(TimesEquals),         \
    X(Modulus),             \
    X(ModulusEquals),       \
    X(Divide),              \
    X(DivideEquals),        \
                            \
    X(LogicNot),            \
    X(LogicNotEquals),      \
    X(LogicAnd),            \
    X(LogicOr),             \
                            \
    X(Less),                \
    X(LessEquals),          \
    X(Assign),              \
    X(Equals),              \
    X(Greater),             \
    X(GreaterEquals),       \
                            \
    X(LBitShift),           \
    X(LBitShiftEquals),     \
    X(RBitShift),           \
    X(RBitShiftEquals),     \
                            \
    X(Ampersand),           \
    X(BitAndEquals),        \
    X(BitOr),               \
    X(BitOrEquals),         \
    X(BitXor),              \
    X(BitXorEquals),        \
    X(BitNot),              \
    X(BitNotEquals),        \
                        \
    X(AUTO),            \
    X(BREAK),           \
    X(CASE),            \
    X(CHAR),            \
    X(CONST),           \
    X(CONTINUE),        \
    X(DEFAULT),         \
    X(DO),              \
    X(DOUBLE),          \
    X(ELSE),            \
    X(ENUM),            \
    X(EXTERN),          \
    X(FLOAT),           \
    X(FOR),             \
    X(GOTO),            \
    X(IF),              \
    X(INLINE),          \
    X(INT),             \
    X(LONG),            \
    X(REGISTER),        \
    X(RESTRICT),        \
    X(RETURN),          \
    X(SHORT),           \
    X(SIGNED),          \
    X(SIZEOF),          \
    X(STATIC),          \
    X(STRUCT),          \
    X(SWITCH),          \
    X(TYPEDEF),         \
    X(UNION),           \
    X(UNSIGNED),        \
    X(VOID),            \
    X(VOLATILE),        \
    X(WHILE),           \
    X(_BOOL),           \
    X(_COMPLEX),        \
    X(_IMAGINARY),      

#define C_NONTERMINAL_LIST(X)       \
    /* § A.2.1 Expressions */       \
    X(PrimaryExpression),           \
    X(PostfixExpression),           \
    X(ArgumentExpressionList),      \
    X(UnaryExpression),             \
    X(CastExpression),              \
    X(MultiplicativeExpression),    \
    X(AdditiveExpression),          \
    X(ShiftExpression),             \
    X(RelationalExpression),        \
    X(EqualityExpression),          \
    X(ANDExpression),               \
    X(ExclusiveORExpression),       \
    X(InclusiveORExpression),       \
    X(LogicalANDExpression),        \
    X(LogicalORExpression),         \
    X(ConditionalExpression),       \
    X(AssignmentExpression),        \
    X(AssignmentOperator),          \
    X(ConstantExpression),          \
    X(Expression),                  \
                                    \
    /* § A.2.2 Declarations */      \
    X(Declaration),                 \
    X(DeclarationSpecifiers),       \
    X(InitDeclaratorList),          \
    X(InitDeclarator),              \
    X(StorageClassSpecifier),       \
    X(TypeSpecifier),               \
    X(StructOrUnionSpecifier),      \
    X(StructOrUnion),               \
    X(StructDeclarationList),       \
    X(SpecifierQualifierList),      \
    X(StructDeclaratorList),        \
    X(StructDeclarator),            \
    X(EnumSpecifier),               \
    X(EnumeratorList),              \
    X(Enumerator),                  \
    X(EnumerationConstant),         \
    X(TypeQualifier),               \
    X(FunctionSpecifier),           \
    X(Declarator),                  \
    X(DirectDeclarator),            \
    X(Pointer),                     \
    X(TypeQualifierList),           \
    X(ParameterTypeList),           \
    X(ParameterList),               \
    X(ParameterDeclaration),        \
    X(IdentifierList),              \
    X(TypeName),                    \
    X(AbstractDeclarator),          \
    X(DirectAbstractDeclarator),    \
    X(TypedefName),                 \
    X(Initializer),                 \
    X(InitializerList),             \
    X(Designation),                 \
    X(DesignatorList),              \
    X(Designator),                  \
                                    \
    /* § A.2.3 Statements */        \
    X(Statement),                   \
    X(LabeledStatement),            \
    X(CompoundStatement),           \
    X(BlockItemList),               \
    X(BlockItem),                   \
    X(ExpressionStatement),         \
    X(SelectionStatement),          \
    X(IterationStatement),          \
    X(JumpStatement),               \
                                    \
    /* § A.2.4 External defns */    \
    X(TranslationUnit),             \
    X(ExternalDeclaration),         \
    X(FunctionDefinition),          \
    X(DeclarationList),             \

#define _(x) x
#define STR(x) #x

typedef enum {
    TerminalMIN = 0,
    C_TERMINAL_LIST(_)
    TerminalMAX,
} c_terminal_symbol;

typedef enum {
    NonterminalMIN = TerminalMAX - 1,
    C_NONTERMINAL_LIST(_)
    NonterminalMAX,
} c_nonterminal_symbol;

const char* C_SymbolNames[] = {
    C_TERMINAL_LIST(STR)
    C_NONTERMINAL_LIST(STR)
};

#endif
