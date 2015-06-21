#include <FL/Fl.H>
#include "Fl_Osc_VSlider.H"
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

Fl_Osc_VSlider::Fl_Osc_VSlider(int X, int Y, int W, int H, const char *label)
    :Fl_Value_Slider(X,Y,W,H,label), Fl_Osc_Widget(this), cb_data(NULL, NULL)
{
    //bounds(0.0f,1.0f);
    Fl_Slider::callback(Fl_Osc_VSlider::_cb);
}

void Fl_Osc_VSlider::init(std::string path_, char type_)
{
    osc_type = type_;
    ext = path_;
    oscRegister(ext.c_str());
}

Fl_Osc_VSlider::~Fl_Osc_VSlider(void)
{}

void Fl_Osc_VSlider::OSC_value(char v)
{
    Fl_Slider::value(v+minimum());
}
        
void Fl_Osc_VSlider::OSC_value(int v)
{
    Fl_Slider::value(v+minimum());
}

void Fl_Osc_VSlider::OSC_value(float v)
{
    Fl_Slider::value(v+minimum());
}

void Fl_Osc_VSlider::cb(void)
{
    const float val = Fl_Slider::value();
    if(osc_type == 'f')
        oscWrite(ext, "f", val-minimum());
    else if(osc_type == 'i')
        oscWrite(ext, "i", (int)(val-minimum()));
    else {
	fprintf(stderr, "invalid `c' from vslider %s%s, using `i'\n", loc.c_str(), ext.c_str());
	oscWrite(ext, "i", (int)(val-minimum()));
    }
    //OSC_value(val);
    
    if(cb_data.first)
        cb_data.first(this, cb_data.second);
}

void Fl_Osc_VSlider::callback(Fl_Callback *cb, void *p)
{
    cb_data.first = cb;
    cb_data.second = p;
}

int Fl_Osc_VSlider::handle(int ev)
{
    bool middle_mouse = (ev == FL_PUSH && Fl::event_state(FL_BUTTON2));
    bool ctl_click    = (ev == FL_PUSH && Fl::event_state(FL_BUTTON1) && Fl::event_ctrl());
    if(middle_mouse || ctl_click) {
        printf("Trying to learn...\n");
        osc->write("/learn", "s", (loc+ext).c_str());
        return 1;
    }
    return Fl_Value_Slider::handle(ev);
}

void Fl_Osc_VSlider::update(void)
{
    oscWrite(ext, "");
}

void Fl_Osc_VSlider::_cb(Fl_Widget *w, void *)
{
    static_cast<Fl_Osc_VSlider*>(w)->cb();
}
