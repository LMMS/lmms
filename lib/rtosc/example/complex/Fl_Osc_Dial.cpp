#include "Fl_Osc_Dial.H"
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

Fl_Osc_Dial::Fl_Osc_Dial(int X, int Y, int W, int H, string n,
        const char *m)
    :Fl_Dial(X,Y,W,H), Fl_Osc_Widget(n,m)
{
    bounds(0.0f,1.0f);
    callback(Fl_Osc_Dial::_cb);

    Fl_Osc_Pane *pane = dynamic_cast<Fl_Osc_Pane*>(parent());
    assert(pane);
    osc = pane->osc;
    osc->createLink(full_path, this);
    osc->requestValue(full_path);
};

Fl_Osc_Dial::~Fl_Osc_Dial(void)
{
    osc->removeLink(full_path, this);
}

void Fl_Osc_Dial::OSC_value(float v)
{
    printf("setting osc value via '%f'\n", v);
    real_value = v;

    const float val = Fl_Osc_Widget::inv_translate(v);

    Fl_Dial::value(val);
    string tmp = string_cast<float,string>(v);
    memset(label_str, 0, sizeof(label_str));
    strncpy(label_str, tmp.c_str(), sizeof(label_str)-1);
    label("                ");
    label(label_str);
}

void Fl_Osc_Dial::cb(void)
{
    const float val = translate(Fl_Dial::value());

    osc->writeValue(full_path, val);
    OSC_value(val);

    string tmp = string_cast<float,string>(val);
    memset(label_str, 0, sizeof(label_str));
    strncpy(label_str, tmp.c_str(), sizeof(label_str)-1);
    label("                ");
    label(label_str);
}

void Fl_Osc_Dial::_cb(Fl_Widget *w, void *)
{
    static_cast<Fl_Osc_Dial*>(w)->cb();
}
