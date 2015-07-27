#include <rtosc/subtree-serialize.h>
#include <rtosc/ports.h>
#include <rtosc/rtosc.h>
#include <cstring>
#include <cassert>


using namespace rtosc;

/*
 * Append another message onto a bundle if the space permits it.
 * If insufficient space is available, then zero is returned and the buffer is
 * untouched.
 *
 * If this is useful it may be generalized to rtosc_bundle_append()
 */
static size_t append_bundle(char *dst, const char *src,
        size_t max_len, size_t dst_len, size_t src_len)
{
    assert(rtosc_message_length(src,src_len) == src_len);

    //Handle Edge case alignment
    //if(rtosc_bundle_elements(dst, dst_len) == 0)
    //    dst_len -= 4;


    if(max_len < dst_len + src_len + 4 || dst_len == 0 || src_len == 0)
        return 0;
    *(int32_t*)(dst+dst_len) = (int32_t)src_len;

    memcpy(dst+dst_len+4, src, src_len);

    return dst_len + src_len + 4;
}

//This object captures the output of any given port by calling it through a no
//argument message
//Assuming that the loc field is set correctly the message stored here will be
//able to be replayed to get an object to a previous state
class VarCapture : public RtData
{
    public:
        char buf[128];
        char location[128];
        char msg[128];
        const char *dummy;
        bool success;

        VarCapture(void)
            :dummy("/ser\0\0\0\0,\0\0\0")
        {
            memset(buf, 0, sizeof(buf));
            memset(location, 0, sizeof(buf));
            this->loc = location;
            success = false;
        }

        const char *capture(const Ports *p, const char *path, void *obj_)
        {
            this->loc = location;
            assert(this->loc == location);
            this->obj  = obj_;
            location[0] = '/';
            strcpy(location+1, path);
            success = false;
            size_t len = rtosc_message(msg, 128, path, "");
            (void) len;
            assert(len);
            assert(!strchr(path, ':'));

            p->dispatch(msg, *this);
            return success ? buf : NULL;
        }

        virtual void reply(const char *path, const char *args, ...)
        {
            assert(!success);
            assert(*path);
            va_list va;
            va_start(va, args);
            size_t len = rtosc_vmessage(buf, 128, path, args, va);
            (void) len;
            assert(len != 0);
            success = true;
            va_end(va);
        }
        virtual void broadcast(const char *msg)
        {
            (void) msg;
        }
};

struct subtree_args_t
{
    VarCapture v, vv;
    size_t len;
    char *buffer;
    size_t buffer_size;
    void *object;
    rtosc::Ports *ports;
};

size_t subtree_serialize(char *buffer, size_t buffer_size,
        void *object, rtosc::Ports *ports)
{
    (void) object;
    assert(buffer);
    assert(ports);

    subtree_args_t args;
    args.v.obj       = object;
    args.len         = rtosc_bundle(buffer, buffer_size, 0xdeadbeef0a0b0c0dULL, 0);
    args.buffer      = buffer;
    args.buffer_size = buffer_size;
    args.object      = object;
    args.ports       = ports;


    //TODO FIXME this is not currently RT safe at the moment
    walk_ports(ports, args.v.loc, 128, &args, [](const Port *p, const char *, void *dat) {
            if(p->meta().find("internal") != p->meta().end())
                return;

            subtree_args_t *args = (subtree_args_t*) dat;

            const char *buf = args->vv.capture(args->ports, args->v.loc+1, args->object);
            if(buf)
                args->len = append_bundle(args->buffer, buf, args->buffer_size, args->len,
                    rtosc_message_length(buf, 128));
            });

    return args.len;
}

void subtree_deserialize(char *buffer, size_t buffer_size,
        void *object, rtosc::Ports *ports, RtData &d)
{
    d.obj = object;
    //simply replay all objects seen here
    for(unsigned i=0; i<rtosc_bundle_elements(buffer, buffer_size); ++i) {
        const char *msg = rtosc_bundle_fetch(buffer, i);
        ports->dispatch(msg+1, d);
    }
}
