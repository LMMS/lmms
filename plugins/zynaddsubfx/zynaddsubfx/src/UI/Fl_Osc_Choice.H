#pragma once
#include <FL/Fl_Choice.H>
#include "Fl_Osc_Widget.H"
#include <string>

class Fl_Osc_Choice:public Fl_Choice, public Fl_Osc_Widget
{

    public:
        Fl_Osc_Choice(int X, int Y, int W, int H, const char *label = NULL);
        virtual ~Fl_Osc_Choice(void);
        //Base is for braindead choices that don't actually start at zero
        void init(std::string path, int base =0);


        //void OSC_value(float);
        void OSC_value(int);
        void OSC_value(char);

        //Refetch parameter information
        void update(void);
        void callback(Fl_Callback *cb, void *p = NULL);
        
        void cb(void);
    private:
        int min;
        std::pair<Fl_Callback*, void*> cb_data;
};
