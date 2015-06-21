#include "PartNameButton.h"

PartNameButton::PartNameButton(int X, int Y, int W, int H, const char *label)
    :Fl_Button(X,Y,W,H,label), Fl_Osc_Widget(this)
{
}

void PartNameButton::OSC_value(const char *label_)
{
    the_string = label_;
    label(the_string.c_str());
}
