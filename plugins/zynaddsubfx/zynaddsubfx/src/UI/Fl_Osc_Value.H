#include <FL/Fl_Value_Input.H>
#include "Fl_Osc_Widget.H"

class Fl_Osc_Value: public Fl_Value_Input, public Fl_Osc_Widget
{
    public:
        Fl_Osc_Value(int X, int Y, int W, int H, const char *label = NULL);
        virtual ~Fl_Osc_Value(void);
        
        //Normal Initialization
        void init(const char *path);
};
