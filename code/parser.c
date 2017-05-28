#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "scanner.h"
#include "parser_c.h"
#include "parser.h"

#define PRINT_SLR_TABLES 0

#define GrammarRule(ParseFunc, Nonterminal, ...) _GrammarRule(ParseFunc, Nonterminal, ##__VA_ARGS__, TerminalMIN)
#define GETRULE(Grammar, N) (Grammar)->Productions.Data[N]

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
    } while (Next != TerminalMIN && Result.Sequence.Length < CF_MAX_SYMBOLS_PER_RULE);

    return Result;
}


void GenerateFollowTable(cf_grammar* Grammar);

#include "parser_c.c"

void PrintRule(FILE* f, cf_grammar* Grammar, int RuleNum) {
    cf_production Rule = GETRULE(Grammar, RuleNum);
    fprintf(f, "%s <- ", SymbolStr(Rule.Nonterminal));
    array_for (cf_symbol_t, Sym, Rule.Sequence) {
        fprintf(f, "%s ", SymbolStr(Sym));
    }
}

void PrintItem(FILE* f, cf_grammar* Grammar, lr_item Item) {
    cf_production Rule = GETRULE(Grammar, Item.RuleNum);
    fprintf(f, "%s <- ", SymbolStr(Rule.Nonterminal));
    array_for (cf_symbol_t, Sym, Rule.Sequence) {
        if (SymIndex == Item.Position) fprintf(f, ". ");
        fprintf(f, "%s ", SymbolStr(Sym));
    }
    if (Item.Position == Rule.Sequence.Length) fprintf(f, ". ");
}

void PrintState(FILE* f, cf_grammar* Grammar, lr_fsa_state State) {
    fprintf(f, "\n");
    array_for(lr_item, Item, State.Items) {
        fprintf(f, "\t| ");
        PrintItem(f, Grammar, Item);
        fprintf(f, "\n");
    }
    fprintf(f, "\n");
}

void PrintRelevantState(FILE* f, cf_grammar* Grammar, lr_fsa_state State, cf_symbol_t Prev, cf_symbol_t Next, bool IncludeReduceable) {
    fprintf(f, "\n");
    array_for(lr_item, Item, State.Items) {
        cf_production Rule = GETRULE(Grammar, Item.RuleNum);
        if ((Prev != NonterminalMIN && Item.Position > 0 && Rule.Sequence.Data[Item.Position - 1] == Prev) ||
            (Next != NonterminalMIN && Item.Position < Rule.Sequence.Length && Rule.Sequence.Data[Item.Position] == Next) ||
            (IncludeReduceable && Item.Position == Rule.Sequence.Length)) {
            fprintf(f, "\t| ");
            PrintItem(f, Grammar, Item);
            fprintf(f, "\n");
        }
    }
    fprintf(f, "\t| ...\n\n");
}

#if 0
static bool
AllNonterminal(set(cf_symbol_t) S) {
    set_for(cf_symbol_t, Item, S) {
        if (!(NonterminalMIN < Item && Item < NonterminalMAX)) return false;
    }
    return true;
}

static bool
AllTerminal(set(cf_symbol_t) S) {
    set_for(cf_symbol_t, Item, S) {
        if (!(TerminalMIN < Item && Item < TerminalMAX)) return false;
    }
    return true;
}
#endif

void GenerateFollowTable(cf_grammar* Grammar) {
    set(cf_symbol_t) First[NonterminalMAX];
    set(cf_symbol_t) Follow[NonterminalMAX];
    bool Nullable[NonterminalMAX];

    for (cf_symbol_t Sym = TerminalMIN + 1; Sym < TerminalMAX; ++Sym) {
        Nullable[Sym] = false;
        First[Sym] = set_init(cf_symbol_t, 1, Sym);
        Follow[Sym] = set_init(cf_symbol_t, 0);
    }

    for (cf_symbol_t Sym = NonterminalMIN + 1; Sym < NonterminalMAX; ++Sym) {
        Nullable[Sym] = false; // Assume not nullable until proven
        First[Sym] = set_init(cf_symbol_t, 0);
        Follow[Sym] = set_init(cf_symbol_t, 0);
    }

    bool Changed;
    do {
        Changed = false;
        array_for(cf_production, Rule, Grammar->Productions) {
            int k = Rule.Sequence.Length;
            cf_symbol_t X = Rule.Nonterminal;

            // Check if the nonterminal production is nullable or is composed entirely of
            // nullable productions
            if (k == 0) { Nullable[X] = true; }
            bool AllNullable = true;
            set_for(cf_symbol_t, Item, Rule.Sequence) {
                AllNullable = Nullable[Item] && AllNullable;
            }
            if (AllNullable) {
                Changed = Changed || !Nullable[Rule.Nonterminal];
                Nullable[Rule.Nonterminal] = true;
            }

            for (int i = 0; i <= k - 1; ++i) {
                cf_symbol_t Yi = Rule.Sequence.Data[i];

                // If everything up to this point in the rule is nullable,
                // then the FIRST of the nonterminal includes the FIRST of this
                // symbol
                AllNullable = true;
                for (int Index = 0; Index <= i - 1; Index++) { 
                    cf_symbol_t Item = Rule.Sequence.Data[Index];
                    AllNullable = Nullable[Item] && AllNullable;
                }
                if (AllNullable) { 
                    Changed = Changed || set_union(cf_symbol_t, &First[X], First[Yi]) > 0;
                }

                // If everything up after this point in the rule is nullable,
                // then the FOLLOW of this symbol includes the FOLLOW of the
                // nonterminal
                AllNullable = true;
                for (int Index = i + 1; Index <= k - 1; Index++) { 
                    cf_symbol_t Item = Rule.Sequence.Data[Index];
                    AllNullable = Nullable[Item] && AllNullable;
                }
                if (AllNullable) {
                    Changed = Changed || set_union(cf_symbol_t, &Follow[Yi], Follow[X]) > 0;
                }

                // If everything between this point in the rule and a future
                // point in the rule is nullable, then the FOLLOW of this symbol
                // includes the FIRST of that point in the rule
                for (int j = i + 1; j <= k - 1; ++j) {
                cf_symbol_t Yj = Rule.Sequence.Data[j];
                    AllNullable = true;
                    for (int Index = i + 1; Index <= j - 1; Index++) { 
                        cf_symbol_t Item = Rule.Sequence.Data[Index];
                        AllNullable = Nullable[Item] && AllNullable;
                    }
                    if (AllNullable) { 
                        Changed = Changed || set_union(cf_symbol_t, &Follow[Yi], First[Yj]) > 0;
                    }
                }
            }
        }
    } while (Changed);

    // Special case grammar rule 0, since we are defining it as the overarching
    // rule in the grammar and it needs to accept on an EOF
    set_add(cf_symbol_t, &Follow[Grammar->Root], EndOfFile);

#if PRINT_SLR_TABLES
    for (cf_symbol_t Sym = NonterminalMIN + 1; Sym < NonterminalMAX; ++Sym) {
        printf("%s:\n", SymbolStr(Sym));
        printf("\tNullable: %c\n", Nullable[Sym] ? 'T' : 'F');
        printf("\tFIRST: ");
        set_for(cf_symbol_t, Item, First[Sym]) printf("%s ", SymbolStr(Item));
        printf("\n");
        printf("\tFOLLOW: ");
        set_for(cf_symbol_t, Item, Follow[Sym]) printf("%s ", SymbolStr(Item));
        printf("\n");
    }
#endif

    memcpy(Grammar->FollowTable, Follow, sizeof(Follow));
    memcpy(Grammar->FirstTable, First, sizeof(First));
}

void ItemClosure(cf_grammar* Grammar, lr_fsa_state* State)
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
            if (NonterminalMIN < Symbol && Symbol < NonterminalMAX) {
                array_for(cf_production, OtherRule, Grammar->Productions) {
                    if (OtherRule.Nonterminal == Symbol) {
                        lr_item NewItem = (lr_item){ 
                            .RuleNum = OtherRuleRef - Grammar->Productions.Data, 
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

void Transitions(cf_grammar* Grammar, lr_fsa_state* State) {
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

typedef struct {
    // Index of state coming from
    int SourceIndex; 
    // Item transitioning on
    lr_item SourceItem; 
    // Index of transition in the source item's transitions array that will need to be updated
    int TransitionIndex; 
} fsa_state_task;
#define ARRAY_TYPE fsa_state_task
#include "array.c"

int MakeState(cf_grammar* Grammar, array(lr_fsa_state)* States, array(fsa_state_task)* WorkQueue,
              int PrevStateIndex, lr_item FromItem)
{
    assert(PrevStateIndex >= 0);
    lr_fsa_state* Result = array_push(lr_fsa_state, States, (lr_fsa_state){0});
    int ResultIndex = Result - States->Data;
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
        fsa_state_task Task = {
            .SourceIndex = ResultIndex,
            .SourceItem = T.SourceItem,
            .TransitionIndex = TIndex,
        };
        array_push(fsa_state_task, WorkQueue, Task);
    }

    return ResultIndex;
}

static void PrintParseTable(lr_parse_table Table) {
    FILE* f = fopen("table.csv", "w");
    assert(f != NULL);
    fprintf(f, "STATE,");
    for (cf_symbol_t Symbol = TerminalMIN + 1; Symbol < NonterminalMAX; ++Symbol) {
        fprintf(f, "%s,", SymbolStr(Symbol));
    }
    fprintf(f, "\n");

    for (size_t RowIndex = 0; RowIndex < Table.Rows; ++RowIndex) {
        lr_table_transition* Row = Table.Table + RowIndex*Table.Cols;
        fprintf(f, "%d,", (int)RowIndex);
        for (size_t ColIndex = 0; ColIndex < Table.Cols; ++ColIndex) {
            lr_table_transition T = Row[ColIndex];
            if (T.Type == T_SHIFT || T.Type == T_GOTO) {
                fprintf(f, "%c%d,",
                       (T.Type == T_SHIFT ? 'S' : 'G'),
                       (int)T.State);
            } else if (T.Type == T_REDUCE) {
                fprintf(f, "%c%d,", 'R', (int)T.RuleNum);
            } else {
                fprintf(f, ",");
            }
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

static array(lr_fsa_state) _States;

lr_parse_table Parse_BuildTable(cf_grammar* Grammar)
{
    array(lr_fsa_state) States = array_new(lr_fsa_state, Grammar->Productions.Length);
    lr_fsa_state* StartState = array_push(lr_fsa_state, &States, (lr_fsa_state){0});

    StartState->Transitions = array_new(lr_transition, 8);
    StartState->Items = array_new(lr_item, 8);

    lr_item First = (lr_item){ .RuleNum = 0, .Position = 0 };
    lr_item* StartItem = array_push(lr_item, &StartState->Items, First);

    ItemClosure(Grammar, StartState);
    Transitions(Grammar, StartState);

    array(fsa_state_task) WorkQueue = array_new(fsa_state_task, States.Length);

    array_for (lr_transition, T, StartState->Transitions) {
        fsa_state_task Task = {
            .SourceIndex = 0,
            .SourceItem = T.SourceItem,
            .TransitionIndex = TIndex,
        };
        array_push(fsa_state_task, &WorkQueue, Task);
    }

    while (WorkQueue.Length > 0) {
        fsa_state_task Task = array_pop(fsa_state_task, &WorkQueue);
        int StateIndex = MakeState(Grammar, &States, &WorkQueue, Task.SourceIndex, Task.SourceItem);
        States.Data[Task.SourceIndex].Transitions.Data[Task.TransitionIndex].DestIndex = StateIndex;
    }

    // At this point we have a graph of fsa_states (edges given by all the
    // transitions out of State).
    // Number each state and use that to generate the table!
    // One row for each state, one S/R column for each terminal, one Goto column
    // for each nonterminal

    lr_parse_table Result = { 
        .Rows = States.Length, 
        .Cols = NonterminalMAX, 
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

        array_for(lr_item, I, State.Items) {
            cf_production Rule = GETRULE(Grammar, I.RuleNum);
            if (I.Position == Rule.Sequence.Length) {
                // Go through all of the terminals in the follow set for the
                // rule's nonterminal and add reduce transitions on those only
                set_for(cf_symbol_t, Sym, Grammar->FollowTable[Rule.Nonterminal]) {
                    assert(TerminalMIN < Sym && Sym < TerminalMAX);
                    if (Row[Sym].Type == T_REDUCE) {
                        fprintf(stderr, "\n LR BUILD ERROR: Reduce/Reduce conflict in state %d on %s.\n", StateIndex, SymbolStr(Sym));
                        PrintState(stderr, Grammar, State);
                        fprintf(stderr, "    Rule #%d: ", Row[Sym].RuleNum);
                        PrintRule(stderr, Grammar, Row[Sym].RuleNum);
                        fprintf(stderr, "\n    Rule #%d: ", I.RuleNum);
                        PrintRule(stderr, Grammar, I.RuleNum);
                        fprintf(stderr, "\n    Resolving in favor of reducing #%d.\n\n", I.RuleNum);
                        //goto fail;
                    }
                    Row[Sym] = (lr_table_transition){ .Type = T_REDUCE, .RuleNum = I.RuleNum };
                }
            }
        }

        if (State.Transitions.Length > 0) {
            array_for(lr_transition, T, State.Transitions) {
                // We can reverse engineer the index in the states array of the
                // transition destination by subtracting the pointer
                if (TerminalMIN < T.Symbol && T.Symbol < TerminalMAX) {
                    if (Row[T.Symbol].Type == T_REDUCE) {
                        fprintf(stderr, "\n LR BUILD ERROR: Shift/Reduce conflict in state %d on %s:\n", StateIndex, SymbolStr(T.Symbol));
                        PrintRelevantState(stderr, Grammar, State, 0, T.Symbol, true);
                        fprintf(stderr, "    Reduce rule #%d: ", Row[T.Symbol].RuleNum);
                        PrintRule(stderr, Grammar, Row[T.Symbol].RuleNum);
                        fprintf(stderr, "\n    Shift state %d -> state %d on %s.\n", StateIndex, T.DestIndex, SymbolStr(T.Symbol));
                        PrintRelevantState(stderr, Grammar, States.Data[T.DestIndex], T.Symbol, 0, false);
                        fprintf(stderr, "    Resolving in favor of shifting to %d.\n\n", T.DestIndex);
                        //goto fail;
                    }
                    Row[T.Symbol] = (lr_table_transition){ .Type = T_SHIFT, .State = T.DestIndex };
                } else if (NonterminalMIN < T.Symbol && T.Symbol < NonterminalMAX) {
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
            fprintf(f, "            \001%s\001 -> %d\n", SymbolStr(Edge.Symbol), (int)Edge.DestIndex);
        }
    }
    fclose(f);

    // TODO clean up states
    _States = States;

    return Result;

fail:
    PrintParseTable(Result);

    free(Result.Table);
    Result.Table = NULL;
    // TODO clean up states
    return Result;
}


typedef struct {
    token Token;
    int State;
    void* Data;
} lr_stack_item;
#define ARRAY_TYPE lr_stack_item
#include "array.c"

void* Parse(tokenizer* Tokenizer)
{
    context Context = { malloc(0xf0000), 0, 0xf0000 };
    cf_grammar Grammar = GenerateGrammar();
    lr_parse_table Table = Parse_BuildTable(&Grammar);

    if (Table.Table == NULL) { return NULL; }

    //PrintParseTable(Table);

    array(lr_stack_item) Stack = array_new(lr_stack_item, 10);

    token Token = Process(PeekToken(Tokenizer)); 
    int State = 0;

    do {
        if (Token.Type == TerminalMIN) { Token = Process(PeekToken(Tokenizer)); }
        lr_table_transition Action = cell(Table, State, Token.Type);

        switch(Action.Type) {
            case T_SHIFT: {
                //printf("Shift %d -> %d on %s\n", State, (int)Action.State, SymbolStr(Token.Type));

                lr_stack_item Pair = { Token, State, NULL };
                array_push(lr_stack_item, &Stack, Pair);
                State = Action.State;

                // Consume token
                GetToken(Tokenizer);
                Token = (token){ .Type = TerminalMIN };

            } break;

            case T_GOTO: {
                //printf("Goto %d -> %d\n", State, (int)Action.State);
                State = Action.State;
                Token = (token){ .Type = TerminalMIN };
            } break;

            case T_REDUCE: {
                //printf("Reduce rule %d ", (int)Action.RuleNum);
                //PrintRule(stdout, &Grammar, Action.RuleNum);
                //printf("\n");
                cf_production Rule = Grammar.Productions.Data[Action.RuleNum];
                token Tokens[CF_MAX_SYMBOLS_PER_RULE];
                void* Parsed[CF_MAX_SYMBOLS_PER_RULE];
                for (int i = 0; i < Rule.Sequence.Length; ++i) {
                    lr_stack_item Pair = array_pop(lr_stack_item, &Stack);
                    Tokens[Rule.Sequence.Length - 1 - i] = Pair.Token;
                    Parsed[Rule.Sequence.Length - 1 - i] = Pair.Data;
                    State = Pair.State;
                }
                lr_stack_item ParsedItem = { 
                    .Token = { .Type = Rule.Nonterminal, .LineStart = Tokens[0].LineStart }, 
                    .State = State, 
                    .Data = Rule.Func(&Context, 
                                      array_wrap(token, Rule.Sequence.Length, Tokens), 
                                      array_wrap(cst_node, Rule.Sequence.Length, (cst_node*)Parsed)) };
                array_push(lr_stack_item, &Stack, ParsedItem);

                Token = (token){ .Type = Rule.Nonterminal};
            } break;

            default: {
                // TODO better error messages!
                fprintf(stderr, "\nSyntax error on line %d:%d\n", Token.LineStart, Token.LineEnd);
                fprintf(stderr, "   got: %s\n", SymbolStr(Token.Type));
                PrintState(stderr, &Grammar, _States.Data[State]);
                fprintf(stderr, "\n");
                //fprintf(stderr, "we're in state %d with a %s token\n", (int)State, SymbolStr(Token.Type));
#if 0
                if (Token.Type == L_Identifier) {
                    fprintf(stderr, "\tid(%s)\n", Tokenizer->Start);
                }
#endif
                array_free(lr_stack_item, &Stack);
                return NULL;
            } break;
        }
    } while (!(Stack.Length == 1 && array_peek(lr_stack_item, &Stack).Token.Type == Grammar.Root));

    void* Root = array_pop(lr_stack_item, &Stack).Data;
    array_free(lr_stack_item, &Stack);
    return Root;
}
