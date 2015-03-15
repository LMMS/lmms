#pragma once
#include <string>
#include <cstring>
#include <cassert>
#include <cmath>
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <FL/Fl_Group.H>

class Fl_Osc_Widget
{
    public:
        Fl_Osc_Widget(void); //Deprecated
        Fl_Osc_Widget(Fl_Widget *self);
        virtual ~Fl_Osc_Widget(void);

        //Callback methods
        virtual void OSC_value(float);
        virtual void OSC_value(bool);
        virtual void OSC_value(int);
        virtual void OSC_value(char);
        virtual void OSC_value(unsigned,void*);
        virtual void OSC_value(const char *);

        //labeled forwarding methods
        virtual void OSC_value(float x, const char *);
        virtual void OSC_value(bool x, const char *);
        virtual void OSC_value(int x, const char *);
        virtual void OSC_value(char x, const char *);
        virtual void OSC_value(unsigned x, void *v, const char *);
        virtual void OSC_value(const char *x, const char *);

        //Raw messages
        virtual void OSC_raw(const char *);
        
        //Widget methods
        void oscWrite(std::string path, const char *args, ...);
        void oscWrite(std::string path);
        void oscRegister(const char *path);

        //Forces an update of parameters as they have become stale somehow
        virtual void update(void);

        //Smoothly change the base path
        virtual void rebase(std::string new_base);
        void oscMove(std::string new_ext);
        //Explict version for weirdly routed controls
        void oscMove(std::string old_loc, std::string new_loc);

        //Base path
        std::string loc;
        //Extension
        std::string ext;
        Fl_Osc_Interface *osc;
    protected:
        Fl_Osc_Pane *fetch_osc_pane(Fl_Widget *w);
};
