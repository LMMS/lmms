#include <rtosc/rtosc.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t midi_t[4];
char buffer[1024];
int err = 0;
#define CHECK(x) \
    if(!(x)) {\
        fprintf(stderr, "failure at line %d (" #x ")\n", __LINE__); \
        ++err;}

//verifies a message with all types included serializes and deserializes
int main()
{
    unsigned message_len;
    int32_t      i = 42;             //integer
    float        f = 0.25;           //float
    const char  *s = "string";       //string
    rtosc_blob_t b = {3,(uint8_t*)s};//blob
    int64_t      h = -125;           //long integer
    uint64_t     t = 22412;          //timetag
    double       d = 0.125;          //double
    const char  *S = "Symbol";       //symbol
    char         c = 25;             //character
    int32_t      r = 0x12345678;     //RGBA
    midi_t       m = {0x12,0x23,     //midi
                      0x34,0x45};
    //true
    //false
    //nil
    //inf

    CHECK(message_len = rtosc_message(buffer, 1024, "/dest",
                "[ifsbhtdScrmTFNI]",
                i,f,s,b.len,b.data,h,t,d,S,c,r,m));

    CHECK(rtosc_argument(buffer, 0).i == i);
    CHECK(rtosc_argument(buffer, 1).f == f);
    CHECK(!strcmp(rtosc_argument(buffer, 2).s,s));
    CHECK(rtosc_argument(buffer, 3).b.len == 3);
    CHECK(!strcmp((char*)rtosc_argument(buffer, 3).b.data, "str"));
    CHECK(rtosc_argument(buffer, 4).h == h);
    CHECK(rtosc_argument(buffer, 5).t == t);
    CHECK(rtosc_argument(buffer, 6).d == d);
    CHECK(!strcmp(rtosc_argument(buffer, 7).s,S));
    CHECK(rtosc_argument(buffer, 8).i == c);
    CHECK(rtosc_argument(buffer, 9).i == r);
    CHECK(rtosc_argument(buffer, 10).m[0] == m[0])
    CHECK(rtosc_argument(buffer, 10).m[1] == m[1])
    CHECK(rtosc_argument(buffer, 10).m[2] == m[2])
    CHECK(rtosc_argument(buffer, 10).m[3] == m[3])
    CHECK(rtosc_type(buffer,11) == 'T');
    CHECK(rtosc_type(buffer,12) == 'F');
    CHECK(rtosc_type(buffer,13) == 'N');
    CHECK(rtosc_type(buffer,14) == 'I');
    CHECK(rtosc_valid_message_p(buffer, message_len));

    return err;
}
