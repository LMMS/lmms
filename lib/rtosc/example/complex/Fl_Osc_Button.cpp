#include "Fl_Osc_Button.H"
#include "Fl_Osc_Interface.H"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

Fl_Osc_Button::Fl_Osc_Button(int X, int Y, int W, int H, const char *n,
        const char *m)
    :Fl_Button(X,Y,W,H), Fl_Osc_Widget(n,m)
{
    label(n);
    callback(Fl_Osc_Button::_cb);

    Fl_Osc_Pane *pane = dynamic_cast<Fl_Osc_Pane*>(parent());
    assert(pane);
    osc = pane->osc;
    assert(osc);
    osc->createLink(full_path, this);
    osc->requestValue(full_path);
}

Fl_Osc_Button::~Fl_Osc_Button(void)
{
    osc->removeLink(full_path, this);
}

void Fl_Osc_Button::OSC_value(bool v)
{
    Fl_Button::value(v);
}

void Fl_Osc_Button::cb(void)
{
    osc->writeValue(full_path, (bool) value());
}

void Fl_Osc_Button::_cb(Fl_Widget *w, void *)
{
    static_cast<Fl_Osc_Button*>(w)->cb();
}
