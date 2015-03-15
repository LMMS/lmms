#pragma once
#include <FL/Fl_Value_Slider.H>
#include "Fl_Osc_Widget.H"
#include <string>

class Fl_Osc_VSlider:public Fl_Value_Slider, public Fl_Osc_Widget
{

    public:
        Fl_Osc_VSlider(int X, int Y, int W, int H, const char *label = NULL);

        virtual ~Fl_Osc_VSlider(void);
        void OSC_value(char);
        void OSC_value(int);
        void OSC_value(float);
        void init(std::string, char type = 'i');

        //Refetch parameter information
        void update(void);
        void callback(Fl_Callback *cb, void *p = NULL);
        
        //MIDI Learn
        int handle(int);

        void cb(void);
        static void _cb(Fl_Widget *w, void *);
    private:
        char osc_type;
        std::pair<Fl_Callback*, void*> cb_data;
};
