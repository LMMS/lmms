#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtosc/rtosc.h>

void check(int b, const char *msg)
{
    if(!b) {
        fprintf(stderr, "%s\n", msg);
        exit(1);
    }
}

int main()
{
    //Verify that given a null buffer, it does not segfault
    check(rtosc_message(0,0,"/page/poge","TIF") == 20,
            "Invalid message length");
    return 0;
}
