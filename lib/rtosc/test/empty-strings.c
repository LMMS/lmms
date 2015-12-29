#include <rtosc/rtosc.h>
#include <stdio.h>
#include <stdlib.h>

char buffer[1024];

//verify that empty strings can be retreived
int main()
{
    size_t length = rtosc_message(buffer, 1024, "/path", "sss", "", "", "");
    // /pat h000 ,sss 0000 0000 0000 0000
    if(length != 28)
    {
        fprintf(stderr, "Bad length for empty strings...");
        exit(1);
    }
    if(!rtosc_argument(buffer, 0).s ||
            !rtosc_argument(buffer, 1).s ||
            !rtosc_argument(buffer, 2).s)
    {
        fprintf(stderr, "Failure to retreive string pointer...");
        exit(1);
    }
    printf("%p %p %p\n",
            rtosc_argument(buffer, 0).s,
            rtosc_argument(buffer, 1).s,
            rtosc_argument(buffer, 2).s);
    return 0;
}
