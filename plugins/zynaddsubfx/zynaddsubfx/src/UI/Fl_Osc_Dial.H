#pragma once
#include <FL/Fl_Dial.H>
#include "WidgetPDial.h"
#include "Fl_Osc_Widget.H"
#include <string>

class Fl_Osc_Dial:public WidgetPDial, public Fl_Osc_Widget
{

    public:
        Fl_Osc_Dial(int X, int Y, int W, int H, const char *label = NULL);
        virtual ~Fl_Osc_Dial(void);

        //Hack to get non-local controls up and running
        void alt_init(std::string base, std::string path_);


        //Normal Initialization
        void init(std::string path);
        //void OSC_value(float);
        void OSC_value(int);
        void OSC_value(char);
        using Fl_Osc_Widget::OSC_value;

        //Refetch parameter information
        void update(void);
        void callback(Fl_Callback *cb, void *p = NULL);

        //Midi learn handler
        int handle(int);

        void mark_dead(void);
        virtual void rebase(std::string new_base) override;

        void cb(void);
    private:
        bool alt_style;
        bool dead;
        std::pair<Fl_Callback*, void*> cb_data;
};
