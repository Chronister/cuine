#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "scanner.h"
#include "parser.h"

#define countof(x) ((sizeof((x)))/sizeof((x)[0]))

#define GrammarRule(Nonterminal, ...) _GrammarRule(Nonterminal, ##__VA_ARGS__, L_TerminalMIN)
#define GETRULE(Grammar, N) Grammar.Productions.Data[N]

cf_production _GrammarRule(parse_func Func, cf_symbol_t Nonterminal, ...)
{
    cf_production Result = { .Nonterminal = Nonterminal, .Func = Func };
    Result.Sequence = array_new(cf_symbol_t, CF_MAX_SYMBOLS_PER_RULE);

    va_list Args;
    va_start(Args, Nonterminal);

    cf_symbol_t Next = va_arg(Args, cf_symbol_t);
    do {
        array_push(cf_symbol_t, &Result.Sequence, Next);
        Next = va_arg(Args, cf_symbol_t);
    } while (Next != L_TerminalMIN && Result.Sequence.Length < CF_MAX_SYMBOLS_PER_RULE);

    return Result;
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

cf_grammar TestGrammar()
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

    return Grammar;
}

size_t MakeState(cf_grammar Grammar, array(lr_fsa_state)* States, ptrdiff_t PrevStateIndex, lr_item FromItem);

void ItemClosure(cf_grammar Grammar, lr_fsa_state* State)
{
    // We need to bring in the closure of all of the items that can be
    // started from existing items. E.g., if you have a rule:
    //      A ::= xBy
    // And your current item is
    //      A ::= x . By
    // You also need to bring in new items for all of the productions for B

    // However, this needs to keep being done until the transitive
    // closure has been achieved. For example, B could have an
    // expansion:
    //      B ::= Czw
    // and in this case, adding the following item for B:
    //      B ::= . Czw
    // means we also need to add in items for all the productions of C!

    size_t MaxVisitedItemIndex = 0;
    size_t NumItems;
    do {
        NumItems = State->Items.Length;
        // The item array will be changing as we go through this, so be sure to
        // use the saved value in this iteration
        for (size_t ItemIndex = MaxVisitedItemIndex; ItemIndex < NumItems; ++ItemIndex) 
        {
            lr_item Item = State->Items.Data[ItemIndex];
            cf_production Rule = GETRULE(Grammar, Item.RuleNum);
            if (Item.Position == Rule.Sequence.Length) { continue; }
            cf_symbol_t Symbol = Rule.Sequence.Data[Item.Position];
            if (L_NonterminalMIN < Symbol && Symbol < L_NonterminalMAX) {
                array_for(cf_production, OtherRule, Grammar.Productions) {
                    if (OtherRule.Nonterminal == Symbol) {
                        lr_item NewItem = (lr_item){ 
                            .RuleNum = OtherRuleRef - Grammar.Productions.Data, 
                            .Position = 0 
                        };
                        array_for(lr_item, Existing, State->Items) {
                            if (memcmp(&Existing, &NewItem, sizeof(lr_item)) == 0) {
                                goto reject;
                            }
                        }
accept:
                        array_push(lr_item, &State->Items, NewItem);
reject:
                        continue;
                    }
                }
            }
        }
        MaxVisitedItemIndex = NumItems;
        // Continue until the Items array doesn't change.
    } while (NumItems < State->Items.Length);
}

void Transitions(cf_grammar Grammar, lr_fsa_state* State) {
    // Once we know what all of the items are, we can add transitions for each
    // incomplete item
    array_for (lr_item, I, State->Items) {
        cf_production Rule = GETRULE(Grammar, I.RuleNum);
        if (I.Position < Rule.Sequence.Length) {
            // TODO this can generate too many transitions.
            // right now, as we can see...
            // We only want one transition per symbol. i.e., if two rules both
            // have some terminal x as the next symbol in the item, then we should 
            // have only one transition on x
            lr_transition Transition = { .Symbol = Rule.Sequence.Data[I.Position], .SourceItem = I };
            // Transition.Dest filled out below

            array_push(lr_transition, &State->Transitions, Transition);
        }
    }
}


size_t MakeState(cf_grammar Grammar, array(lr_fsa_state)* States, ptrdiff_t PrevStateIndex, lr_item FromItem)
{
    assert(PrevStateIndex >= 0);
    lr_fsa_state* Result = array_push(lr_fsa_state, States, (lr_fsa_state){0});
    size_t ResultIndex = Result - States->Data;
    Result->Items = array_new(lr_item, 8);
    Result->Transitions = array_new(lr_transition, 8);
    
    lr_fsa_state* PrevState = States->Data + PrevStateIndex;
    cf_production FromRule = GETRULE(Grammar, FromItem.RuleNum);

    array_for (lr_item, Item, PrevState->Items) {
        cf_production Rule = GETRULE(Grammar, Item.RuleNum);
        if (Item.Position < Rule.Sequence.Length && 
            Rule.Sequence.Data[Item.Position] == 
             FromRule.Sequence.Data[FromItem.Position])
        {
            lr_item NewItem = { .RuleNum = Item.RuleNum, .Position = Item.Position + 1 };
            array_push(lr_item, &Result->Items, NewItem);
        }
    }

    ItemClosure(Grammar, Result);

    // Check if there are already any states with the same set of items as this
    // one -- if so, we do not need to create a new one.
    // This n^2 check is probably okay, since most grammars will have relatively few
    // number of rules (<1000), and so most of the state machines will have
    // relative few number of states (<10000)
    // Still not super comfortable with it, but it does seem to be the most
    // straightfoward way. I'll have to think about it more.
    array_for(lr_fsa_state, Other, (*States)) {
        if (Other.Items.Length != Result->Items.Length) continue;
        if (OtherRef == Result) { continue; }
        // Can the ordering be different?
        // I'm thinking no, since you always go through terminals/nonterminals 
        // in a specified order, and the grammar rules are ordered as well.
        array_for(lr_item, OtherItem, Other.Items) {
            lr_item ThisItem = Result->Items.Data[OtherItemIndex];
            if (memcmp(&OtherItem, &ThisItem, sizeof(lr_item)) != 0) { goto reject; } 
        }
accept:
        array_free(lr_item, &Result->Items);
        array_free(lr_transition, &Result->Transitions);
        array_pop(lr_fsa_state, States);
        return OtherIndex;
reject:
        continue;
    }

    Transitions(Grammar, Result);

    array_for (lr_transition, T, Result->Transitions) {
        TRef->DestIndex = MakeState(Grammar, States, ResultIndex, T.SourceItem);
        Result = States->Data + ResultIndex;
    }

    return ResultIndex;
}

void PrintParseTable(lr_parse_table Table) {
    printf("  ");
    for (cf_symbol_t Symbol = L_TerminalMIN + 1; Symbol < L_NonterminalMAX; ++Symbol) {
        printf("%16s", L_SymbolNames[Symbol]);
    }
    printf("\n");

    for (size_t RowIndex = 0; RowIndex < Table.Rows; ++RowIndex) {
        lr_table_transition* Row = Table.Table + RowIndex*Table.Cols;
        printf("%2d", (int)RowIndex);
        for (size_t ColIndex = 0; ColIndex < Table.Cols; ++ColIndex) {
            lr_table_transition T = Row[ColIndex];
            if (T.Type == T_SHIFT || T.Type == T_GOTO) {
                printf("\t\t%c%-4d",
                       (T.Type == T_SHIFT ? 'S' : 'G'),
                       (int)T.State);
            } else if (T.Type == T_REDUCE) {
                printf("\t\t%c%-4d", 'R', (int)T.RuleNum);
            } else {
                printf("\t\t");
            }
        }
        printf("\n");
    }
}

lr_parse_table Parse_BuildTable(cf_grammar Grammar)
{
    array(lr_fsa_state) States = array_new(lr_fsa_state, Grammar.Productions.Length);
    lr_fsa_state* StartState = array_push(lr_fsa_state, &States, (lr_fsa_state){0});

    StartState->Transitions = array_new(lr_transition, 8);
    StartState->Items = array_new(lr_item, 8);

    lr_item First = (lr_item){ .RuleNum = 0, .Position = 0 };
    lr_item* StartItem = array_push(lr_item, &StartState->Items, First);

    ItemClosure(Grammar, StartState);
    Transitions(Grammar, StartState);

    array_for (lr_transition, T, StartState->Transitions) {
        TRef->DestIndex = MakeState(Grammar, &States, 0, T.SourceItem);
        StartState = States.Data;
    }

    // At this point we have a graph of fsa_states (edges given by all the
    // transitions out of State).
    // Number each state (have them in array?) and use that to generate the
    // table!
    // One row for each state, one S/R column for each terminal, one Goto column
    // for each nonterminal

    lr_parse_table Result = { 
        .Rows = States.Length, 
        .Cols = L_NonterminalMAX, 
    };
    Result.Table = calloc(Result.Rows * Result.Cols, sizeof(lr_table_transition));
    
    // For each state:
    // The entries in the corresponding row in the table are:
    //   For terminals which correspond to transitions out of the state:
    //     SHIFT dest
    //   For nonterminals which correspond to transitions out of the state:
    //     GOTO dest

    array_for(lr_fsa_state, State, States) {
        lr_table_transition* Row = Result.Table + ((StateRef - States.Data) * Result.Cols);
        // One of two things should happen here, if the grammar is LR(0):
        //  1. One of the items in the state is complete (A ::= xyz .)
        //     In this case, every state in the terminal part of the row should become a REDUCE
        //  2. We have transitions out of the state, on some terminals or nonterminals
        //     In this case, we fill out the corresponding columns with SHIFT or GOTO
        //
        // If both are the case, then the grammar is NOT LR(0)! Which is fine,
        // a lot of grammars aren't, but we don't handle that yet. For now,
        // we're going to always prefer to shift rather than reduce. 
        //
        // Also, if we have REDUCE/REDUCE conflicts, later
        // terminals/nonterminals will overwrite earlier ones.

        int ReduceRule = -1;
        array_for(lr_item, I, State.Items) {
            if (I.Position == GETRULE(Grammar, I.RuleNum).Sequence.Length) {
                if (ReduceRule != -1) {
                    fprintf(stderr, "\n LR BUILD ERROR: Reduce/Reduce conflict in state %d: Rule %d and rule %d\n\n", StateIndex, ReduceRule, I.RuleNum);
                    goto fail;
                }
                ReduceRule = I.RuleNum;
                for (int Col = L_TerminalMIN + 1; Col < L_TerminalMAX; ++Col) {
                    // Hmm... need to be able to know what the rule number is.
                    // Or do we? All that matters is that the parser can use
                    // this information to take a few symbols off of the stack
                    // and push the reduced form on. Let's just include the rule
                    // directly for now. This might inflate the table size a bit.
                    Row[Col] = (lr_table_transition){ .Type = T_REDUCE, .RuleNum = I.RuleNum };
                }
            }
        }

        if (State.Transitions.Length > 0) {
            array_for(lr_transition, T, State.Transitions) {
                // We can reverse engineer the index in the states array of the
                // transition destination by subtracting the pointer
                if (L_TerminalMIN < T.Symbol && T.Symbol < L_TerminalMAX) {
                    if (Row[T.Symbol].Type == T_REDUCE) {
                        fprintf(stderr, "\n LR BUILD ERROR: Shift/Reduce conflict in state %d.\n", StateIndex);
                        fprintf(stderr, "    Reduce rule #%d, Shift to state %d.\n\n", Row[T.Symbol].RuleNum, T.DestIndex);
                        //goto fail;
                    }
                    Row[T.Symbol] = (lr_table_transition){ .Type = T_SHIFT, .State = T.DestIndex };
                } else if (L_NonterminalMIN < T.Symbol && T.Symbol < L_NonterminalMAX) {
                    Row[T.Symbol] = (lr_table_transition){ .Type = T_GOTO, .State = T.DestIndex };
                }
            }
        } 
    }

    FILE* f = fopen("table.nfa", "w");
    fprintf(f, "Nfa (id:0)\n");
    fprintf(f, "    All States (with hashcodes):\n");

    array_for(lr_fsa_state, State, States) {
        // I don't have actual names for these. What I *should* do, is
        // stringify and concatenate all of the rules involved, but the NFA
        // viewer won't handle that very well without some work. For now I'm
        // just going to use the pointers as the ids and names. Should at least
        // blah blah blah
        fprintf(f, "        %d (obj id: %p)\n", StateIndex, StateRef);
    }

    fprintf(f, "    Start State:   %d\n", 0);
    fprintf(f, "    Accept States:  [0]\n");
    fprintf(f, "    Transitions:\n");

    array_for(lr_fsa_state, State, States) {
        fprintf(f, "        %d:\n", StateIndex);
        array_for(lr_transition, Edge, State.Transitions) {
            fprintf(f, "            \001%s\001 -> %d\n", L_SymbolNames[Edge.Symbol], (int)Edge.DestIndex);
        }
    }
    fclose(f);

    // TODO clean up states
    return Result;

fail:
    PrintParseTable(Result);

    free(Result.Table);
    Result.Table = NULL;
    // TODO clean up states
    return Result;
}


static token
TranslateType(token Token) {
    switch(Token.Type) {
        case Comma: Token.Type = L_Comma; break; 
        case LParen: Token.Type = L_LParen; break;
        case RParen: Token.Type = L_RParen; break;
        case Identifier: Token.Type = L_Identifier; break;
        case EndOfFile: Token.Type = L_EOF; break;
    }
    return Token;
}

typedef struct {
    token Token;
    int State;
    lst_node* Data;
} lr_stack_item;
#define ARRAY_TYPE lr_stack_item
#include "array.c"

lst_node* Parse_List(tokenizer* Tokenizer)
{
    l_context Context = { malloc(0xf0000), 0, 0xf0000 };
    cf_grammar Grammar = TestGrammar();
    lr_parse_table Table = Parse_BuildTable(Grammar);

    if (Table.Table == NULL) { return NULL; }

    PrintParseTable(Table);

    array(lr_stack_item) Stack = array_new(lr_stack_item, 10);

    token Token = TranslateType(PeekToken(Tokenizer)); 
    int State = 0;

    do {
        if (Token.Type == L_TerminalMIN) { Token = TranslateType(PeekToken(Tokenizer)); }
        lr_table_transition Action = cell(Table, State, Token.Type);

        switch(Action.Type) {
            case T_SHIFT: {
                printf("Shift %d -> %d\n", State, (int)Action.State);

                lr_stack_item Pair = { Token, State, NULL };
                array_push(lr_stack_item, &Stack, Pair);
                State = Action.State;

                // Consume token
                GetToken(Tokenizer);
                Token = (token){ .Type = L_TerminalMIN };

            } break;

            case T_GOTO: {
                printf("Goto %d -> %d\n", State, (int)Action.State);
                State = Action.State;
                Token = (token){ .Type = L_TerminalMIN };
            } break;

            case T_REDUCE: {
                printf("Reduce rule %d\n", (int)Action.RuleNum);
                cf_production Rule = Grammar.Productions.Data[Action.RuleNum];
                token Tokens[CF_MAX_SYMBOLS_PER_RULE];
                lst_node* Parsed[CF_MAX_SYMBOLS_PER_RULE];
                for (int i = 0; i < Rule.Sequence.Length; ++i) {
                    lr_stack_item Pair = array_pop(lr_stack_item, &Stack);
                    Tokens[Rule.Sequence.Length - 1 - i] = Pair.Token;
                    Parsed[Rule.Sequence.Length - 1 - i] = Pair.Data;
                    State = Pair.State;
                }
                lr_stack_item ParsedItem = { 
                    .Token = { .Type = Rule.Nonterminal, .LineStart = Tokens[0].LineStart }, 
                    .State = State, 
                    .Data = Rule.Func(&Context, Tokens, Parsed) };
                array_push(lr_stack_item, &Stack, ParsedItem);

                Token = (token){ .Type = Rule.Nonterminal};
            } break;

            default: {
                // TODO better error messages!
                fprintf(stderr, "Syntax error of some kind on line %d:%d\n", Token.LineStart, Token.LineEnd);
                fprintf(stderr, "we're in state %d with a %s token\n", (int)State, L_SymbolNames[Token.Type]);
                array_free(lr_stack_item, &Stack);
                return NULL;
            } break;
        }
    } while (!(Stack.Length == 1 && array_peek(lr_stack_item, &Stack).Token.Type == L_Sp));

    lst_node* Root = array_pop(lr_stack_item, &Stack).Data;
    array_free(lr_stack_item, &Stack);
    return Root;
}

#if 0
void Foo(void)
{
    // TODO need a way to specify the code to generate/run when these rules are
    // matched.
    cf_production Productions[] = {

        /* § A.2.1 Expressions */
        GrammarRule(PrimaryExpression,  Identifier);
        GrammarRule(PrimaryExpression,  Constant);
        GrammarRule(PrimaryExpression,  StringLiteral);
        GrammarRule(PrimaryExpression,  LParen, Expression, RParen);

        GrammarRule(PostfixExpression,  PrimaryExpression);
        GrammarRule(PostfixExpression,  PostfixExpression, LBracket, Expression, RBracket);
        GrammarRule(PostfixExpression,  PostfixExpression, LParen, RParen);
        GrammarRule(PostfixExpression,  PostfixExpression, LParen, ArgumentExpressionList, RParen);
        GrammarRule(PostfixExpression,  PostfixExpression, Dot, Identifier);
        GrammarRule(PostfixExpression,  PostfixExpression, Arrow, Identifier);
        GrammarRule(PostfixExpression,  PostfixExpression, Increment);
        GrammarRule(PostfixExpression,  PostfixExpression, Decrement);
        GrammarRule(PostfixExpression,  LParen, TypeName, RParen, LCurly, InitializerList, RCurly);
        GrammarRule(PostfixExpression,  LParen, TypeName, RParen, LCurly, InitializerList, RCurly, Comma);

        GrammarRule(ArgumentExpressionList,  AssignmentExpression);
        GrammarRule(ArgumentExpressionList,  ArgumentExpressionList, AssignmentExpression);

        GrammarRule(UnaryExpression,  PostfixExpression);
        GrammarRule(UnaryExpression,  Increment, UnaryExpression);
        GrammarRule(UnaryExpression,  Decrement, UnaryExpression);
        GrammarRule(UnaryExpression,  UnaryOperator, CastExpression);
        GrammarRule(UnaryExpression,  SIZEOF, UnaryExpression);
        GrammarRule(UnaryExpression,  SIZEOF, LParen, Typename, RParen);
        
        GrammarRule(UnaryOperator,  Ampersand);
        GrammarRule(UnaryOperator,  Asterisk);
        GrammarRule(UnaryOperator,  Plus);
        GrammarRule(UnaryOperator,  Minus);
        GrammarRule(UnaryOperator,  BitNot);
        GrammarRule(UnaryOperator,  LogicNot);

        GrammarRule(CastExpression,  UnaryExpression);
        GrammarRule(CastExpression,  LParen, TypeName, RParen, CastExpression);

        GrammarRule(MultiplicativeExpression,  CastExpression);
        GrammarRule(MultiplicativeExpression,  MultiplicativeExpression, Asterisk, CastExpression);
        GrammarRule(MultiplicativeExpression,  MultiplicativeExpression, Divide, CastExpression);
        GrammarRule(MultiplicativeExpression,  MultiplicativeExpression, Modulus, CastExpression);

        GrammarRule(AdditiveExpression,  MultiplicativeExpression);
        GrammarRule(AdditiveExpression,  AdditiveExpression, Plus, MultiplicativeExpression);
        GrammarRule(AdditiveExpression,  AdditiveExpression, Minus, MulitplicativeExpression);

        GrammarRule(ShiftExpression,  AdditiveExpression);
        GrammarRule(ShiftExpression,  ShiftExpression, LBitShift, AdditiveExpression);
        GrammarRule(ShiftExpression,  ShiftExpression, RBitShift, AdditiveExpression);

        GrammarRule(RelationalExpression,  ShiftExpression);
        GrammarRule(RelationalExpression,  RelationalExpression, Less, ShiftExpression);
        GrammarRule(RelationalExpression,  RelationalExpression, Greater, ShiftExpression);
        GrammarRule(RelationalExpression,  RelationalExpression, LessEquals, ShiftExpression);
        GrammarRule(RelationalExpression,  RelationalExpression, GreaterEquals, ShiftExpression);

        GrammarRule(EqualityExpression,  RelationalExpression);
        GrammarRule(EqualityExpression,  EqualityExpression, Equals, RelationalExpression);
        GrammarRule(EqualityExpression,  EqualityExpression, LogicNotEquals, RelationalExpression);

        GrammarRule(ANDExpression,  EqualityExpression);
        GrammarRule(ANDExpression,  ANDExpression, Ampersand, EqualityExpression);

        GrammarRule(ExclusiveORExpression,  ANDExpression);
        GrammarRule(ExclusiveORExpression,  ExclusiveORExpression, BitXor, ANDExpression);

        GrammarRule(InclusiveORExpression, ExclusiveORExpression);
        GrammarRule(InclusiveORExpression, InclusiveORExpression, BitOr, ANDExpression);

        GrammarRule(LogicalANDExpression,  InclusiveORExpression);
        GrammarRule(LogicalANDExpression,  LogicalANDExpression, LogicAnd, InclusiveORExpression);

        GrammarRule(LogicalORExpression,  LogicalANDExpression);
        GrammarRule(LogicalORExpression,  LogicalORExpression, LogicOr, LogicalANDExpression);

        GrammarRule(ConditionalExpression,  LogicalORExpression);
        GrammarRule(ConditionalExpression,  ConditionalExpression, QuestionMark, Expression, Colon, ConditionalExpression);

        GrammarRule(AssignmentExpression,  ConditionalExpression);
        GrammarRule(AssignmentExpression,  UnaryExpression, AssignmentOperator, AssignmentExpression);

        GrammarRule(AssignmentOperator,  Assign);
        GrammarRule(AssignmentOperator,  TimesEquals);
        GrammarRule(AssignmentOperator,  DivideEquals);
        GrammarRule(AssignmentOperator,  ModulusEquals);
        GrammarRule(AssignmentOperator,  PlusEquals);
        GrammarRule(AssignmentOperator,  MinusEquals);
        GrammarRule(AssignmentOperator,  LBitShiftEquals);
        GrammarRule(AssignmentOperator,  RBitShiftEquals);
        GrammarRule(AssignmentOperator,  BitAndEquals);
        GrammarRule(AssignmentOperator,  BitXorEquals);
        GrammarRule(AssignmentOperator,  BitOrEquals);

        GrammarRule(Expression,  AssignmentExpression);
        GrammarRule(Expression,  Expression, Comma, AssignmentExpression);

        GrammarRule(ConstantExpression,  ConditionalExpression);

        GrammarRule(ConstantExpression,  ConditionalExpression);

        /* § A.2.2 Declarations */
        GrammarRule(Declaration,  DeclarationSpecifiers, Semicolon);
        GrammarRule(Declaration,  DeclarationSpecifiers, InitDeclaratorList, Semicolon);

        GrammarRule(DeclarationSpecifiers,  StorageClassSpecifier);
        GrammarRule(DeclarationSpecifiers,  StorageClassSpecifier, DeclarationSpecifiers);
        GrammarRule(DeclarationSpecifiers,  TypeSpecifier);
        GrammarRule(DeclarationSpecifiers,  TypeSpecifier, DeclarationSpecifiers);
        GrammarRule(DeclarationSpecifiers,  TypeQualifier);
        GrammarRule(DeclarationSpecifiers,  TypeQualifier, DeclarationSpecifiers);
        GrammarRule(DeclarationSpecifiers,  FunctionSpecifier);
        GrammarRule(DeclarationSpecifiers,  FunctionSpecifier, DeclarationSpecifiers);

        GrammarRule(InitDeclaratorList,  InitDeclarator);
        GrammarRule(InitDeclaratorList,  InitDeclaratorList, Comma, InitDeclarator);

        GrammarRule(InitDeclarator,  Declarator);
        GrammarRule(InitDeclarator,  Declarator, Equals, Initializer);

        GrammarRule(StorageClassSpecifier,  Typedef);
        GrammarRule(StorageClassSpecifier,  Extern);
        GrammarRule(StorageClassSpecifier,  Static);
        GrammarRule(StorageClassSpecifier,  Auto);
        GrammarRule(StorageClassSpecifier,  Register);

        GrammarRule(TypeSpecifier,  VOID);
        GrammarRule(TypeSpecifier,  CHAR);
        GrammarRule(TypeSpecifier,  SHORT);
        GrammarRule(TypeSpecifier,  INT);
        GrammarRule(TypeSpecifier,  LONG);
        GrammarRule(TypeSpecifier,  FLOAT);
        GrammarRule(TypeSpecifier,  DOUBLE);
        GrammarRule(TypeSpecifier,  SIGNED);
        GrammarRule(TypeSpecifier,  UNSIGNED);
        GrammarRule(TypeSpecifier,  _BOOL);
        GrammarRule(TypeSpecifier,  _COMPLEX);
        GrammarRule(TypeSpecifier,  StructOrUnionSpecifier);
        GrammarRule(TypeSpecifier,  EnumSpecifier);
        GrammarRule(TypeSpecifier,  TypedefName);

        GrammarRule(StructOrUnionSpecifier,  StructOrUnion, LCurly, StructDeclarationList, RCurly);
        GrammarRule(StructOrUnionSpecifier,  StructOrUnion, Identifier, LBracket, StructDeclarationList, RBracket);
        GrammarRule(StructOrUnionSpecifier,  StructOrUnion, Identifier);

        GrammarRule(StructOrUnion,  Struct);
        GrammarRule(StructOrUnion,  Union);

        GrammarRule(StructDeclarationList,  StructDeclaration);
        GrammarRule(StructDeclarationList,  StructDeclarationList, StructDeclaration);

        GrammarRule(StructDeclaration,  SpecifierQualifierList, StructDeclaratorList);

        GrammarRule(SpecifierQualifierList,  TypeSpecifier);
        GrammarRule(SpecifierQualifierList,  TypeSpecifier, SpecifierQualifierList);
        GrammarRule(SpecifierQualifierList,  TypeQualifier);
        GrammarRule(SpecifierQualifierList,  TypeQualifier, SpecifierQualifierList);

        GrammarRule(StructDeclaratorList,  StructDeclarator);
        GrammarRule(StructDeclaratorList,  StructDeclaratorList, Comma, StructDeclarator);

        GrammarRule(StructDeclarator,  Declarator);
        GrammarRule(StructDeclarator,  Colon, ConstantExpression);
        GrammarRule(StructDeclarator,  Declarator, Colon, ConstantExpression);

        GrammarRule(EnumSpecifier,  ENUM, LCurly, EnumeratorList, RCurly);
        GrammarRule(EnumSpecifier,  ENUM, Identifier, LBracket, EnumeratorList, RCurly);
        GrammarRule(EnumSpecifier,  ENUM, LCurly, EnumeratorList, Comma, RCurly);
        GrammarRule(EnumSpecifier,  ENUM, Identifier, LBracket, EnumeratorList, Comma, RCurly);
        GrammarRule(EnumSpecifier,  ENUM, Identifier);

        GrammarRule(EnumeratorList,  Enumerator);
        GrammarRule(EnumeratorList,  EnumeratorList, Comma, Enumerator);

        GrammarRule(Enumerator,  EnumerationConstant);
        GrammarRule(Enumerator,  EnumerationConstant, Equals, ConstantExpression);

        GrammarRule(TypeQualifier,  CONST);
        GrammarRule(TypeQualifier,  RESTRICT);
        GrammarRule(TypeQualifier,  VOLATILE);

        GrammarRule(FunctionSpecifier,  INLINE);

        GrammarRule(Declarator,  DirectDeclarator);
        GrammarRule(Declarator,  Pointer, DirectDeclarator);

        GrammarRule(DirectDeclarator,  Identifier);
        GrammarRule(DirectDeclarator,  LParen, Declarator, RParen);
        GrammarRule(DirectDeclarator,  DirectDeclarator, LBracket, RBracket);
        GrammarRule(DirectDeclarator,  DirectDeclarator, LBracket, AssignmentExpression, RBracket);
        GrammarRule(DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, RBracket);
        GrammarRule(DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, AssignmentExpression, RBracket);
        GrammarRule(DirectDeclarator,  DirectDeclarator, LBracket, STATIC, AssignmentExpression, RBracket);
        GrammarRule(DirectDeclarator,  DirectDeclarator, LBracket, STATIC, TypeQualifierList, AssignmentExpression, RBracket);
        GrammarRule(DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, STATIC, AssignmentExpression, RBracket);
        GrammarRule(DirectDeclarator,  DirectDeclarator, LBracket, TypeQualifierList, Asterisk, RBracket);
        GrammarRule(DirectDeclarator,  DirectDeclarator, LParen, ParameterTypeList, RParen);
        GrammarRule(DirectDeclarator,  DirectDeclarator, LParen, RParen);
        GrammarRule(DirectDeclarator,  DirectDeclarator, LParen, IdentifierList, RParen);

        GrammarRule(Pointer,  Asterisk);
        GrammarRule(Pointer,  Asterisk, TypeQualifierList);
        GrammarRule(Pointer,  Asterisk, Pointer);
        GrammarRule(Pointer,  Asterisk, TypeQualifierList, Pointer);

        GrammarRule(TypeQualifierList,  TypeQualifier);
        GrammarRule(TypeQualifierList,  TypeQualifierList, TypeQualifier);

        GrammarRule(ParameterTypeList,  ParameterList);
        GrammarRule(ParameterTypeList,  ParameterList, Comma, Dot, Dot, Dot);

        GrammarRule(ParameterList,  ParameterDeclaration);
        GrammarRule(ParameterList,  ParameterList, Comma, ParameterDeclaration);

        GrammarRule(ParameterDeclaration,  DeclarationSpecifiers, Declarator);
        GrammarRule(ParameterDeclaration,  DeclarationSpecifiers);
        GrammarRule(ParameterDeclaration,  DeclarationSpecifiers, AbstractDeclarator);

        GrammarRule(IdentifierList,  Identifier);
        GrammarRule(IdentifierList,  IdentifierList, Comma, Identifier);

        GrammarRule(TypeName,  SpecifierQualifierList);
        GrammarRule(TypeName,  SpecifierQualifierList, AbstractDeclarator);

        GrammarRule(AbstractDeclarator,  Pointer);
        GrammarRule(AbstractDeclarator,  DirectAbstractDeclarator);
        GrammarRule(AbstractDeclarator,  Pointer, DirectAbstractDeclarator);

        GrammarRule(DirectAbstractDeclarator,  LParen, AbstractDeclarator, RParen);

        GrammarRule(DirectAbstractDeclarator,  LBracket, RBracket);
        GrammarRule(DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, RBracket);
        GrammarRule(DirectAbstractDeclarator,  LBracket, TypeQualifierList, RBracket);
        GrammarRule(DirectAbstractDeclarator,  LBracket, AssignmentExpression, RBracket);
        GrammarRule(DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, TypeQualifierList, RBracket);
        GrammarRule(DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, AssignmentExpression, RBracket);
        GrammarRule(DirectAbstractDeclarator,  LBracket, TypeQualifierList, AssignmentExpression, RBracket);
        GrammarRule(DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, TypeQualifierList, AssignmentExpression, RBracket);

        GrammarRule(DirectAbstractDeclarator,  LBracket, STATIC, AssignmentExpression, RBracket);
        GrammarRule(DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, STATIC, AssignmentExpression, RBracket);
        GrammarRule(DirectAbstractDeclarator,  LBracket, STATIC, TypeQualifierList, AssignmentExpression, RBracket);
        GrammarRule(DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, STATIC, TypeQualifierList, AssignmentExpression, RBracket);

        GrammarRule(DirectAbstractDeclarator,  LBracket, TypeQualifierList, STATIC, AssignmentExpression, RBracket);
        GrammarRule(DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, TypeQualifierList, STATIC, AssignmentExpression, RBracket);

        GrammarRule(DirectAbstractDeclarator,  LBracket, Asterisk, RBracket);
        GrammarRule(DirectAbstractDeclarator,  DirectAbstractDeclarator, LBracket, Asterisk, RBracket);

        GrammarRule(DirectAbstractDeclarator,  LParen, RParen);
        GrammarRule(DirectAbstractDeclarator,  DirectAbstractDeclarator, LParen, RParen);
        GrammarRule(DirectAbstractDeclarator,  LParen, ParameterTypeList, RParen);
        GrammarRule(DirectAbstractDeclarator,  DirectAbstractDeclarator, LParen, ParameterTypeList, RParen);

        GrammarRule(TypedefName,  Identifier);

        GrammarRule(Initializer,  AssignmentExpression);
        GrammarRule(Initializer,  LCurly, InitializerList, RCurly);
        GrammarRule(Initializer,  LCurly, InitializerList, Comma, RCurly);

        GrammarRule(InitializerList,  Initializer);
        GrammarRule(InitializerList,  Designation, Initializer);
        GrammarRule(InitializerList,  InitializerList, Comma, Initializer);
        GrammarRule(InitializerList,  InitializerList, Comma, Designation, Initializer);

        GrammarRule(Designation,  DesignatorList, Equals);

        GrammarRule(DesignatorList,  Designator);
        GrammarRule(DesignatorList,  DesignatorList, Designator);

        GrammarRule(Designator,  LBracket, ConstantExpression, RBracket);
        GrammarRule(Designator,  Dot, Identifier);
 
        /* § A.2.3 Statements */
    };


    cf_grammar Grammar = { countof(Productions), Productions };
}
#endif
