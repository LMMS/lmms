#pragma once
#include <FL/Fl_Input.H>
#include "Fl_Osc_Widget.H"

class Fl_Osc_Input: public Fl_Input, public Fl_Osc_Widget
{
    public:
        Fl_Osc_Input(int X, int Y, int W, int H, const char *label = NULL);
        virtual ~Fl_Osc_Input(void);
        
        //Normal Initialization
        void init(const char *path);

        virtual void OSC_value(const char *value) override;
};
