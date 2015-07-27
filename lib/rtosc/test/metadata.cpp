#include <rtosc/rtosc.h>
#include <rtosc/ports.h>
#include <cassert>

using namespace rtosc;

void dummy(msg_t, RtData&)
{}

#define MAP(a,b) ":" #a "\0=" #b "\0"

Ports ports = {
    {"porta", ":name\0=porta\0"
              ":doc\0=This is an example description for porta\0",
              NULL, dummy},
    {"portb", ":min\0=23\0", 0, dummy},
    {"portc:i", ":realtime\0:scale\0=linear\0", 0, dummy},
    {"portc:",  NULL, 0, dummy},
    {"portd",  NULL, 0, dummy},
    {"porte", MAP(name, porte)
              MAP(min, 12)
              MAP(max, 23)
              MAP(Hello, "WOrld"), 0, dummy}
};

int main()
{
    for(const auto &port : ports) {
        for(const auto desc : port.meta())
            printf("%s:'%s' => '%s'\n", port.name, desc.title, desc.value);
        assert(port.meta().length() < 100);
    }
}
