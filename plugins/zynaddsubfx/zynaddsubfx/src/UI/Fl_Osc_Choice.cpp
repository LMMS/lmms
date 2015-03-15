#include "Fl_Osc_Choice.H"
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

static void callback_fn(Fl_Widget *w, void *)
{
    ((Fl_Osc_Choice*)w)->cb();
}

Fl_Osc_Choice::Fl_Osc_Choice(int X, int Y, int W, int H, const char *label)
    :Fl_Choice(X,Y,W,H, label), Fl_Osc_Widget(this), cb_data(NULL, NULL)
{
    min = 0;
    Fl_Choice::callback(callback_fn, NULL);
}

void Fl_Osc_Choice::init(std::string path_, int base)
{
    min = base;
    ext = path_;
    Fl_Osc_Pane *pane = fetch_osc_pane(this);
    assert(pane);
    assert(pane->osc);
    osc = pane->osc;
    oscRegister(path_.c_str());
};

Fl_Osc_Choice::~Fl_Osc_Choice(void)
{}
        
void Fl_Osc_Choice::callback(Fl_Callback *cb, void *p)
{
    cb_data.first = cb;
    cb_data.second = p;
}

void Fl_Osc_Choice::OSC_value(int v)
{
    value(v-min);
    if(cb_data.first)
        cb_data.first(this, cb_data.second);
}

void Fl_Osc_Choice::OSC_value(char v)
{
    OSC_value((int)v);
}

void Fl_Osc_Choice::cb(void)
{
    assert(osc);
    oscWrite(ext, "i", value()+min);
    if(cb_data.first)
        cb_data.first(this, cb_data.second);
}

void Fl_Osc_Choice::update(void)
{
    assert(osc);
    oscWrite(ext);
}

