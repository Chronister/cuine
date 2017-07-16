

int identity(int x) { return x; }
int foo(int x) { return 2*x; }
int bar(int x) { return x + 1; }

enum foobar { FOO, BAR };

typedef int (*fooptr_t)(int);
typedef fooptr_t (*foomaker_t)(enum foobar);

// fooptr_t foomaker(enum foobar kind)
int (*foomaker(enum foobar kind))(int) 
{
    if (kind == FOO) return foo;
    if (kind == BAR) return bar;
    return identity;
};

//foomaker_t foomakermaker()
int (*(*foomakermaker())(enum foobar))(int) { return foomaker; }

int main(int argc, char* argv[]) {
    return foomakermaker()(BAR)(17);
}
