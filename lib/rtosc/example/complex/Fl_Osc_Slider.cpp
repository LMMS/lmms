#include "Fl_Osc_Slider.H"
#include "Fl_Osc_Interface.H"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

template<typename A, typename B>
B string_cast(const A &a)
{
    std::stringstream s;
    s.precision(3);
    B b;
    s << " " << a << " ";
    s >> b;
    return b;
}

Fl_Osc_Slider::Fl_Osc_Slider(int X, int Y, int W, int H, string n, const char *m)
    :Fl_Slider(X,Y,W,H), Fl_Osc_Widget(n,m)
{
    bounds(0.0f,1.0f);
    callback(Fl_Osc_Slider::_cb);

    Fl_Osc_Pane *pane = dynamic_cast<Fl_Osc_Pane*>(parent());
    assert(pane);
    osc = pane->osc;
    osc->createLink(full_path, this);
    osc->requestValue(full_path);
}

Fl_Osc_Slider::~Fl_Osc_Slider(void)
{
    osc->removeLink(full_path, this);
}

void Fl_Osc_Slider::OSC_value(float v)
{
    real_value = v;
    const float val = inv_translate(v);
    Fl_Slider::value(val);
    label_str = string_cast<float,string>(v);
    label("                ");
    label(label_str.c_str());
}

void Fl_Osc_Slider::cb(void)
{
    const float val = translate(Fl_Slider::value());
    osc->writeValue(full_path, val);
    OSC_value(val);

    label_str = string_cast<float,string>(val);
    label("                ");
    label(label_str.c_str());
}

void Fl_Osc_Slider::_cb(Fl_Widget *w, void *)
{
    static_cast<Fl_Osc_Slider*>(w)->cb();
}
