#include "Fl_Osc_Output.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

static void callback_fn(Fl_Widget *w, void *)
{
    ((Fl_Osc_Output*)w)->cb();
}

Fl_Osc_Output::Fl_Osc_Output(int X, int Y, int W, int H, const char *label)
    :Fl_Value_Output(X,Y,W,H, label), Fl_Osc_Widget(this)
{
    Fl_Value_Output::callback(callback_fn);
}


void Fl_Osc_Output::init(const char *path)
{
    name = path;
    oscRegister(path);
};

void Fl_Osc_Output::callback(Fl_Callback *cb, void *p)
{
    cb_data.first = cb;
    cb_data.second = p;
}

void Fl_Osc_Output::OSC_value(char v)
{
    newvalue_ = v;
    value(v);
    
    //Hide the fact that this widget is async
    if(cb_data.first)
        cb_data.first(this, cb_data.second);
}

void Fl_Osc_Output::OSC_value(float v)
{
    newvalue_ = v;
    value(v);
   
    //Hide the fact that this widget is async
    if(cb_data.first)
        cb_data.first(this, cb_data.second);
}
        
void Fl_Osc_Output::update(void)
{
    oscWrite(name);
}

float Fl_Osc_Output::newvalue(void) const
{
    return newvalue_;
}

void Fl_Osc_Output::cb(void)
{
    oscWrite(name);
}
