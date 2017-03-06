#ifndef MACRO_H
#define MACRO_H

#define EXPAND(a) a
#define IDCAT(a, name) EXPAND(a)##name

#define STR(x) #x

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define countof(x) ((sizeof((x)))/sizeof((x)[0]))

#endif
