//enum foobar { FOO, BAR };

typedef int (*fooptr_t)(int);
typedef fooptr_t (*foomaker_t)(enum foobar);

int identity(int x) { return x; }
int foo(int x) { return 2*x; }
int bar(int x) { return x + 1; }

// fooptr_t foomaker(enum foobar kind)
int (*foomaker(enum foobar kind))(int) 
{
    if (kind == FOO) return foo;
    if (kind == BAR) return bar;
    return identity;
}

//foomaker_t foomakermaker()
static int (*(*foomakermaker())(enum foobar))(int) { return foomaker; }

/* The above function should produce an AST that looks something like this:
 
┌[declaration]──────────────────────────────────┐
├ Name: foomakermaker                           │ 
├ Specifiers: static                            │
├ Base type:                                    │
│ ┌[function_type]────────────────────────────┐ │
│ ├ Name: foomakermaker                       │ │
│ ├ Arguments: []                             │ │
│ ├ Return type:                              │ │
│ │ ┌[function_type]────────────────────────┐ │ │
│ │ ├ Name:                                 │ │ │
│ │ ├ Arguments: [declaration: enum foobar] │ │ │
│ │ ├ Return type:                          │ │ │
│ │ │ ┌[function_type]────────────────┐     │ │ │
│ │ │ ├ Name:                         │     │ │ │
│ │ │ ├ Arguments: [declaration: int] │     │ │ │
│ │ │ ├ Return type: int              │     │ │ │
│ │ │ └───────────────────────────────┘     │ │ │
│ │ └───────────────────────────────────────┘ │ │
│ └───────────────────────────────────────────┘ │
└───────────────────────────────────────────────┘
*/

int main(int argc, char* argv[]) {
    return foomakermaker()(BAR)(17);
}
