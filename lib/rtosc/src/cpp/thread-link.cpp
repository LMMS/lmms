#include "../../include/rtosc/thread-link.h"

namespace rtosc {
#ifdef off_t
#undef off_t
#endif
#define off_t signed long


//Ringbuffer internal structure
//XXX possible undefined behavior depending on future semantics of volatile
struct internal_ringbuffer_t {
    char *buffer;
    volatile off_t write;
    volatile off_t read;
    size_t size;
};

typedef internal_ringbuffer_t ringbuffer_t;

static size_t ring_read_size(ringbuffer_t *ring)
{
    const size_t w = ring->write;
    const size_t r = ring->read;

    return (w-r+ring->size) % ring->size;
}
static size_t ring_write_size(ringbuffer_t *ring)
{
    //leave one forbidden element
    const size_t w = ring->write;
    const size_t r = ring->read;
    if(r == w)
        return ring->size - 1;
    return ((r - w + ring->size) % ring->size) - 1;
}
static void ring_write(ringbuffer_t *ring, const char *data, size_t len)
{
    assert(ring_write_size(ring) >= len);
    const off_t  next_write = (ring->write + len)%ring->size;

    //discontinuous write
    if(next_write < ring->write) {
        const size_t w1 = ring->size - ring->write - 1;
        const size_t w2 = len - w1;
        memcpy(ring->buffer+ring->write, data,    w1);
        memcpy(ring->buffer,             data+w1, w2);
    } else { //contiguous
        memcpy(ring->buffer+ring->write, data, len);
    }
    ring->write = next_write;
}
static void ring_read(ringbuffer_t *ring, char *data, size_t len)
{
    assert(ring_read_size(ring) >= len);
    const off_t  next_read = (ring->read + len)%ring->size;

    //discontinuous read
    if(next_read < ring->read) {
        const size_t r1 = ring->size - ring->read - 1;
        const size_t r2 = len - r1;
        memcpy(data,    ring->buffer+ring->read, r1);
        memcpy(data+r1, ring->buffer,             r2);
    } else { //contiguous
        memcpy(data, ring->buffer+ring->read, len);
    }
    ring->read = next_read;
}
static void ring_read_vector(ringbuffer_t *ring, ring_t *r)
{
    assert(r);
    size_t read_size = ring_read_size(ring);
    off_t  read      = ring->read;
    r[0].data = ring->buffer+ring->read;
    if(read_size+read > ring->size) { //discontinuous
        size_t r2 = (read_size+1+read)%ring->size;
        size_t r1 = read_size - r2;
        r[0].len  = r1;
        r[1].data = ring->buffer;
        r[1].len  = r2;
    } else {
        r[0].len  = read_size;
        r[1].data = NULL;
        r[1].len  = 0;
    }
}

ThreadLink::ThreadLink(size_t max_message_length, size_t max_messages)
    :MaxMsg(max_message_length),
    BufferSize(MaxMsg*max_messages),
    write_buffer(new char[MaxMsg]),
    read_buffer(new char[MaxMsg]),
    ring(new ringbuffer_t)
{
    ring->buffer = new char[BufferSize];
    ring->size   = BufferSize;
    ring->read   = 0;
    ring->write  = 0;
    memset(write_buffer, 0, MaxMsg);
    memset(read_buffer, 0, MaxMsg);
}

ThreadLink::~ThreadLink(void)
{
    delete[] ring->buffer;
    delete   ring;
    delete[] write_buffer;
    delete[] read_buffer;
}

void ThreadLink::write(const char *dest, const char *args, ...)
{
    va_list va;
    va_start(va,args);
    const size_t len =
        rtosc_vmessage(write_buffer,MaxMsg,dest,args,va);
    va_end(va);
    if(ring_write_size(ring) >= len)
        ring_write(ring,write_buffer,len);
}

void ThreadLink::writeArray(const char *dest, const char *args, const rtosc_arg_t *aargs)
{
    const size_t len =
        rtosc_amessage(write_buffer, MaxMsg, dest, args, aargs);
    if(ring_write_size(ring) >= len)
        ring_write(ring,write_buffer,len);
}

/**
 * Directly write message to ringbuffer
 */
void ThreadLink::raw_write(const char *msg)
{
    const size_t len = rtosc_message_length(msg, -1);//assumed valid
    if(ring_write_size(ring) >= len)
        ring_write(ring,msg,len);
}

/**
 * @returns true iff there is another message to be read in the buffer
 */
bool ThreadLink::hasNext(void) const
{
    return ring_read_size(ring);
}

/**
 * Read a new message from the ringbuffer
 */
msg_t ThreadLink::read(void) {
    ring_t r[2];
    ring_read_vector(ring,r);
    const size_t len =
        rtosc_message_ring_length(r);
    assert(ring_read_size(ring) >= len);
    assert(len <= MaxMsg);
    ring_read(ring, read_buffer, len);
    return read_buffer;
}

/**
 * Peak at last message read without reading another
 */
msg_t ThreadLink::peak(void) const
{
    return read_buffer;
}

/**
 * Raw write buffer access for more complicated task
 */
char *ThreadLink::buffer(void) {return write_buffer;}
/**
 * Access to write buffer length
 */
size_t ThreadLink::buffer_size(void) const {return BufferSize;}

};
