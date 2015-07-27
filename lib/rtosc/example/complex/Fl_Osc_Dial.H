#pragma once
#include <FL/Fl_Dial.H>
#include "Fl_Osc_Widget.H"
#include <string>

using std::string; //yes this is bad form FIXME

class Fl_Osc_Dial:public Fl_Dial, Fl_Osc_Widget
{

    public:
        Fl_Osc_Dial(int X, int Y, int W, int H, string name,
                const char *metadata);
        virtual ~Fl_Osc_Dial(void);
        void OSC_value(float);

        void cb(void);
        static void _cb(Fl_Widget *w, void *);
    private:
        char label_str[64];
        double real_value;
        class Fl_Osc_Interface *osc;
};
