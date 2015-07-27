#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtosc/rtosc.h>

char buffer_a[256];
char buffer_b[256];
char buffer_c[256];
char buffer_d[256];
char buffer_e[256];

int err = 0;
void check(int b, const char *msg, int loc)
{
    if(!b) {
        fprintf(stderr, "%s:%d\n", msg, loc);
        err = 1;
    }
}

#define VERB_BUNDLE \
good &= (*msg++ == 'b'); \
good &= (*msg++ == 'u'); \
good &= (*msg++ == 'n'); \
good &= (*msg++ == 'd'); \
good &= (*msg++ == 'l'); \
good &= (*msg++ == 'e');

#define POUND_BUNDLE \
good = 1; \
good &= (*msg++ == '#'); \
VERB_BUNDLE \
good &= (*msg++ == '\0'); \
check(good, "bad bundle start", __LINE__);

#define TIMETAG msg += 8;

#define SKIP_SIZE msg += 4;

#define MSG_A \
good = 1; \
good &= (*msg++ == '/'); \
VERB_BUNDLE \
good &= (*msg++ == '\0'); \
\
good &= (*msg++ == ','); \
good &= (*msg++ == 's'); \
good &= (*msg++ == '\0'); \
good &= (*msg++ == '\0'); \
\
VERB_BUNDLE \
good &= (*msg++ == '\0'); \
good &= (*msg++ == '\0'); \
check(good, "bad message 'A'", __LINE__);

#define MSG_B \
good = 1; \
good &= (*msg++ == '/'); \
VERB_BUNDLE \
good &= (*msg++ == '-'); \
VERB_BUNDLE \
good &= (*msg++ == '\0'); \
good &= (*msg++ == '\0'); \
\
good &= (*msg++ == ','); \
good &= (*msg++ == 's'); \
good &= (*msg++ == 's'); \
good &= (*msg++ == '\0'); \
\
VERB_BUNDLE \
good &= (*msg++ == '\0'); \
good &= (*msg++ == '\0'); \
VERB_BUNDLE \
good &= (*msg++ == '\0'); \
good &= (*msg++ == '\0'); \
check(good, "bad message 'B'", __LINE__);

#define BUNDLE_C \
POUND_BUNDLE \
TIMETAG \
SKIP_SIZE \
MSG_A \
SKIP_SIZE \
MSG_A


int main()
{
    check(rtosc_message(buffer_a, 256, "/bundle", "s", "bundle"),
            "bad message", __LINE__);
    check(rtosc_message(buffer_b, 256, "/bundle-bundle", "ss", "bundle", "bundle"),
            "bad message", __LINE__);
    int len;
    check(rtosc_bundle(buffer_c, 256, 0, 2, buffer_a, buffer_a) == 64,
            "bad bundle", __LINE__);
    check(rtosc_message_length(buffer_c, 256) == 64,
            "bad bundle length", __LINE__);
    check((len = rtosc_bundle(buffer_d, 256, 0, 3, buffer_c, buffer_c, buffer_b)) == 192,
            "bad bundle", __LINE__);
    check(rtosc_message_length(buffer_d, 256) == 192,
            "bad bundle length", __LINE__);
    check(rtosc_message_length(buffer_d, len) == 192,
            "bad bundle length", __LINE__);

    //now to verify the bundle
    const char *msg = buffer_d;
    int good;
    POUND_BUNDLE
    TIMETAG
    SKIP_SIZE
    BUNDLE_C
    SKIP_SIZE
    BUNDLE_C
    SKIP_SIZE
    MSG_B

    check(rtosc_bundle_elements(buffer_d, len) == 3,
            "bad bundle elements", __LINE__);
    
    check(!strcmp("bundle",
                rtosc_argument(rtosc_bundle_fetch(rtosc_bundle_fetch(buffer_d, 1), 1), 0).s),
            "bad data retreival", __LINE__);
    
    //Verify the failure behavior when a bad length is provided
    check(rtosc_bundle_elements(buffer_d, len-1) == 2,
            "bad truncated bundle elements", __LINE__);
    check(rtosc_message_length(buffer_d, len-1) == 0,
            "bad truncated bundle length", __LINE__);
    return err;
}
