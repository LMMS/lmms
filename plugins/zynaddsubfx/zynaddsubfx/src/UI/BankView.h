#ifndef BANKVIEW_H
#define BANKVIEW_H

#include "Fl_Osc_Widget.H"
#include "Fl_Osc_Choice.H"
#include <FL/Fl_Group.H>
#include <FL/Fl_Check_Button.H>
#include <string>

#include "common.H"

class Bank;
class BankView;
class Fl_Light_Button;

class BankList : public Fl_Osc_Choice
{
    public:
        BankList(int x,int y, int w, int h, const char *label=0);
        void init(std::string path);
        void OSC_raw(const char *msg);
};

class BankSlot : public Fl_Button
{
    public:
        BankSlot(int x,int y, int w, int h, const char *label=0);
        int handle(int event);
        void init(int nslot_, BankView *bv_);

        void update(const char *name__, const char *fname__);

        bool empty(void) const;
        const char *name(void) const;
        const char *filename(void) const;
    private:
        std::string name_;
        std::string filename_;
        char labelstr[128];
        int nslot;
        BankView *bv;
};

class BankViewControls: public Fl_Group
{
    public:
        BankViewControls(int x,int y, int w, int h, const char *label=0);
        void init(BankView *bv_);


        int mode(void) const;
        void mode(int);

    private:
        Fl_Check_Button *read;
        Fl_Check_Button *write;
        Fl_Check_Button *clear;
        Fl_Check_Button *swap;

        //1 -> read
        //2 -> write
        //3 -> clear
        //4 -> swap
        int mode_;

        static void cb_clearbutton(Fl_Light_Button*, void*);
        static void cb_readbutton(Fl_Light_Button*, void*);
        static void cb_writebutton(Fl_Light_Button*, void*);
};

class BankView: public Fl_Group, public Fl_Osc_Widget
{
    public:
        BankView(int x,int y, int w, int h, const char *label=0);
        ~BankView(void);
        void init(Fl_Osc_Interface *osc_, BankViewControls *bvc_, int *npart_);

        void react(int event, int slot);

        virtual void OSC_raw(const char *msg) override;
        void cbwig(Fl_Widget *w);

        void refresh(void);
    private:
        BankViewControls *bvc;
        BankSlot *slots[160];

        Fl_Osc_Interface *osc;
        std::string loc;

        //XXX TODO locked banks...
        int nselected;
        int *npart;

        Fl_Widget *cbwig_;
};

#endif
