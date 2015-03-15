#ifndef VU_METER_H
#define VU_METER_H
#include <FL/Fl_Box.H>

class VuMeter: public Fl_Box
{
    public:

        VuMeter(int x,int y, int w, int h, const char *label=0)
            :Fl_Box(x,y,w,h,label)
        {}

    protected:
        float limit(float x)
        {
            if(x<0.0)
                x=0.0;
            else if(x>1.0)
                x=1.0;
            return x;
        }
}; 
#endif
