#include "macro.h"
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef ARRAY_TYPE
#error "You must have ARRAY_TYPE defined before including array.c."
#endif

#define a ARRAY_TYPE

#ifndef array
#define array(T) IDCAT(T, _array)

#define array_at(A, i) A.Data[i]
#define array_for(T, it, A) for (int it##Cond = 1, it##Index = 0; it##Cond && it##Index < (A).Length; it##Cond = !it##Cond, it##Index++) \
                            for (T *it##Ref = (A).Data + it##Index, it = *it##Ref; it##Cond; it##Cond = !it##Cond)

#define array_new(T, Capacity) T ## _Array_New(Capacity)
// NOTE DO NOT TRY TO PUSH TO A WRAPPED ARRAY
#define array_wrap(T, n, data) T ## _Array_Wrap(n, data)
#define array_init(T, n, ...) T ## _Array_Init(n, __VA_ARGS__)
#define array_free(T, A) T ## _Array_Free(A)
#define array_grow(T, A, NewCapacity) T ## _Array_Grow(A, NewCapacity)

#define array_push(T, A, I) T ## _Array_Push(A, I)
#define array_push_all(T, A, B) T ## _Array_PushAll(A, B)
#define array_pop(T, A) T ## _Array_Pop(A)
#define array_peek(T, A) T ## _Array_Peek(A)

#endif

typedef struct {
    // Invariant: Data is either NULL or allocated with malloc
    //            Data block size is Capacity * sizeof(a)
    //            Length <= Capacity
    a* Data;
    int Length;
    int Capacity;
} array(a);

array(a) IDCAT(a, _Array_New) (size_t Capacity)
{
    array(a) Result = { .Capacity = Capacity, .Data = malloc(sizeof(a) * Capacity)};
    return Result;
}

// TODO someone could try to push to this and all hell might break loose
// add a readonly flag to the array?
array(a) IDCAT(a, _Array_Wrap) (size_t Length, a* Items)
{
    array(a) Result = { .Capacity = Length, .Length = Length, .Data = Items };
    return Result;
}

array(a) IDCAT(a, _Array_Init) (size_t Length, ...)
{
    va_list Args;
    va_start(Args, Length);
    array(a) Result = { .Capacity = Length, .Length = Length};
    if (Length > 0) Result.Data = malloc(sizeof(a) * Length);
    else            Result.Data = NULL;
    array_for(a, Item, Result) {
        *ItemRef = va_arg(Args, a);
    }
    return Result;
}

void IDCAT(a, _Array_Grow) (array(a)* A, size_t NewCapacity)
{
    if (NewCapacity <= A->Capacity) { return; }
    void* NewMem = malloc(NewCapacity * sizeof(a));
    assert(NewMem);
    memcpy(NewMem, A->Data, A->Length * sizeof(a));
    free(A->Data);
    A->Data = NewMem;
    A->Capacity = NewCapacity;
}

a* IDCAT(a, _Array_Push) (array(a)* A, a Item)
{
    if (A->Data == NULL || A->Length + 1 >= A->Capacity)
        IDCAT(a, _Array_Grow)(A, MAX(A->Capacity * 2, 10));

    A->Data[A->Length++] = Item;
    return A->Data + (A->Length - 1);
}

a* IDCAT(a, _Array_PushAll) (array(a)* A, array(a) B)
{
    if (A->Data == NULL || A->Length + B.Length >= A->Capacity)
        IDCAT(a, _Array_Grow)(A, MAX(A->Capacity * 2, A->Length + B.Length));

    int Length = A->Length;
    array_for(a, Item, B) {
        A->Data[Length++] = Item;
    }
    A->Length = Length;
    return A->Data + Length;
}

a IDCAT(a, _Array_Pop) (array(a)* A) {
    a Result = A->Data[--A->Length];
    return Result;
}

a IDCAT(a, _Array_Peek) (array(a)* A) {
    a Result = A->Data[A->Length - 1];
    return Result;
}

void IDCAT(a, _Array_Free) (array(a)* A) {
    A->Capacity = A->Length = 0;
    free(A->Data);
    A->Data = NULL;
}

#undef a
#undef ARRAY_TYPE
