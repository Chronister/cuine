static int bar = 3, baz;

static const char* quux = "quux";

static volatile const int * const * const volatile ** restrict volatile *foo;

typedef int int32_t;

long long foo(int bar);

int main(int argc, char* argv) {
    register int x = 5, z = 2;
    auto int y = 10;
    volatile float test = 3.14159f;
    const double test2 = 4e-31;
    long double test3 = 9E99;
    double q = test1 + test3;
    y += x % y;
    int b = x ^ y & z * z | 37;
    if (x + y > 16) {
        int z = 7;
        x = z - y;
    } else x = 2;
    return x;
}
