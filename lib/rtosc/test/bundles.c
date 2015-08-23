#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtosc/rtosc.h>

char buffer_a[256];
char buffer_b[256];
char buffer_c[256];

size_t len_a;
size_t len_b;
size_t len_c;


void check(int b, const char *msg, int loc)
{
    if(!b) {
        fprintf(stderr, "%s:%d\n", msg, loc);
        exit(1);
    }
}

#define MSG1 "/fly" "ing-" "monk" "ey\0\0" ",s\0\0" "bann" "ana\0"
#define MSG2 "/foo" "bar-" "mess" "age\0" ",iT\0" "\0\0\0\x2a"

               /*bundle          timetag*/
#define RESULT "#bun" "dle\0" "\0\0\0\0" "\0\0\0\0" \
               /*size1             size2*/ \
               "\0\0\0\x1c" MSG1 "\0\0\0\x18" MSG2

//TODO nested bundles (though I swore those were already done)
int main()
{
    printf("%d %d %d\n", (int)sizeof(MSG1)-1, (int)sizeof(MSG2)-1, (int)sizeof(RESULT)-1);
    check((len_a = rtosc_message(buffer_a, 256,
                    "/flying-monkey", "s", "bannana")) == sizeof(MSG1)-1,
            "bad message", __LINE__);
    check((len_b = rtosc_message(buffer_b, 256,
                    "/foobar-message", "iT", 42)) == sizeof(MSG2)-1,
            "bad message", __LINE__);
    check(!rtosc_bundle_p(buffer_a),
            "False positive bundle_p()", __LINE__);
    check(!rtosc_bundle_p(buffer_b),
            "False positive bundle_p()", __LINE__);
    len_c = rtosc_bundle(buffer_c, 256, 0, 2, buffer_a, buffer_b);
        printf("len_c => '%d'\n correct is %d\n", (int)len_c, (int)sizeof(RESULT)-1);
    check((len_c = rtosc_bundle(buffer_c, 256, 0, 2, buffer_a, buffer_b)) == sizeof(RESULT)-1,
            "bad bundle", __LINE__);
    check(rtosc_message_length(buffer_c, len_c) == len_c,
            "bad message length", __LINE__);

    check(rtosc_bundle_p(buffer_c),
            "Bad bundle detection", __LINE__);
    check(rtosc_bundle_elements(buffer_c, 256)==2,
            "Bad bundle_elements length", __LINE__);
    check(!strcmp("/flying-monkey", rtosc_bundle_fetch(buffer_c, 0)),
            "Bad bundle_fetch", __LINE__);
    check(!strcmp("/foobar-message", rtosc_bundle_fetch(buffer_c, 1)),
            "Bad bundle_fetch", __LINE__);

    //Check minimum bundle size #bundle + time tag
    check(rtosc_bundle(buffer_c, 256, 1, 0) == (8+8),
            "Bad minimum bundle length", __LINE__);

    //check message length support
    check(rtosc_message_length(buffer_c, 256) == (8+8),
            "Bad message length", __LINE__);

    //Verify that timetag can be fetched
    check(rtosc_bundle_timetag(buffer_c)==1,
            "Bad timetag", __LINE__);

    return 0;
}
