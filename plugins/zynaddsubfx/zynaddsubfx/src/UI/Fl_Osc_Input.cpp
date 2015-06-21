#include "Fl_Osc_Input.H"

Fl_Osc_Input::Fl_Osc_Input(int X, int Y, int W, int H, const char *label)
    :Fl_Input(X,Y,W,H, label), Fl_Osc_Widget(this)
{
}

Fl_Osc_Input::~Fl_Osc_Input(void)
{}

void Fl_Osc_Input::init(const char *path)
{
    ext = path;
    oscRegister(path);
}

void Fl_Osc_Input::OSC_value(const char *v)
{
    value(v);
}
