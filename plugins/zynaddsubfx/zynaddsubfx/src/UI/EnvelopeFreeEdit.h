#include <FL/Fl_Box.H>
#include "../Params/EnvelopeParams.h"
#include "Fl_Osc_Widget.H"


//Define the types of envelope (TODO a properly located enum)
//TODO check if ASR should be ASR  *OR* ADR

#define ENV_ADSR 1
//#define ENV_ADSR 2
#define ENV_ASR 3
#define ENV_ADSR_FILTER 4
#define ENV_ADSR_BW 5

class EnvelopeFreeEdit : public Fl_Box, Fl_Osc_Widget
{
    public:
        EnvelopeFreeEdit(int x,int y, int w, int h, const char *label=0);
        void init(void);
        void setpair(Fl_Box *pair_);
        int handle(int event);

        void draw(void);
        void OSC_raw(const char *msg) override;
        void update(void);


        int lastpoint;

        //How many points
        char Penvpoints;
    private:
        int getpointx(int n) const;
        int getpointy(int n) const;
        int getnearest(int x,int y) const;
        float getdt(int i) const;

        Fl_Box *pair; //XXX what the heck is this?

        //cursor state
        int currentpoint, cpx, cpdt;

        //The Points
        char Penvdt[MAX_ENVELOPE_POINTS];
        char Penvval[MAX_ENVELOPE_POINTS];
        //The Sustain point
        char Penvsustain;
};
