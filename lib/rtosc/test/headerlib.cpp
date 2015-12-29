#include <rtosc/rtosc.h>
#include <rtosc/thread-link.h>
#include <rtosc/ports.h>
#include <string>
#include <iostream>

using namespace rtosc;

std::string resultA;
int resultB = 0;
Ports ports = {
    {"setstring:s", "", 0, [](msg_t msg,RtData&) {resultA = rtosc_argument(msg,0).s;}},
    {"setint:i",    "", 0, [](msg_t msg,RtData&) {resultB = rtosc_argument(msg,0).i;}},
    {"echo:ss",     "", 0, [](msg_t,RtData&) {}}
};

ThreadLink tlink(512,100);

int main()
{
    for(int j=0; j<100; ++j) {
        for(int i=0; i<100; ++i){
            tlink.write("badvalue",  "");
            tlink.write("setstring", "s", "testing");
            tlink.write("setint",    "i", 123);
            tlink.write("setint",    "s", "dog");
            tlink.write("echo",      "ss", "hello", "rtosc");
        }

        RtData d;
        d.loc_size = 0;
        d.obj = d.loc = NULL;
        while(tlink.hasNext())
            ports.dispatch(tlink.read(), d);
    }
    tlink.write("echo", "ss", "hello", "rtosc");

    return !(resultB==123 && resultA=="testing");
}
