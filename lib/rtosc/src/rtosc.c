#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>

#include <rtosc/rtosc.h>

const char *rtosc_argument_string(const char *msg)
{
    assert(msg && *msg);
    while(*++msg); //skip pattern
    while(!*++msg);//skip null
    return msg+1;  //skip comma
}

unsigned rtosc_narguments(const char *msg)
{
    const char *args = rtosc_argument_string(msg);
    int nargs = 0;
    while(*args++)
        nargs += (*args == ']' || *args == '[') ? 0 : 1;
    return nargs;
}

static int has_reserved(char type)
{
    switch(type)
    {
        case 'i'://official types
        case 's':
        case 'b':
        case 'f':

        case 'h'://unofficial
        case 't':
        case 'd':
        case 'S':
        case 'r':
        case 'm':
        case 'c':
            return 1;
        case 'T':
        case 'F':
        case 'N':
        case 'I':
        case '[':
        case ']':
            return 0;
    }

    //Should not happen
    return 0;
}

static unsigned nreserved(const char *args)
{
    unsigned res = 0;
    for(;*args;++args)
        res += has_reserved(*args);

    return res;
}

char rtosc_type(const char *msg, unsigned nargument)
{
    assert(nargument < rtosc_narguments(msg));
    const char *arg = rtosc_argument_string(msg);
    while(1) {
        if(*arg == '[' || *arg == ']')
            ++arg;
        else if(!nargument || !*arg)
            return *arg;
        else
            ++arg, --nargument;
    }
}

static unsigned arg_off(const char *msg, unsigned idx)
{
    if(!has_reserved(rtosc_type(msg,idx)))
        return 0;

    //Iterate to the right position
    const uint8_t *args = (const uint8_t*) rtosc_argument_string(msg);
    const uint8_t *aligned_ptr = args-1;
    const uint8_t *arg_pos = args;

    while(*++arg_pos);
    //Alignment
    arg_pos += 4-(arg_pos-((uint8_t*)aligned_ptr))%4;

    //ignore any leading '[' or ']'
    while(*args == '[' || *args == ']')
        ++args;

    while(idx--) {
        uint32_t bundle_length = 0;
        switch(*args++)
        {
            case 'h':
            case 't':
            case 'd':
                arg_pos +=8;
                break;
            case 'm':
            case 'r':
            case 'f':
            case 'c':
            case 'i':
                arg_pos += 4;
                break;
            case 'S':
            case 's':
                while(*++arg_pos);
                arg_pos += 4-(arg_pos-((uint8_t*)aligned_ptr))%4;
                break;
            case 'b':
                bundle_length |= (*arg_pos++ << 24);
                bundle_length |= (*arg_pos++ << 16);
                bundle_length |= (*arg_pos++ << 8);
                bundle_length |= (*arg_pos++);
                if(bundle_length%4)
                    bundle_length += 4-bundle_length%4;
                arg_pos += bundle_length;
                break;
            case '[':
            case ']':
                //completely ignore array chars
                ++idx;
                break;
            case 'T':
            case 'F':
            case 'I':
                ;
        }
    }
    return arg_pos-(uint8_t*)msg;
}

size_t rtosc_message(char   *buffer,
                     size_t      len,
                     const char *address,
                     const char *arguments,
                     ...)
{
    va_list va;
    va_start(va, arguments);
    size_t result = rtosc_vmessage(buffer, len, address, arguments, va);
    va_end(va);
    return result;
}

//Calculate the size of the message without writing to a buffer
static size_t vsosc_null(const char        *address,
                         const char        *arguments,
                         const rtosc_arg_t *args)
{
    unsigned pos = 0;
    pos += strlen(address);
    pos += 4-pos%4;//get 32 bit alignment
    pos += 1+strlen(arguments);
    pos += 4-pos%4;

    unsigned toparse = nreserved(arguments);
    unsigned arg_pos = 0;

    //Take care of varargs
    while(toparse)
    {
        char arg = *arguments++;
        assert(arg);
        int i;
        const char *s;
        switch(arg) {
            case 'h':
            case 't':
            case 'd':
                ++arg_pos;
                pos += 8;
                --toparse;
                break;
            case 'm':
            case 'r':
            case 'c':
            case 'f':
            case 'i':
                ++arg_pos;
                pos += 4;
                --toparse;
                break;
            case 's':
            case 'S':
                s = args[arg_pos++].s;
                assert(s && "Input strings CANNOT be NULL");
                pos += strlen(s);
                pos += 4-pos%4;
                --toparse;
                break;
            case 'b':
                i = args[arg_pos++].b.len;
                pos += 4 + i;
                if(pos%4)
                    pos += 4-pos%4;
                --toparse;
                break;
            default:
                ;
        }
    }

    return pos;
}
size_t rtosc_vmessage(char   *buffer,
                      size_t      len,
                      const char *address,
                      const char *arguments,
                      va_list ap)
{
    const unsigned nargs = nreserved(arguments);
    if(!nargs)
        return rtosc_amessage(buffer,len,address,arguments,NULL);

    rtosc_arg_t args[nargs];

    unsigned arg_pos = 0;
    const char *arg_str = arguments;
    uint8_t *midi_tmp;
    while(arg_pos < nargs)
    {
        switch(*arg_str++) {
            case 'h':
            case 't':
                args[arg_pos++].h = va_arg(ap, int64_t);
                break;
            case 'd':
                args[arg_pos++].d = va_arg(ap, double);
                break;
            case 'c':
            case 'i':
            case 'r':
                args[arg_pos++].i = va_arg(ap, int);
                break;
            case 'm':
                midi_tmp = va_arg(ap, uint8_t *);
                args[arg_pos].m[0] = midi_tmp[0];
                args[arg_pos].m[1] = midi_tmp[1];
                args[arg_pos].m[2] = midi_tmp[2];
                args[arg_pos++].m[3] = midi_tmp[3];
                break;
            case 'S':
            case 's':
                args[arg_pos++].s = va_arg(ap, const char *);
                break;
            case 'b':
                args[arg_pos].b.len = va_arg(ap, int);
                args[arg_pos].b.data = va_arg(ap, unsigned char *);
                arg_pos++;
                break;
            case 'f':
                args[arg_pos++].f = va_arg(ap, double);
                break;
            default:
                ;
        }
    }

    return rtosc_amessage(buffer,len,address,arguments,args);
}

size_t rtosc_amessage(char              *buffer,
                      size_t             len,
                      const char        *address,
                      const char        *arguments,
                      const rtosc_arg_t *args)
{
    const size_t total_len = vsosc_null(address, arguments, args);

    if(!buffer)
        return total_len;

    //Abort if the message cannot fit
    if(total_len>len) {
        memset(buffer, 0, len);
        return 0;
    }

    memset(buffer, 0, total_len);

    unsigned pos = 0;
    while(*address)
        buffer[pos++] = *address++;

    //get 32 bit alignment
    pos += 4-pos%4;

    buffer[pos++] = ',';

    const char *arg_str = arguments;
    while(*arg_str)
        buffer[pos++] = *arg_str++;

    pos += 4-pos%4;

    unsigned toparse = nreserved(arguments);
    unsigned arg_pos = 0;
    while(toparse)
    {
        char arg = *arguments++;
        assert(arg);
        int32_t i;
        int64_t d;
        const uint8_t *m;
        const char *s;
        const unsigned char *u;
        rtosc_blob_t b;
        switch(arg) {
            case 'h':
            case 't':
            case 'd':
                d = args[arg_pos++].t;
                buffer[pos++] = ((d>>56) & 0xff);
                buffer[pos++] = ((d>>48) & 0xff);
                buffer[pos++] = ((d>>40) & 0xff);
                buffer[pos++] = ((d>>32) & 0xff);
                buffer[pos++] = ((d>>24) & 0xff);
                buffer[pos++] = ((d>>16) & 0xff);
                buffer[pos++] = ((d>>8) & 0xff);
                buffer[pos++] = (d & 0xff);
                --toparse;
                break;
            case 'r':
            case 'f':
            case 'c':
            case 'i':
                i = args[arg_pos++].i;
                buffer[pos++] = ((i>>24) & 0xff);
                buffer[pos++] = ((i>>16) & 0xff);
                buffer[pos++] = ((i>>8) & 0xff);
                buffer[pos++] = (i & 0xff);
                --toparse;
                break;
            case 'm':
                //TODO verify ordering of spec
                m = args[arg_pos++].m;
                buffer[pos++] = m[0];
                buffer[pos++] = m[1];
                buffer[pos++] = m[2];
                buffer[pos++] = m[3];
                --toparse;
                break;
            case 'S':
            case 's':
                s = args[arg_pos++].s;
                while(*s)
                    buffer[pos++] = *s++;
                pos += 4-pos%4;
                --toparse;
                break;
            case 'b':
                b = args[arg_pos++].b;
                i = b.len;
                buffer[pos++] = ((i>>24) & 0xff);
                buffer[pos++] = ((i>>16) & 0xff);
                buffer[pos++] = ((i>>8) & 0xff);
                buffer[pos++] = (i & 0xff);
                u = b.data;
                if(u) {
                    while(i--)
                        buffer[pos++] = *u++;
                }
                else
                    pos += i;
                if(pos%4)
                    pos += 4-pos%4;
                --toparse;
                break;
            default:
                ;
        }
    }

    return pos;
}

rtosc_arg_t rtosc_argument(const char *msg, unsigned idx)
{
    rtosc_arg_t result = {0};
    char type = rtosc_type(msg, idx);
    //trivial case
    if(!has_reserved(type)) {
        switch(type)
        {
            case 'T':
                result.T = true;
                break;
            case 'F':
                result.T = false;
                break;
            default:
                ;
        }
    } else {
        const unsigned char *arg_pos = (const unsigned char*)msg+arg_off(msg,idx);
        switch(type)
        {
            case 'h':
            case 't':
            case 'd':
                result.t |= (((uint64_t)*arg_pos++) << 56);
                result.t |= (((uint64_t)*arg_pos++) << 48);
                result.t |= (((uint64_t)*arg_pos++) << 40);
                result.t |= (((uint64_t)*arg_pos++) << 32);
                result.t |= (((uint64_t)*arg_pos++) << 24);
                result.t |= (((uint64_t)*arg_pos++) << 16);
                result.t |= (((uint64_t)*arg_pos++) << 8);
                result.t |= (((uint64_t)*arg_pos++));
                break;
            case 'r':
            case 'f':
            case 'c':
            case 'i':
                result.i |= (*arg_pos++ << 24);
                result.i |= (*arg_pos++ << 16);
                result.i |= (*arg_pos++ << 8);
                result.i |= (*arg_pos++);
                break;
            case 'm':
                result.m[0] = *arg_pos++;
                result.m[1] = *arg_pos++;
                result.m[2] = *arg_pos++;
                result.m[3] = *arg_pos++;
                break;
            case 'b':
                result.b.len |= (*arg_pos++ << 24);
                result.b.len |= (*arg_pos++ << 16);
                result.b.len |= (*arg_pos++ << 8);
                result.b.len |= (*arg_pos++);
                result.b.data = (unsigned char *)arg_pos;
                break;
            case 'S':
            case 's':
                result.s = (char *)arg_pos;
                break;
        }
    }

    return result;
}

static unsigned char deref(unsigned pos, ring_t *ring)
{
    return pos<ring[0].len ? ring[0].data[pos] :
        ((pos-ring[0].len)<ring[1].len ? ring[1].data[pos-ring[0].len] : 0x00);
}

static size_t bundle_ring_length(ring_t *ring)
{
    unsigned pos = 8+8;//goto first length field
    uint32_t advance = 0;
    do {
        advance = deref(pos+0, ring) << (8*0) |
                  deref(pos+1, ring) << (8*1) |
                  deref(pos+2, ring) << (8*2) |
                  deref(pos+3, ring) << (8*3);
        if(advance)
            pos += 4+advance;
    } while(advance);

    return pos <= (ring[0].len+ring[1].len) ? pos : 0;
}

//Zero means no full message present
size_t rtosc_message_ring_length(ring_t *ring)
{
    //Check if the message is a bundle
    if(deref(0,ring) == '#' &&
            deref(1,ring) == 'b' &&
            deref(2,ring) == 'u' &&
            deref(3,ring) == 'n' &&
            deref(4,ring) == 'd' &&
            deref(5,ring) == 'l' &&
            deref(6,ring) == 'e' &&
            deref(7,ring) == '\0')
        return bundle_ring_length(ring);

    //Proceed for normal messages
    //Consume path
    unsigned pos = 0;
    while(deref(pos++,ring));
    pos--;

    //Travel through the null word end [1..4] bytes
    for(int i=0; i<4; ++i)
        if(deref(++pos, ring))
            break;

    if(deref(pos, ring) != ',')
        return 0;

    unsigned aligned_pos = pos;
    int arguments = pos+1;
    while(deref(++pos,ring));
    pos += 4-(pos-aligned_pos)%4;

    unsigned toparse = 0;
    {
        int arg = arguments-1;
        while(deref(++arg,ring))
            toparse += has_reserved(deref(arg,ring));
    }

    //Take care of varargs
    while(toparse)
    {
        char arg = deref(arguments++,ring);
        assert(arg);
        uint32_t i;
        switch(arg) {
            case 'h':
            case 't':
            case 'd':
                pos += 8;
                --toparse;
                break;
            case 'm':
            case 'r':
            case 'c':
            case 'f':
            case 'i':
                pos += 4;
                --toparse;
                break;
            case 'S':
            case 's':
                while(deref(++pos,ring));
                pos += 4-(pos-aligned_pos)%4;
                --toparse;
                break;
            case 'b':
                i = 0;
                i |= (deref(pos++,ring) << 24);
                i |= (deref(pos++,ring) << 16);
                i |= (deref(pos++,ring) << 8);
                i |= (deref(pos++,ring));
                pos += i;
                if((pos-aligned_pos)%4)
                    pos += 4-(pos-aligned_pos)%4;
                --toparse;
                break;
            default:
                ;
        }
    }


    return pos <= (ring[0].len+ring[1].len) ? pos : 0;
}

size_t rtosc_message_length(const char *msg, size_t len)
{
    ring_t ring[2] = {{(char*)msg,len},{NULL,0}};
    return rtosc_message_ring_length(ring);
}

bool rtosc_valid_message_p(const char *msg, size_t len)
{
    //Validate Path Characters (assumes printable characters are sufficient)
    if(*msg != '/')
        return false;
    const char *tmp = msg;
    for(unsigned i=0; i<len; ++i) {
        if(*tmp == 0)
            break;
        if(!isprint(*tmp))
            return false;
        tmp++;
    }

    //tmp is now either pointing to a null or the end of the string
    const size_t offset1 = tmp-msg;
    size_t       offset2 = tmp-msg;
    for(; offset2<len; offset2++) {
        if(*tmp == ',')
            break;
        tmp++;
    }

    //Too many NULL bytes
    if(offset2-offset1 > 4)
        return false;

    if((offset2 % 4) != 0)
        return false;

    size_t observed_length = rtosc_message_length(msg, len);
    return observed_length == len;
}

size_t rtosc_bundle(char *buffer, size_t len, uint64_t tt, int elms, ...)
{
    char *_buffer = buffer;
    memset(buffer, 0, len);
    strcpy(buffer, "#bundle");
    buffer += 8;
    (*(uint64_t*)buffer) = tt;
    buffer +=8;
    va_list va;
    va_start(va, elms);
    for(int i=0; i<elms; ++i) {
        const char   *msg  = va_arg(va, const char*);
        //It is assumed that any passed message/bundle is valid
        size_t        size = rtosc_message_length(msg, -1);
        *(uint32_t*)buffer = size;
        buffer += 4;
        memcpy(buffer, msg, size);
        buffer+=size;
    }
    va_end(va);

    return buffer-_buffer;
}

#define POS ((size_t)(((const char *)lengths) - buffer))
size_t rtosc_bundle_elements(const char *buffer, size_t len)
{
    const uint32_t *lengths = (const uint32_t*) (buffer+16);
    size_t elms = 0;
    //TODO
    while(POS < len && *lengths) {
        lengths += *lengths/4+1;

        if(POS > len)
            break;
        ++elms;
    }
    return elms;
}
#undef POS

const char *rtosc_bundle_fetch(const char *buffer, unsigned elm)
{
    const uint32_t *lengths = (const uint32_t*) (buffer+16);
    size_t elm_pos = 0;
    while(elm_pos!=elm && *lengths) ++elm_pos, lengths+=*lengths/4+1;

    return (const char*) (elm==elm_pos?lengths+1:NULL);
}

size_t rtosc_bundle_size(const char *buffer, unsigned elm)
{
    const uint32_t *lengths = (const uint32_t*) (buffer+16);
    size_t elm_pos = 0;
    size_t last_len = 0;
    while(elm_pos!=elm && *lengths) {
        last_len = *lengths;
        ++elm_pos, lengths+=*lengths/4+1;
    }

    return last_len;
}

int rtosc_bundle_p(const char *msg)
{
    return !strcmp(msg,"#bundle");
}

uint64_t rtosc_bundle_timetag(const char *msg)
{
    return *(uint64_t*)(msg+8);
}
