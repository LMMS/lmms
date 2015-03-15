#pragma once
#include <FL/Fl_Value_Output.H>
#include "Fl_Osc_Widget.H"

class Fl_Osc_Output:public Fl_Value_Output, Fl_Osc_Widget
{
    public:
        Fl_Osc_Output(int x, int y, int w, int h, const char *label = NULL);
        void init(const char *path);
        void OSC_value(char);
        void OSC_value(float);
        using Fl_Osc_Widget::OSC_value;
        
        void update(void);
        void callback(Fl_Callback *cb, void *p = NULL);

        float newvalue() const;

        void cb(void);
    private:
        float newvalue_;
        std::string name;
        std::pair<Fl_Callback*, void*> cb_data;
};
