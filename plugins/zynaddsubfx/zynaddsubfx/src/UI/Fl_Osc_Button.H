#pragma once
#include <FL/Fl_Button.H>
#include "Fl_Osc_Widget.H"
#include <string>

using std::string; //yes this is bad form FIXME

class Fl_Osc_Button:public Fl_Button, public Fl_Osc_Widget
{

    public:
        Fl_Osc_Button(int X, int Y, int W, int H, const char *label);

        virtual ~Fl_Osc_Button(void);
        virtual void OSC_value(bool);

        virtual void rebase(std::string) override;

        //void init(Fl_Osc_Interface *, std::string);
        //void init(std::string);

        //void cb(void);
        //static void _cb(Fl_Widget *w, void *);
};
