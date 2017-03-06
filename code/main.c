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
    } while (Token.Type != TOKEN_EOF);
#else
    void* Result = Parse(&Tzer);
#if 0
    if (Result != NULL) {
        WalkTree(Result, 0);
        printf("\n");
    }
#endif
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
