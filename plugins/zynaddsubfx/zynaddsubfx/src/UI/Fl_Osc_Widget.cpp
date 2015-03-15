#include "Fl_Osc_Widget.H"
#include <rtosc/rtosc.h>

Fl_Osc_Widget::Fl_Osc_Widget(void) //Deprecated
:loc(), osc(NULL)
{}

Fl_Osc_Widget:: Fl_Osc_Widget(Fl_Widget *self)
{
    assert(fetch_osc_pane(self));
    if(auto *pane = fetch_osc_pane(self)) {
        osc = pane->osc;
        loc = pane->loc();
    }
    assert(osc);
}

Fl_Osc_Widget::~Fl_Osc_Widget(void)
{
    if(osc)
        osc->removeLink(this);
};

void Fl_Osc_Widget::OSC_value(float) {}
void Fl_Osc_Widget::OSC_value(bool) {}
void Fl_Osc_Widget::OSC_value(int) {}
void Fl_Osc_Widget::OSC_value(char) {}
void Fl_Osc_Widget::OSC_value(unsigned,void*) {}
void Fl_Osc_Widget::OSC_value(const char *) {}

//labeled forwarding methods
void Fl_Osc_Widget::OSC_value(float x, const char *) {OSC_value(x);}
void Fl_Osc_Widget::OSC_value(bool x, const char *) {OSC_value(x);}
void Fl_Osc_Widget::OSC_value(int x, const char *) {OSC_value(x);}
void Fl_Osc_Widget::OSC_value(char x, const char *) {OSC_value(x);}
void Fl_Osc_Widget::OSC_value(unsigned x, void *v, const char *) {OSC_value(x,v);}
void Fl_Osc_Widget::OSC_value(const char *x, const char *) {OSC_value(x);}

void Fl_Osc_Widget::OSC_raw(const char *)
{}


void Fl_Osc_Widget::oscWrite(std::string path, const char *args, ...)
{
    char buffer[1024];
    //puts("writing OSC");
    //printf("Path = '%s'\n", path.c_str());

    va_list va;
    va_start(va, args);

    if(rtosc_vmessage(buffer, 1024, (loc+path).c_str(), args, va))
        osc->writeRaw(buffer);
    else
        puts("Dangerous Event ommision");

    va_end(va);
    ////Try to pretty print basic events
    //if(!strcmp(args, "c") || !strcmp(args, "i"))
    //    printf("Args = ['%d']\n", rtosc_argument(buffer, 0).i);
    //if(!strcmp(args, "f"))
    //    printf("Args = ['%f']\n", rtosc_argument(buffer, 0).f);
    //if(!strcmp(args, "T"))
    //    printf("Args = [True]\n");
    //if(!strcmp(args, "F"))
    //    printf("Args = [False]\n");
}

void Fl_Osc_Widget::oscWrite(std::string path)
{
    osc->requestValue(loc+path);
}

void Fl_Osc_Widget::oscRegister(const char *path)
{
    osc->createLink(loc+path, this);
    osc->requestValue(loc+path);
}

void Fl_Osc_Widget::update(void)
{
    if(*((loc+ext).rbegin()) != '/')
        osc->requestValue(loc+ext);
}

Fl_Osc_Pane *Fl_Osc_Widget::fetch_osc_pane(Fl_Widget *w)
{
    if(!w)
        return NULL;

    Fl_Osc_Pane *pane = dynamic_cast<Fl_Osc_Pane*>(w->parent());
    if(pane)
        return pane;
    return fetch_osc_pane(w->parent());
}


void Fl_Osc_Widget::rebase(std::string new_base)
{
    osc->renameLink(loc+ext, new_base+ext, this);
    loc = new_base;
    osc->requestValue(loc+ext);
}

void Fl_Osc_Widget::oscMove(std::string new_ext)
{
    osc->renameLink(loc+ext, loc+new_ext, this);
    ext = new_ext;
    osc->requestValue(loc+ext);
}

void Fl_Osc_Widget::oscMove(std::string old_loc, std::string new_loc)
{
    osc->renameLink(old_loc, new_loc, this);
    osc->requestValue(new_loc);
}
