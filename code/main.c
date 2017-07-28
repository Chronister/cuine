#include <stdlib.h>
#include <stdio.h>
#include "scanner.h"

#include "scanner.c"
#include "parser.c"

void Usage(const char* ProgName) {
    printf("Usage: %s <filename>\n", ProgName);
}

char* ReadFileAndNullTerminate(const char* Filename)
{
    FILE* InFile = NULL;
    InFile = fopen(Filename, "rb");
    if (InFile == NULL) {
        fprintf(stderr, "Could not open file %s\n", Filename);
        return NULL;
    }

    fseek(InFile, 0, SEEK_END);
    size_t InFileSize = ftell(InFile);
    fseek(InFile, 0, SEEK_SET);

    char* In = (char*)malloc(InFileSize + 1);
    int Result = fread(In, 1, InFileSize, InFile);
    if (Result != InFileSize) {
        fprintf(stderr, "Only got %d of %d bytes of file!", Result, (int)InFileSize);
        exit(1);
    }
    In[InFileSize] = 0;
    fclose(InFile);
    return In;
}

#if 0
void WalkTree(lst_node* Node, int Tab) {
    switch(Node->Type) {
        case LST_Identifier:
            lst_node_identifier* S = (lst_node_identifier*)Node;
            printf(" %.*s", S->ValueLength, S->Value);
            break;
        case LST_List:
            printf(" [", Node->SourceLine);

            lst_node_list* L = ((lst_node_list*)Node)->First;
            while (L != NULL) {
                if (L->Data)
                    WalkTree(L->Data, Tab + 1);
                L = L->Next;
            }

            printf(" ]", Node->SourceLine);
            break;
    }
}
#endif

void WalkTree(cst_node Node, int Tab, bool Inline) {
    if (!Inline && (Node ? (*(cst_node_type*)Node != CST_Block && *(cst_node_type*)Node != CST_DeclarationList) : 1))
        for (int i = 0; i < Tab; ++i) printf("  ");

    if (Node == NULL) {
        printf("null"); 
        if (!Inline) printf("\n");
        return;
    }
    switch(*(cst_node_type*)Node) 
    {
        case CST_TranslationUnit:
        {
            printf("translation unit:");
            if (!Inline) printf("\n");
            array_for(cst_node, Decl, ((cst_node_translation_unit*)Node)->ExternDecls) {
                WalkTree(Decl, Tab + 1, Inline);
            }
            if (!Inline) printf("\n");
        } break;
            
        case CST_FunctionType:
        {
            cst_node_function_type* Func = (cst_node_function_type*)Node;
            printf("function");
            printf(" (");
            array_for(cst_node, Argument, Func->Arguments) {
                WalkTree(Argument, Tab + 1, true);
                if (ArgumentIndex < Func->Arguments.Length - 1) printf(", ");
            }
            printf(") -> ");
            WalkTree(Func->ReturnType, Tab + 1, true);
            if (!Inline) printf("\n");
            if (Func->Body) WalkTree(Func->Body, Tab, Inline);
        } break;

        case CST_Block:
            //array_for(cst_node, Stmt, ((cst_node_block*)Node)->Statements) {
            //    WalkTree(Stmt, Tab + 1, false);
            //}
            break;

        case CST_Declaration:
        {
            cst_node_declaration* Decl = (cst_node_declaration*)Node;
            if (Decl->SpecifierFlags & DECL_TYPEDEF) printf("typedef ");
            if (Decl->SpecifierFlags & DECL_EXTERN) printf("extern ");
            if (Decl->SpecifierFlags & DECL_STATIC) printf("static ");
            if (Decl->SpecifierFlags & DECL_AUTO) printf("auto ");
            if (Decl->SpecifierFlags & DECL_REGISTER) printf("register ");
            if (Decl->SpecifierFlags & DECL_CONST) printf("const ");
            if (Decl->SpecifierFlags & DECL_RESTRICT) printf("restrict ");
            if (Decl->SpecifierFlags & DECL_VOLATILE) printf("volatile ");
            if (Decl->SpecifierFlags & DECL_INLINE) printf("inline ");
            WalkTree(Decl->BaseType, Tab + 1, true);
            array_rof(cst_declaration_flags, Pointer, Decl->PointerLevel) {
                printf("* ");
                if (Pointer & DECL_CONST) printf("const ");
                if (Pointer & DECL_RESTRICT) printf("restrict ");
                if (Pointer & DECL_VOLATILE) printf("volatile ");
            }
            WalkTree(Decl->Name, Tab + 1, true);

            if (Decl->Initializer != NULL) {
                printf(" = ");
                WalkTree(Decl->Initializer, Tab + 1, true);
            }

            if (!Inline) printf("\n");
        } break;

        case CST_DeclarationList:
        {
            cst_node_declaration_list* List = (cst_node_declaration_list*)Node;
            //printf("declaration list:");
            //if (!Inline) printf("\n");
            array_for(cst_node, Decl, List->Declarations) {
                WalkTree(Decl, Tab, false);
            }
        } break;

        case CST_Identifier:
        {
            cst_node_identifier* Ident = (cst_node_identifier*)Node;
            printf("%.*s", Ident->TextLength, Ident->Text);
        } break;

        case CST_StringConstant:
        {
            cst_node_string_constant* String = (cst_node_string_constant*)Node;
            printf("\"%.*s\"", String->TextLength, String->Text);
        } break;

        case CST_BuiltinType:
        {
            cst_node_builtin_type* TypeNode = (cst_node_builtin_type*)Node;
            if (TypeNode->Type & TYPE_VOID) printf("void ");
            if (TypeNode->Type & TYPE_BOOL) printf("_Bool ");
            if (TypeNode->Type & TYPE_CHAR) printf("char ");
            if (TypeNode->Type & TYPE_SHORT) printf("short ");
            if (TypeNode->Type & TYPE_INT) printf("int ");
            if (TypeNode->Type & TYPE_LONG) printf("long ");
            if (TypeNode->Type & TYPE_LONGLONG) printf("long");
            if (TypeNode->Type & TYPE_SIGNED) printf("signed ");
            if (TypeNode->Type & TYPE_UNSIGNED) printf("unsigned ");
            if (TypeNode->Type & TYPE_FLOAT) printf("float ");
            if (TypeNode->Type & TYPE_DOUBLE) printf("double ");
            if (TypeNode->Type & TYPE_COMPLEX) printf("_Complex ");
        } break;

        case CST_BinaryOperator:
        {
            cst_node_binary_operator* Op = (cst_node_binary_operator*)Node;
            printf("(");
            WalkTree(Op->Left, Tab + 1, true);
            printf(" ");
            printf(TokenType_Str(Op->Operation));
            printf(" ");
            WalkTree(Op->Right, Tab + 1, true);
            printf(")");
            if (!Inline) { printf("\n"); }
        } break;

        case CST_UnaryOperator:
        {
            cst_node_unary_operator* Op = (cst_node_unary_operator*)Node;
            printf("(");
            if (Op->Affix == PREFIX) {
                printf(TokenType_Str(Op->Operation));
                printf(" ");
            }
            WalkTree(Op->Operand, Tab + 1, true);
            if (Op->Affix == POSTFIX) {
                printf(" ");
                printf(TokenType_Str(Op->Operation));
            }
            printf(")");
            if (!Inline) { printf("\n"); }
        } break;

        case CST_Conditional:
        {
            cst_node_conditional* Cond = (cst_node_conditional*)Node;
            printf("if (");
            WalkTree(Cond->Condition, Tab + 1, true);
            printf(")");
            if (!Inline) printf("\n");
            WalkTree(Cond->TrueBranch, Tab + 1, Inline);
            if (Cond->FalseBranch != NULL) {
                if (!Inline) for (int i = 0; i < Tab; ++i) printf("  ");
                printf("else");
                if (!Inline) printf("\n");
                WalkTree(Cond->FalseBranch, Tab + 1, Inline);
            }
        } break;

        case CST_Jump:
        {
            cst_node_jump* Jump = (cst_node_jump*)Node;
            switch(Jump->Type) {
                case CONTINUE: printf("continue");
                case BREAK:    printf("break");
                case RETURN:   printf("return");
            }
            if (Jump->Expression) {
                printf(" ");
                WalkTree(Jump->Expression, Tab + 1, true);
            }
            if (!Inline) printf("\n");
        } break;

        case CST_Assignment:
        {
            cst_node_assignment* Assignment = (cst_node_assignment*)Node;

            WalkTree(Assignment->LValue, Tab + 1, true);
            switch(Assignment->Operator) {
                case Assign: printf(" = "); break;
                case TimesEquals: printf(" *= "); break;
                case DivideEquals: printf(" /= "); break;
                case ModulusEquals: printf(" %%= "); break;
                case PlusEquals: printf(" += "); break;
                case MinusEquals: printf(" -= "); break;
                case LBitShiftEquals: printf(" <<= "); break;
                case RBitShiftEquals: printf(" >>= "); break;
                case BitAndEquals: printf(" &= "); break;
                case BitXorEquals: printf(" ^= "); break;
                case BitOrEquals: printf(" |= "); break;
            }
            WalkTree(Assignment->RValue, Tab + 1, true);
            if (!Inline) printf("\n");
        } break;

        default:
            printf("unhandled node %s", NodeNameStr((cf_symbol_t)*(cst_node_type*)Node));
            break;
    }
}

int main(int ArgCount, char* ArgValues[])
{
    if (ArgCount < 2) {
        Usage(ArgValues[0]);
        return 0;
    }
    
    char* Filename = ArgValues[1];
    char* File = ReadFileAndNullTerminate(Filename);
    if (File == NULL) {
        fprintf(stderr, "Couldn't read file %s, exiting.\n", Filename);
    }

    tokenizer Tzer = {0};
    Tzer.Start = File;
    Tzer.LineStart = File;
    Tzer.At = File;
    Tzer.Line = 1;
    Tzer.Filename = Filename;

#if 0
    token Token;
    do {
        Token = GetToken(&Tzer);
        if (Token.TextLength == 0) {
            printf("Got token %s at line %d\n", TokenType_Str(Token.Type), Token.LineStart);
        } else {
            printf("Got token %s(%.*s) at line %d\n", TokenType_Str(Token.Type), Token.TextLength, Token.Text, Token.LineStart);
        }
    } while (Token.Type != EndOfFile);
#else
    void* Result = Parse(&Tzer);
    if (Result != NULL) {
        WalkTree(Result, 0, false);
        printf("\n");
    }
#endif
        printf("Parsing complete\n");
}

















/*
    OLD OLD OLD

    struct point_array
    {
        point* Elements;
        size_t Size;
        size_t Capacity;
    }

    === Declaration
    point[?] Points;
     ~~>
    point_array Points;

    === Assignment to end
    Points[>] = Point{2, 2};
     ~~>
    Point P = {2, 2};
    if (Size + 1 >= Capacity) { Expand(Points); }
    Elements[Size] = P;

    === Assignment to beginning
    Points[<] = Point{2, 2};
     ~~>
    Point P = {2, 2};
    if (Size + 1 >= Capacity) { Expand(Points); }
    Copy(Points[0], Size, Points[1], Size);
    Points[0] = P;

    === Sub-array (by copy) (Python-ish syntax)
    point[?] PtsMid = Points[2:6]; // Gives a copy with the same values as elements [2, 3, 4, 5, 6]
    point[?] PtsEnd = Points[2:]; // Gives a copy with the same values as elements [2, 3, 4, ..., Size]
    point[?] PtsSrt = Points[:6]; // Gives a copy with the asme values as elements [0, 1, 2, 3, ... 6]
    point[?] NewPts = Points[:]; // Returns an exact copy.
    

    
    ==========
    Type need-to-knows

     + What type am I assigning to this? (Know the type of any given symbol)
     - What are the intermediate types and how do they convert?
     - 
*/
