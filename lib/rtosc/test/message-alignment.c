#include <rtosc/rtosc.h>
#include <stdio.h>
#include <string.h>
char buffer[1024];
int err = 0;
#define CHECK(x) \
    if(!(x)) {\
        fprintf(stderr, "failure at line %d (" #x ")\n", __LINE__); \
        ++err;}

int main()
{
    int32_t      i = 42;             //integer
    float        f = 0.25;           //float
    const char  *s = "string";       //string
    size_t message_size = 32;
    CHECK(rtosc_message(buffer, 1024, "/dest",
                "ifs", i,f,s) == message_size);
    CHECK(rtosc_argument(buffer, 0).i == i);
    CHECK(rtosc_argument(buffer, 1).f == f);
    CHECK(!strcmp(rtosc_argument(buffer, 2).s, s));
    CHECK(rtosc_message_length(buffer,1024) == 32);

    memmove(buffer+1, buffer, message_size);
    CHECK(!strcmp(buffer+1, "/dest"));
    CHECK(rtosc_argument(buffer+1, 0).i == i);
    CHECK(rtosc_argument(buffer+1, 1).f == f);
    CHECK(!strcmp(rtosc_argument(buffer+1, 2).s, s));
    CHECK(rtosc_message_length(buffer+1,1024-1) == 32);

    memmove(buffer+2, buffer+1, message_size);
    CHECK(!strcmp(buffer+2, "/dest"));
    CHECK(rtosc_argument(buffer+2, 0).i == i);
    CHECK(rtosc_argument(buffer+2, 1).f == f);
    CHECK(!strcmp(rtosc_argument(buffer+2, 2).s, s));
    CHECK(rtosc_message_length(buffer+2,1024-2) == 32);

    memmove(buffer+3, buffer+2, message_size);
    CHECK(!strcmp(buffer+3, "/dest"));
    CHECK(rtosc_argument(buffer+3, 0).i == i);
    CHECK(rtosc_argument(buffer+3, 1).f == f);
    CHECK(!strcmp(rtosc_argument(buffer+3, 2).s, s));
    CHECK(rtosc_message_length(buffer+3,1024-3) == 32);

    memmove(buffer+4, buffer+3, message_size);
    CHECK(!strcmp(buffer+4, "/dest"));
    CHECK(rtosc_argument(buffer+4, 0).i == i);
    CHECK(rtosc_argument(buffer+4, 1).f == f);
    CHECK(!strcmp(rtosc_argument(buffer+4, 2).s, s));
    CHECK(rtosc_message_length(buffer+4,1024-4) == 32);

    for(int j=0; j<4; ++j) {
        fprintf(stderr, "offset %d\n", j);
        CHECK(rtosc_argument(buffer+4+j, 0).i == i);
        CHECK(rtosc_argument(buffer+4+j, 1).f == f);
        CHECK(!strcmp(rtosc_argument(buffer+4+j, 2).s, s));
        CHECK(rtosc_message_length(buffer+4+j,1024-4-j) == (unsigned)(32-j));
    }

    return err;
}
