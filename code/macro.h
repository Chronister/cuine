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

#endif
