#include "Fl_Osc_Pane.H"
#include "Fl_Osc_Widget.H"
#include <cassert>
#include <cstdio>

Fl_Osc_Pane::Fl_Osc_Pane(void)
    :osc(NULL), base()
{}


Fl_Osc_Window::Fl_Osc_Window(int w, int h, const char *L)
    :Fl_Double_Window(w,h,L)
{}
        
void Fl_Osc_Window::init(Fl_Osc_Interface *osc_, std::string loc_)
{
    osc   = osc_;
    base = loc_;
}

std::string Fl_Osc_Window::loc(void) const
{
    return base;
}

static void nested_update(Fl_Group *g)
{
    unsigned nchildren = g->children();
    for(unsigned i=0; i < nchildren; ++i) {
        Fl_Widget *widget = g->child(i);
        if(Fl_Osc_Widget *o = dynamic_cast<Fl_Osc_Widget*>(widget)) {
            o->update();
        } else if(Fl_Group *o = dynamic_cast<Fl_Group*>(widget)) {
            nested_update(o);
        }

    }
}

void Fl_Osc_Window::update(void)
{
    unsigned nchildren = this->children();
    for(unsigned i=0; i < nchildren; ++i) {
        Fl_Widget *widget = this->child(i);
        if(Fl_Osc_Widget *o = dynamic_cast<Fl_Osc_Widget*>(widget)) {
            o->update();
        }
        if(dynamic_cast<Fl_Group*>(widget))
            nested_update(dynamic_cast<Fl_Group*>(widget));
    }
}

static void nested_rebase(Fl_Group *g, std::string new_base)
{
    unsigned nchildren = g->children();
    for(unsigned i=0; i < nchildren; ++i) {
        Fl_Widget *widget = g->child(i);
        if(Fl_Osc_Widget *o = dynamic_cast<Fl_Osc_Widget*>(widget)) {
            o->rebase(new_base);
        } else if(Fl_Osc_Group *o = dynamic_cast<Fl_Osc_Group*>(widget)) {
            o->rebase(new_base);
        } else if(Fl_Group *o = dynamic_cast<Fl_Group*>(widget)) {
            nested_rebase(o, new_base);
        }

    }
}

void Fl_Osc_Window::rebase(std::string new_base)
{
    unsigned nchildren = this->children();
    for(unsigned i=0; i < nchildren; ++i) {
        Fl_Widget *widget = this->child(i);
        if(Fl_Osc_Widget *o = dynamic_cast<Fl_Osc_Widget*>(widget)) {
            o->rebase(new_base);
        }
        if(Fl_Osc_Group *o = dynamic_cast<Fl_Osc_Group*>(widget))
            o->rebase(new_base);
        else if(dynamic_cast<Fl_Group*>(widget))
            nested_rebase(dynamic_cast<Fl_Group*>(widget), new_base);
    }
}

static Fl_Osc_Pane *find_osc_pane(Fl_Widget *root)
{
    if(!root)
        return NULL;
    Fl_Group *next = root->parent();

    if(auto *p = dynamic_cast<Fl_Osc_Pane*>(next))
        return p;
    else
        return find_osc_pane(next);
}

Fl_Osc_Group::Fl_Osc_Group(int x, int y, int w, int h, const char *L)
:Fl_Group(x,y,w,h,L)
{
    if(auto *p = find_osc_pane(this)) {
        osc  = p->osc;
        base = p->loc();
        assert(osc);
    }
};


std::string Fl_Osc_Group::loc(void) const
{
    return base + ext;
}


void Fl_Osc_Group::rebase(std::string new_base)
{
    nested_rebase(this, new_base+ext);
    base = new_base;
}

void Fl_Osc_Group::reext(std::string new_ext)
{
    nested_rebase(this, base+new_ext);
    ext = new_ext;
}
