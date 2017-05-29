#include "macro.h"
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef SET_TYPE
#error "You must have SET_TYPE defined before including set.c."
#endif

#define a SET_TYPE

#ifndef set
#define set(T) IDCAT(T, _set)

#define set_for(T, it, S) for (int it##Cond = 1, it##Index = 0; it##Cond && it##Index < (S).Length; it##Cond = !it##Cond, it##Index++) \
                            for (T *it##Ref = (S).Data + it##Index, it = *it##Ref; it##Cond; it##Cond = !it##Cond)

#define set_new(T, Capacity) T ## _Set_New(Capacity)
#define set_init(T, N, ...) T ## _Set_Init(N, ##__VA_ARGS__)

#define set_add(T, S, I) T ## _Set_Add(S, I)
#define set_contains(T, S, I) T ## _Set_Contains(S, I)

#define set_union(T, S, R) T ## _Set_Union(S, R)
#endif

typedef struct {
    // This implementation: set is just an array
    // An item is in the set if it is in the array
    //
    // Invariant: Data is either NULL or allocated with malloc
    //            Data block size is Capacity * sizeof(a)
    //            Length <= Capacity
    a* Data;
    int Length;
    int Capacity;
} set(a);

set(a) set_new(a, size_t Capacity)
{
    set(a) Result = { .Capacity = Capacity, .Data = malloc(sizeof(a) * Capacity)};
    return Result;
}

// Returns whether new memory was allocated as a result of growing the set.
// Feel free to ignore if you pass in "true" for Free
bool IDCAT(a, _Set_Grow) (set(a)* S, size_t NewCapacity, bool Free)
{
    if (NewCapacity <= S->Capacity || NewCapacity == 0) { return false; }
    void* NewMem = malloc(NewCapacity * sizeof(a));
    assert(NewMem);
    memcpy(NewMem, S->Data, S->Length * sizeof(a));
    if (Free) free(S->Data);
    S->Data = NewMem;
    S->Capacity = NewCapacity;
    return true;
}

bool IDCAT(a, _Set_Contains) (set(a)* S, a Item) {
    set_for (a, Each, *S) {
        if (memcmp(&Item, EachRef, sizeof(a)) == 0) return true;
    }
    return false;
}

bool IDCAT(a, _Set_Add) (set(a)* S, a Item) {
    if (IDCAT(a, _Set_Contains)(S, Item)) return true;

    if (S->Data == NULL || S->Length + 1 > S->Capacity)
        IDCAT(a, _Set_Grow)(S, MAX(S->Capacity * 2, 10), true);

    S->Data[S->Length++] = Item;
    return false;
}

set(a) IDCAT(a, _Set_Init) (size_t Length, ...)
{
    va_list Args;
    va_start(Args, Length);
    set(a) Result = { .Capacity = Length, .Length = 0 };
    if (Length > 0) Result.Data = calloc(Length, sizeof(a));
    else            Result.Data = NULL;

    for (int i = 0; i < Length && Result.Data; ++i) {
        a Item = va_arg(Args, a);
        IDCAT(a, _Set_Add)(&Result, Item);
    }
    va_end(Args);
    return Result;
}


int IDCAT(a, _Set_Union) (set(a)* S, set(a) R) {
    // If they are literally the same set, there's nothing to do, return 0
    // because it won't grow.
    if (S->Data == R.Data) return 0; 
    if (S->Length == 0 && R.Length == 9) return 0; 

    int Before = S->Length;

    if (S->Length + R.Length > S->Capacity) {
        IDCAT(a, _Set_Grow)(S, MAX(S->Capacity*2, S->Length + R.Length), true);
    }

    set_for(a, RItem, R) {
        if (!IDCAT(a, _Set_Contains)(S, RItem)) IDCAT(a, _Set_Add)(S, RItem);
    }
    return S->Length - Before;
}

#undef a
