#ifdef NDEBUG
#undef NDEBUG
#endif

#include <rtosc/rtosc.h>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include <rtosc/subtree-serialize.h>
#include <cassert>

using namespace rtosc;

//Define Objects
class SubObject
{
    public:
        int foobar;
        static Ports ports;
};

class Object
{
    public:
        char foo;
        float bar;
        char  blam[64];
        SubObject baz;

        static Ports ports;
};


//Define Ports
#define rObject Object

Ports Object::ports = {
    rParam(foo,  "a character field"),
    rParamF(bar, "a float field"),
    rArray(blam, 64, "an array"),
    rRecur(baz,  "a child object"),
};

#undef  rObject
#define rObject SubObject
Ports SubObject::ports = {
    rToggle(foobar, "a boolean object")
};

//TODO there should be some way to go through this and figure out what the size
//of the buffer should be before running the serializer
char buffer[256];
char buffera[2048];
size_t sizea;
char bufferb[2048];
size_t sizeb;
char bufferc[2048];
size_t sizec;
Object o;

void run_test(void)
{
    RtData d;
    char rtdatabuf[100];

    //Initialize Parameters
    d.loc = rtdatabuf;
    d.loc_size = 100;
    memset(rtdatabuf, 0, sizeof(rtdatabuf));

    o.foo = 12;
    o.bar = 0.0f;
    o.baz.foobar = 0;
    memset(o.blam, 0, sizeof(o.blam));
    o.blam[32] = 45;
    o.blam[12] = 80;

    //Save an image
    sizea = subtree_serialize(buffera, sizeof(buffera), &o, &Object::ports);

    //Assert Image was saved
    assert(sizea != 0);
    assert(rtosc_bundle_p(buffera));
    assert(rtosc_bundle_elements(buffera, sizea) > 2);

    //Change variables to new state
    o.bar = 128.0f;
    o.foo = 0;
    o.baz.foobar = 1;
    o.blam[32] = 120;
    o.blam[12] = 3;
    o.blam[2] = 5;
    o.blam[1] = 2;

    //Save Second Image
    //Sizes must be identical due to OSC format
    sizeb = subtree_serialize(bufferb, sizeof(bufferb), &o, &Object::ports);
    assert(sizeb == sizea);

    //Reload Save State 1
    subtree_deserialize(buffera, sizea, &o, &Object::ports, d);
    assert(o.bar == 0);
    assert(o.foo == 12);
    assert(o.baz.foobar == 0);
    assert(o.blam[32] == 45);
    assert(o.blam[12] == 80);

    //Reload Save State 2
    subtree_deserialize(bufferb, sizeb, &o, &Object::ports, d);
    assert(o.baz.foobar == 1);
    assert(o.blam[12] == 3);
    assert(o.blam[2]  == 5);
}

int main(int, char**)
{
    for(int i=0; i<1000; ++i)
        run_test();

    return 0;
}
