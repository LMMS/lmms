#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtosc/rtosc.h>

char buffer[256];
char buffer2[256];

void check(int b, const char *msg, int loc)
{
    if(!b) {
        fprintf(stderr, "%s:%d\n", msg, loc);
        exit(1);
    }
}

int main()
{
    memset(buffer2, 0xff, sizeof(buffer2));

    //Check the creation of a simple no arguments message
    check(rtosc_message(buffer, 256, "/page/poge", "TIF") == 20,
            "Incorrect message length", __LINE__);

    //                         4     8      12   16
    check(!memcmp(buffer, "/pag""e/po""ge\0\0"",TIF""\0\0\0", 20),
            "Incorrect message contents", __LINE__);

    check(rtosc_message_length(buffer, 256) == 20,
            "Incorrect detected message length", __LINE__);

    //Verify that it can be read properly
    check(rtosc_narguments(buffer) == 3,
            "Incorrect number of arguments", __LINE__);

    check(rtosc_type(buffer, 0) == 'T',
            "Incorrect truth argument", __LINE__);

    check(rtosc_type(buffer, 1) == 'I',
            "Incorrect infinity argument", __LINE__);

    check(rtosc_type(buffer, 2) == 'F',
            "Incorrect false argument", __LINE__);

    //Check the creation of a more complex message
    check(rtosc_message(buffer, 256, "/testing", "is", 23, "this string") == 32,
            "Incorrect message length", __LINE__);
    //                       4     8        12      16         20    24    28    32
    check(!memcmp(buffer,"/tes""ting""\0\0\0\0"",is\0""\0\0\0\x17""this"" str""ing", 32),
            "Invalid OSC message", __LINE__);

    //Verify that a string argument can be retreived
    rtosc_message(buffer, 256, "/register", "iis", 2, 13, "/amp-env/av");
    const char *pos = rtosc_argument(buffer,2).s;
    while(*pos) putchar(*pos++);
    printf("%p %p %c(%d)\n", buffer, rtosc_argument(buffer,2).s,
            *(rtosc_argument(buffer,2).s), *(rtosc_argument(buffer,2).s));
    check(!strcmp(rtosc_argument(buffer,2).s,"/amp-env/av"),
            "Bad string object", __LINE__);

    //Verify that buffer overflows will not occur
    check(rtosc_message(buffer, 32, "/testing", "is", 23, "this string") == 32,
            "Incorrect message length", __LINE__);

    check(rtosc_message_length(buffer, 256) == 32,
            "Invalid detected message length", __LINE__);

    check(rtosc_message(buffer, 31, "/testing", "is", 23, "this string") == 0,
            "Incorrect message length", __LINE__);

    check(!*buffer,
            "Buffer was not cleared on possible overflow", __LINE__);

    //check simple float retrevial
    check(rtosc_message(buffer, 32, "oscil/freq", "f", 13523.34) != 0,
            "Bad message", __LINE__);

    check(rtosc_argument(buffer,0).f+0.1>13523.34 &&
            rtosc_argument(buffer,0).f-0.1<13523.34,
            "Incorrect floating point value", __LINE__);

    //Check simple character retrevial
    check(rtosc_message(buffer, 256, "/test", "cccc", 0xde,0xad,0xbe,0xef) != 0,
            "Bad message", __LINE__);

    check(rtosc_argument(buffer, 0).i == 0xde,
            "Bad argument", __LINE__);
    check(rtosc_argument(buffer, 3).i == 0xef,
            "Bad argument", __LINE__);

    //Verify argument retreval for short messages
    check(rtosc_message(buffer, 256, "/b", "c", 7) != 0,
            "Bad message", __LINE__);
    check(rtosc_argument(buffer+1, 0).i == 7,
            "Bad argument", __LINE__);

    //Work on a recently found bug
    check((rtosc_message(buffer, 256, "m",
                "bb", 4, buffer2, 1, buffer2)) == 24,
            "Bad message", __LINE__);
    check(rtosc_argument(buffer, 0).b.len == 4,
            "Bad argument", __LINE__);
    check(rtosc_argument(buffer, 1).b.len == 1,
            "Bad argument", __LINE__);
    return 0;
}
