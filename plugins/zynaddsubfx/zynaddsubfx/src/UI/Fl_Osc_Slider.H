#pragma once
#include <FL/Fl_Slider.H>
#include "Fl_Osc_Widget.H"
#include <string>

class Fl_Osc_Slider:public Fl_Slider, public Fl_Osc_Widget
{

    public:
        Fl_Osc_Slider(int X, int Y, int W, int H, const char *label = NULL);
       // string name,
        //        const char *metadata);

        virtual ~Fl_Osc_Slider(void);
        void OSC_value(int) override;
        void OSC_value(float) override;
        void OSC_value(char) override;
        void init(std::string, char type = 'f');

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
