#include <stdlib.h>
#include <rtosc/rtosc.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

const char paths[10][32] = {
    "/",
    "#24:",
    "#20:ff",
    "path",
    "path#234/:ff",
    "path#1asdf",
    "foobar#123/::ff",
    "blam/",
    "bfn::c",
    "bfnpar1::c"
};

int error = 0;

#define work(col, val, name, ...) rtosc_message(buffer, 256, name, __VA_ARGS__); \
    for(int i=0; i<10; ++i) {\
        int matches = rtosc_match(paths[i], buffer); \
        if(((col == i) && (matches != val))) {\
            printf("Failure to match '%s' to '%s'\n", name, paths[i]); \
            error = 1; \
        } else if((col != i) && matches) { \
            printf("False positive match on '%s' to '%s'\n", name, paths[i]); \
            error = 1; \
        } \
    }

int main()
{
    char buffer[256];
    work(0,1,"/",          "");
    work(1,1,"19",         "");
    work(2,1,"14",         "ff", 1.0,2.0);
    work(3,0,"pat",        "");
    work(3,1,"path",       "");
    work(3,0,"paths",      "");
    work(4,1,"path123/",   "ff", 1.0, 2.0);
    work(5,1,"path0asdf",  "");
    work(6,1,"foobar23/",  "");
    work(6,0,"foobar123/", "");
    work(6,1,"foobar122/", "");
    work(7,1,"blam/",      "");
    work(7,1,"blam/blam",  "");
    work(7,0,"blam",       "");
    work(9,1,"bfnpar1",    "c");
    work(9,1,"bfnpar1",    "");

    return error;
}
