
extern const char*
SymbolStr(cf_symbol_t Sym)
{
    if (Sym > L_TerminalMIN && Sym < L_NonterminalMAX) {
        return L_SymbolNames[Sym];
    }
    return "???";
}

static token 
Process(token Token) {
    switch(Token.Type) {
        case Comma: Token.Type = L_Comma; break; 
        case LParen: Token.Type = L_LParen; break;
        case RParen: Token.Type = L_RParen; break;
        case Identifier: Token.Type = L_Identifier; break;
        case EndOfFile: Token.Type = L_EOF; break;
    }
    return Token;
}

PARSE_FUNC(parse_rule0, Context, Tokens, Parsed) {
    return Parsed[0];
}

PARSE_FUNC(parse_rule1, Context, Tokens, Parsed) {
    // Tokens = L_LParen, L_L, L_RParen
    return Parsed[1];
};

PARSE_FUNC(parse_rule2, Context, Tokens, Parsed) {
    // Tokens = Identifier
    lst_node_identifier Result = { LST_Identifier };
    Result.Header.SourceLine = Tokens[0].LineStart;

    Result.Value = Tokens[0].Text;
    Result.ValueLength = Tokens[0].TextLength;
    return PushNode(Context, Result);
};

PARSE_FUNC(parse_rule3, Context, Tokens, Parsed) {
    // Tokens = L_S 
    lst_node_list Value = { LST_List };
    Value.Header.SourceLine = Tokens[0].LineStart;

    lst_node_list* Result = (lst_node_list*)PushNode(Context, Value);
    Result->Data = Parsed[0];
    Result->First = Result;
    return (lst_node*)Result;
}

#define TEST_GRAMMAR 2

#if TEST_GRAMMAR == 1
PARSE_FUNC(parse_rule4, Context, Tokens, Parsed) {
    // Tokens = L_L, L_S 
    lst_node_list Value = { LST_List };
    Value.Header.SourceLine = Tokens[0].LineStart;

    lst_node_list* Result = (lst_node_list*)PushNode(Context, Value);
    lst_node_list* Prev = (lst_node_list*)Parsed[0];
    Result->Data = Parsed[2];
    Result->First = Prev->First;
    Prev->Next = Result;
    return (lst_node*)Result;
}
#elif TEST_GRAMMAR == 2
PARSE_FUNC(parse_rule4, Context, Tokens, Parsed) {
    // Tokens = L_S, L_L 
    lst_node_list Value = { LST_List };
    Value.Header.SourceLine = Tokens[0].LineStart;

    lst_node_list* Result = (lst_node_list*)PushNode(Context, Value);
    lst_node_list* Prev = (lst_node_list*)Parsed[2];
    Result->Data = Parsed[0];
    Result->First = Result;
    Result->Next = Prev;
    return (lst_node*)Result;
}
#endif

// Here's the test grammar.
// In BNF notation, this would look like:
//
//   S' ::= S EOF
//   S  ::= (L) | IDENTIFIER
//   L  ::= S
//   L  ::= L,S
//
// Essentially, correctly nested comma-separated lists of identifiers.
// So LISP, basically (except with commas!)

cf_grammar GenerateGrammar()
{
    cf_production Rules[] = {
        GrammarRule(parse_rule0, L_Sp, L_S, L_EOF),
        GrammarRule(parse_rule1, L_S, L_LParen, L_L, L_RParen),
        GrammarRule(parse_rule2, L_S, L_Identifier),
        GrammarRule(parse_rule3, L_L, L_S),
#if TEST_GRAMMAR == 1
        GrammarRule(parse_rule4, L_L, L_L, L_Comma, L_S),
#elif TEST_GRAMMAR == 2
        GrammarRule(parse_rule4, L_L, L_S, L_Comma, L_L),
#endif
    };

    array(cf_production) Productions = array_new(cf_production, countof(Rules));
    Productions.Length = countof(Rules);

    array_for(cf_production, Rule, Productions) {
        *RuleRef = Rules[(RuleRef - Productions.Data)];
    }

    cf_grammar Grammar = { Productions };
    Grammar.Root = L_Sp;
    GenerateFollowTable(&Grammar);

    return Grammar;
}
