
static int bar = 3, baz;

static const char* quux = "quux";

static volatile const int * const * const volatile ** restrict volatile *foo;

typedef int int32_t;

long long foo(int bar);

int main(int argc, char* argv) {
    register int x = 5;
    auto int y = 10;
    if (x + y > 16) {
        int z = 7;
        x = z - y;
    } else x = 2;
    return x;
}
