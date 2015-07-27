#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include <rtosc/undo-history.h>
#include <cstdarg>

#include <cassert>

using namespace rtosc;

class Object
{
    public:
        Object(void)
            :x(0),b(0),i(0)
        {}
        float x;
        char b;
        int i;
};


#define rObject Object

Ports ports = {
    rParam(b, "b"),
    rParamF(x, "x"),
    rParamI(i, "i"),
};

char reply_buf[256];
struct Rt:public RtData
{
    Rt(Object *o, UndoHistory *uh_)
    {
        memset(reply_buf, 0, sizeof(reply_buf));
        loc      = new char[128];
        memset(loc, 0, 128);
        loc_size = 128;
        obj      = o;
        uh       = uh_;
        enable   = true;
    }
    void reply(const char *path, const char *args, ...)
    {
        if(strcmp(path, "undo_change") || !enable)
            return;

        va_list va;
        va_start(va, args);
        rtosc_vmessage(reply_buf, sizeof(reply_buf),
                path, args, va);
        uh->recordEvent(reply_buf);
        va_end(va);
    }

    bool enable;
    UndoHistory *uh;
};


char message_buff[256];
int main()
{
    memset(message_buff, 0, sizeof(reply_buf));
    //Initialize structures
    Object o;
    UndoHistory hist;
    Rt rt(&o, &hist);
    hist.setCallback([&rt](const char*msg) {ports.dispatch(msg+1, rt);});

    assert(o.b == 0);

    rt.matches = 0;
    int len = rtosc_message(message_buff, 128, "b", "c", 7);
    for(int i=0; i<len; ++i)
        printf("%hhx",message_buff[i]);
    printf("\n");
    ports.dispatch(message_buff, rt);
    assert(rt.matches == 1);
    printf("rt.matches == '%d'\n", rt.matches);
    assert(o.b == 7);

    rt.enable = false;
    hist.showHistory();
    hist.seekHistory(-1);
    assert(o.b == 0);
    hist.showHistory();

    hist.seekHistory(+1);
    printf("the result is '%d'\n", o.b);
    assert(o.b == 7);

    return 0;
}


