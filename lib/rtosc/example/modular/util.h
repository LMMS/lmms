#include <functional>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <rtosc/rtosc.h>
#include <rtosc/ports.h>

static const char *
snip(const char *m)
__attribute__((unused));
static const char *
snip(const char *m)
{
    while(*m && *m!='/')++m;
    return *m?m+1:m;
}

using rtosc::RtData;

//floating point parameter
#define PARAM(type, var, name, scale, _min, _max, desc) \
{#name":f", #scale "," # _min "," #_max ":'parameter'" desc, 0, \
    [](const char *m, RtData &d) {((type*)d.obj)->var = rtosc_argument(m,0).f;}}

//optional subclass
#define OPTION(type, cast, name, var) \
{#name "/", ":'option':", &cast ::ports, \
    [](const char *m, RtData &d) { \
        cast *c = dynamic_cast<cast*>(((type*)d.obj)->var); \
        if(c){ d.obj=c; cast::ports.dispatch(snip(m),d);}}}

//Dummy - a placeholder port
#define DUMMY(name) \
{#name, ":'dummy':", 0, [](const char *, RtData&){}}

//Recur - perform a simple recursion
#define RECUR(type, cast, name, var) \
{#name"/", ":'recursion':", &cast::ports, [](const char *m, RtData &d){\
    d.obj = &(((type*)d.obj)->var); \
    cast::ports.dispatch(snip(m), d);}}

//Recurs - perform a ranged recursion
#define RECURS(type, cast, name, var, length) \
{#name "#" #length "/", ":'recursion':", &cast::ports, [](const char *m, RtData &d){ \
    const char *mm = m; \
    while(!isdigit(*mm))++mm; \
    d.obj = &(((type*)d.obj)->var)[atoi(mm)]; \
    cast::ports.dispatch(snip(m), d);}}
