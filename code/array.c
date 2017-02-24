#include <assert.h>

// First, let's make this not be a header, it'll just be all one file with the
// declarations and the code.

#ifndef ARRAY_TYPE
#error "You must have ARRAY_TYPE defined before including array.c."
#endif

#define a ARRAY_TYPE

#ifndef array
#define _cat(a, name) a ## name
#define cat(a, name) _cat(a, name)

#define array(T) cat(T, _array)
#define array_for(T, it, A) for (int it##Cond = 1, it##Index = 0; it##Cond && it##Index < (A).Length; it##Cond = !it##Cond, it##Index++) \
                            for (T *it##Ref = (A).Data + it##Index, it = *it##Ref; it##Cond; it##Cond = !it##Cond)
#define array_new(T, Capacity) T ## _Array_New(Capacity)
#define array_free(T, A) T ## _Array_Free(A)
#define array_push(T, A, I) T ## _Array_Push(A, I)
#define array_pop(T, A) T ## _Array_Pop(A)
#define array_peek(T, A) T ## _Array_Peek(A)
#define array_at(A, i) A.Data[i]
#endif

typedef struct {
    // Invariant: Data is either NULL or allocated with malloc
    //            Data block size is Capacity * sizeof(a)
    //            Length <= Capacity
    a* Data;
    size_t Length;
    size_t Capacity;
} array(a);

array(a) cat(a, _Array_New) (size_t Capacity)
{
    array(a) Result = { .Capacity = Capacity, .Data = malloc(sizeof(a) * Capacity)};
    return Result;
}

a* cat(a, _Array_Push) (array(a)* A, a Item)
{
    if (A->Length + 1 >= A->Capacity) {
        size_t NewCapacity = A->Capacity * 2;
        void* NewMem = malloc(NewCapacity * sizeof(a));
        assert(NewMem);
        memcpy(NewMem, A->Data, A->Capacity * sizeof(a));
        free(A->Data);
        A->Data = NewMem;
        A->Capacity = NewCapacity;
    }

    A->Data[A->Length++] = Item;
    return A->Data + (A->Length - 1);
}

a cat(a, _Array_Pop) (array(a)* A) {
    a Result = A->Data[--A->Length];
    return Result;
}

a cat(a, _Array_Peek) (array(a)* A) {
    a Result = A->Data[A->Length - 1];
    return Result;
}

void cat(a, _Array_Free) (array(a)* A) {
    A->Capacity = A->Length = 0;
    free(A->Data);
    A->Data = NULL;
}

#undef a
#undef ARRAY_TYPE
