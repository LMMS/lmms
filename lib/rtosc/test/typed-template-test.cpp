//#include <rtosc/typed-message.h>
#include "../include/rtosc/typed-message.h"
#include <cstdio>

using rtosc::rtMsg;
using rtosc::get;

int main() {
    char buf[1024];
    char buf2[1024];
    rtosc_message(buf, 1024, "/send-to", "si", "Testing", 42);
    rtosc_message(buf2, 1024, "/send-to", "iff", 12, 3.15, 81.0);
    printf("Starting...\n");
    rtMsg<const char *, int32_t> msg{buf};
    auto s = get<0>(msg);
    printf("s=%s\n", s);

    if(rtMsg<const char *, int32_t> m{buf+1, "send-to"})
        printf("PASS\n");
    else
        printf("FAIL\n");

    if(rtMsg<const char *> m{buf})
        printf("FAIL\n");
    else
        printf("PASS\n");

    if(rtMsg<int32_t, int32_t> m{buf})
        printf("FAIL\n");
    else
        printf("PASS\n");

    if(rtMsg<int32_t, float, float> m{buf2})
        printf("PASS\n");
    else
        printf("FAIL\n");

    if(rtMsg<int32_t, const char*> m{buf}) {
        printf("FAIL\n");
        printf("%s\n", get<1>(m));
    } else
        printf("PASS\n");

    return 0;
};
