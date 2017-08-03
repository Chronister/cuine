#ifndef MACRO_H
#define MACRO_H

#define _IDCAT(a, name) a ##name
#define IDCAT(a, name) _IDCAT(a, name)

#define STR(x) #x

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define FLAG(n) ((1) << (n))

#define countof(x) ((sizeof((x)))/sizeof((x)[0]))

#define else_invalid else assert(!"Invalid default case!")
#define default_invalid default: assert(!"Invalid default case!"); break

#define cast(T, x) ((T)(x))

#if defined(__GNUC__)
    #define always_inline __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define always_inline inline
#endif

// Variadic macros that select a function based on the number of arguments
// Thanks to this stackoverflow answer:
// https://stackoverflow.com/a/24028231
#define GLUE(x, y) x y

#define RETURN_ARG_COUNT(_1_, _2_, _3_, _4_, _5_, count, ...) count
#define EXPAND_ARGS(args) RETURN_ARG_COUNT args
#define COUNT_ARGS_MAX5(...) EXPAND_ARGS((__VA_ARGS__, 5, 4, 3, 2, 1, 0))

#define OVERLOAD_MACRO2(name, count) name##count
#define OVERLOAD_MACRO1(name, count) OVERLOAD_MACRO2(name, count)
#define OVERLOAD_MACRO(name, count) OVERLOAD_MACRO1(name, count)

#define CALL_OVERLOAD(name, ...) GLUE(OVERLOAD_MACRO(name, COUNT_ARGS_MAX5(__VA_ARGS__)), (__VA_ARGS__))

#endif
