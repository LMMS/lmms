#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include <iostream>


class Object
{
    public:
        int foo;
        int bar;
};

#define rObject Object


//Assuming this test even compiles that implies some level of sanity to the
//syntaxtual sugar
rtosc::Ports p = {
    rOption(foo, rOpt(0,red) rOpt(1,blue) rOpt(2,green) rOpt(3,teal), "various options"),
    rOption(bar, rOptions(red,blue,green,teal), "various options")
};

int main()
{
    //Now a quick check for the sanity of memory access
    auto meta = p["foo"]->meta();
    for(auto x:meta)
    {
        printf("%s->%s\n", x.title, x.value);
    }

    rtosc::OscDocFormatter format{&p, "", "", "", "", ""};
    std::cout << format << std::endl;
    return 0;
}
