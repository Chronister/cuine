#ifndef GRAMMAR_C_H
#define GRAMMAR_C_H

/// C-specific symbols

#define C_TERMINAL_LIST(X)  \
    X(EndOfFile),           \
    X(IntegerConstant),     \
    X(FloatingConstant),    \
    X(CharacterConstant),   \
                            \
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
    X(Constant),                    \
                                    \
    /* § A.2.1 Expressions */       \
    X(PrimaryExpression),           \
    X(PostfixExpression),           \
    X(ArgumentExpressionList),      \
    X(UnaryExpression),             \
    X(UnaryOperator),               \
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
    X(DeclarationQualifiers),       \
    X(InitDeclaratorList),          \
    X(InitDeclarator),              \
    X(StorageClassSpecifier),       \
    X(TypeSpecifier),               \
    X(MultiType),                   \
    X(MultiTypeList),               \
    X(StructOrUnionSpecifier),      \
    X(StructOrUnion),               \
    X(StructDeclarationList),       \
    X(StructDeclaration),           \
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
                                    \
    X(SourceFile),                  \

#define _(x) x
#define STR(x) #x

typedef enum c_terminal_symbol {
    C_TerminalMIN = -1,
    C_TERMINAL_LIST(_)
    C_TerminalMAX,
} c_terminal_symbol;

typedef enum c_nonterminal_symbol {
    C_NonterminalMIN = C_TerminalMAX - 1,
    C_NONTERMINAL_LIST(_)
    C_NonterminalMAX,
} c_nonterminal_symbol;

const char* C_SymbolNames[] = {
    C_TERMINAL_LIST(STR)
    C_NONTERMINAL_LIST(STR)

    [AUTO] = "auto",
    [BREAK] = "break",
    [CASE] = "case",
    [CHAR] = "char",
    [CONST] = "const",
    [CONTINUE] = "continue",
    [DEFAULT] = "default",
    [DO] = "do",
    [DOUBLE] = "double",
    [ELSE] = "else",
    [ENUM] = "enum",
    [EXTERN] = "extern",
    [FLOAT] = "float",
    [FOR] = "for",
    [GOTO] = "goto",
    [IF] = "if",
    [INLINE] = "inline",
    [INT] = "int",
    [LONG] = "long",
    [REGISTER] = "register",
    [RESTRICT] = "restrict",
    [RETURN] = "return",
    [SHORT] = "short",
    [SIGNED] = "signed",
    [SIZEOF] = "sizeof",
    [STATIC] = "static",
    [STRUCT] = "struct",
    [SWITCH] = "switch",
    [TYPEDEF] = "typedef",
    [UNION] = "union",
    [UNSIGNED] = "unsigned",
    [VOID] = "void",
    [VOLATILE] = "volatile",
    [WHILE] = "while",
    [_BOOL] = "_Bool",
    [_COMPLEX] = "_Complex",
    [_IMAGINARY] = "_Imaginary",
};

#endif
