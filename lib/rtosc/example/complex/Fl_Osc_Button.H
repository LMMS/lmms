#pragma once
#include <FL/Fl_Button.H>
#include "Fl_Osc_Widget.H"
#include <string>

using std::string; //yes this is bad form FIXME

class Fl_Osc_Button:public Fl_Button, Fl_Osc_Widget
{

    public:
        Fl_Osc_Button(int X, int Y, int W, int H, const char *name,
                const char *metadata);

        virtual ~Fl_Osc_Button(void);
        void OSC_value(bool);

        void cb(void);
        static void _cb(Fl_Widget *w, void *);
    private:
        class Fl_Osc_Interface *osc;
};
