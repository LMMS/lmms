#include "Fl_Osc_Value.H"

Fl_Osc_Value::Fl_Osc_Value(int X, int Y, int W, int H, const char *label)
    :Fl_Value_Input(X,Y,W,H, label), Fl_Osc_Widget(this)
{
}

Fl_Osc_Value::~Fl_Osc_Value(void)
{}

void Fl_Osc_Value::init(const char *path)
{
    (void)path;
}

