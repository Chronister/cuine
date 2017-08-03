static int bar = 3, baz;

static const char* quux = "quux";

static volatile const int * const * const volatile ** restrict volatile *ptr;

typedef int int32_t;

union vec2 {
    struct {
        int x, y;
    } pt;
    struct {
        int w, h;
    } sz;
};

struct foo_props {
    bool isBar;
    volatile float bazAmount;

    const int (*do_baz)(union vec2);
};

//typedef long long (*foo_t)(int);
//foo_t foos[3];
long long foo(int bar), foo2(float baz);

int main(int argc, char* argv) {
    register int x = 5, z = 2;
    auto int y = 10;
    volatile float test = 3.14159f;
    double test2 = 4e-31;
    long double test3 = 9E99;
    double q = test + test3;
    y += x % y;
    int b = x ^ y & z * z | 37;
    if (x + y > 16) {
        int z = 7;
        x = z - y;
        ++x;
        z = --y + sizeof test +++test2 --- --test3;
    } else x = 2;
    foos[1] = foo;
    const int* p = &y;
    int r = *p;
    return x;
}
