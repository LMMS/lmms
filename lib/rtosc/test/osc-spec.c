#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtosc/rtosc.h>

char buffer[256];

const unsigned char message_one[4*8] = {
    0x2f, 0x6f, 0x73, 0x63,
    0x69, 0x6c, 0x6c, 0x61,
    0x74, 0x6f, 0x72, 0x2f,
    0x34, 0x2f, 0x66, 0x72,
    0x65, 0x71, 0x75, 0x65,
    0x6e, 0x63, 0x79, 0x00,
    0x2c, 0x66, 0x00, 0x00,
    0x43, 0xdc, 0x00, 0x00,
};

const unsigned char message_two[4*10] = {
    0x2f, 0x66, 0x6f, 0x6f,
    0x00, 0x00, 0x00, 0x00,
    0x2c, 0x69, 0x69, 0x73,
    0x66, 0x66, 0x00, 0x00,
    0x00, 0x00, 0x03, 0xe8,
    0xff, 0xff, 0xff, 0xff,
    0x68, 0x65, 0x6c, 0x6c,
    0x6f, 0x00, 0x00, 0x00,
    0x3f, 0x9d, 0xf3, 0xb6,
    0x40, 0xb5, 0xb2, 0x2d,
};

void check(int b, const char *msg, int loc)
{
    if(!b) {
        fprintf(stderr, "%s:%d\n", msg, loc);
        exit(1);
    }
}

/*
 * The OSC 1.0 spec provides two example messages at
 * http://opensoundcontrol.org/spec-1_0-examples
 * 
 * This test verifies that these messages are identically serialized
 */
int main()
{
    check(rtosc_message(buffer, 256, "/oscillator/4/frequency", "f", 440.0f),
            "Failed message creation", __LINE__);
    check(!memcmp(buffer, message_one, sizeof(message_one)),
            "Message does not meet spec", __LINE__);

    check(rtosc_message(buffer, 256, "/foo", "iisff",
                1000, -1, "hello", 1.234f, 5.678f),
            "Failed message creation", __LINE__);
    check(!memcmp(buffer, message_two, sizeof(message_two)),
            "Message does not meet spec", __LINE__);
    return 0;
}
