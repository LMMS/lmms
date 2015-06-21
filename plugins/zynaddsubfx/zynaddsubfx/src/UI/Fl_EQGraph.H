#pragma once
#include "Fl_Osc_Widget.H"
#include <FL/Fl_Box.H>
#include "../globals.h"

class EffectMgr;
class Fl_Osc_Interface;
class Fl_EQGraph:public Fl_Box, public Fl_Osc_Widget
{
    public:
        Fl_EQGraph(int x,int y, int w, int h, const char *label=0);
        virtual ~Fl_EQGraph(void);
        void draw(void);

        void OSC_raw(const char *msg);
        void update(void);

    private:
        void draw_freq_line(float freq,int type);

        double getresponse(int maxy,float freq) const;

        float getfreqx(float x) const;
        float getfreqpos(float freq) const;

        float num[MAX_EQ_BANDS*MAX_FILTER_STAGES*2+1];
        float dem[MAX_EQ_BANDS*MAX_FILTER_STAGES*2+1];
};
