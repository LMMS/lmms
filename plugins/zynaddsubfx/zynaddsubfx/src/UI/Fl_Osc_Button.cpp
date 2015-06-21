#include "Fl_Osc_Button.H"
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

Fl_Osc_Button::Fl_Osc_Button(int X, int Y, int W, int H, const char *label)
    :Fl_Button(X,Y,W,H,label), Fl_Osc_Widget(this)
{
}

Fl_Osc_Button::~Fl_Osc_Button(void)
{}

void Fl_Osc_Button::OSC_value(bool v)
{
    Fl_Button::value(v);
}

void Fl_Osc_Button::rebase(std::string)
{
}
