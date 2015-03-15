#pragma once
#include <FL/Fl_Dial.H>
#include "WidgetPDial.h"
#include "Fl_Osc_Widget.H"
#include <string>

class Fl_Osc_DialF:public WidgetPDial, Fl_Osc_Widget
{

    public:
        Fl_Osc_DialF(int X, int Y, int W, int H, const char *label = NULL);
        virtual ~Fl_Osc_DialF(void);
        void init(const char *path);
        void OSC_value(float);

        //Refetch parameter information
        void update(void);
        void callback(Fl_Callback *cb, void *p = NULL);

        void cb(void);
    private:
        std::pair<Fl_Callback*, void*> cb_data;
};
