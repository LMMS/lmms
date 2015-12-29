#include <lo/lo.h>
#include <rtosc/rtosc.h>
#include <string.h>
#include <stdio.h>

char buffer[128];

char buffer2[1024];

char buffer3[2048];

int good = 1;
int main()
{
    //clean buffer
    memset(buffer, 0, sizeof(buffer));

    //generate liblo message
    size_t len = 128;
    lo_message message = lo_message_new();
    lo_message_add_float(message, 24.0);
    lo_message_add_int32(message, 42);
    lo_message_serialise(message, "/path", buffer, &len);
    lo_message_free(message);

    printf("Generating a message from liblo '/path\\0\\0\\0,fi\\0iiiiffff'...\n");

    good &= !strcmp("/path", buffer);
    good &= 24.0 == rtosc_argument(buffer, 0).f;
    good &= 42   == rtosc_argument(buffer, 1).i;
    good &= len  == rtosc_message_length(buffer, 128);
    printf("Message sanity is currently '%d'\n", good);

    printf("Generating a message from rtosc '/li\\0\\0,bb\\0iiiibbbbiiiibbbb'...\n");
    size_t len2 = rtosc_message(buffer2, 1024, "/li", "bb", 4, buffer, 4, buffer);
    for(size_t i=0; i<len2; i+=4)  {
        for(size_t j=i; j<len2 && j<i+4; ++j) {
            printf("%02x", (unsigned char)buffer2[j]);
        }
        printf(" ");
    }
    printf("\n");

    for(size_t i=0; i<len2; i+=4)  {
        for(size_t j=i; j<len2 && j<i+4; ++j) {
            printf("%c", (unsigned char)buffer2[j]);
        }
        printf(" ");
    }
    printf("\n");
    printf("message size is '%zd'\n", len2);

    int result = 0;
    lo_message msg2 = lo_message_deserialise((void*)buffer2, len2, &result);
    good &= msg2 != NULL;
    if(msg2 == NULL)
        printf("Liblo Did Not Accept the Rtosc Message [error '%d']\n", result);
    else
        printf("Liblo Accpted The Rtosc Message\n");

    printf("Generating a message from rtosc '/close-ui\\0\\0\\0,\\0\\0\\0'...\n");
    int len3 = rtosc_message(buffer3+4, 2048, "/close-ui", "");
    lo_message msg3 = lo_message_deserialise((void*)buffer3+4, len3, &result);
    printf("len3 = %d\n", len3);
    good &= msg2 != NULL;
    if(msg3 == NULL)
        printf("Liblo Did Not Accept the Rtosc Message [error '%d']\n", result);
    else
        printf("Liblo Accpted The Rtosc Message\n");

    return !good;
}
